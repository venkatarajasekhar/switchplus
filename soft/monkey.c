
#include "callback.h"
#include "monkey.h"
#include "pin.h"
#include "registers.h"
#include "usb.h"

#include <stdarg.h>
#include <stddef.h>

// The chip-select to the console, we configure as a GPIO.
#define CONSOLE_CS (&GPIO_BYTE[7][19])


// Buffer for log text.
static unsigned char monkey_buffer[4096] __aligned (4096)
    __section ("ahb0.monkey_buffer");
#define monkey_buffer_end (monkey_buffer + sizeof monkey_buffer)

// Buffer for incoming characters.
static unsigned char monkey_recv[1024] __aligned (512)
    __section ("ahb0.monkey_recv");


// Ring-buffer descriptor.  We keep at least one byte free, so that
// insert==limit implies the buffer is empty.

// Position for inserting characters.
static unsigned char * insert_pos = monkey_buffer;
// Limit for inserting characters.
static unsigned char * limit_pos = monkey_buffer;

// First in-flight and next-to-queue over USB.
static unsigned char * usb_flight_pos = monkey_buffer;
static unsigned char * usb_send_pos = monkey_buffer;

// First in-flight and next-to-queue over SSP.
static unsigned char * ssp_flight_pos = monkey_buffer;
static unsigned char * ssp_send_pos = monkey_buffer;

static int monkey_in_next = -1;         // For ungetc.
static struct {
    unsigned char * next;
    unsigned char * end;
} monkey_recv_pos[2];


static void monkey_in_complete (dTD_t * dtd, unsigned status, unsigned remain);
static void monkey_out_complete (dTD_t * dtd, unsigned status, unsigned remain);

bool debug_flag;
bool verbose_flag;
static bool log_ssp;


void init_monkey_usb (void)
{
    qh_init (0x03, 0x20000000);         // No 0-size-frame on the monkey.
    qh_init (0x83, 0x20000000);
    endpt->ctrl[3] = 0x00c800c8;

    usb_send_pos = usb_flight_pos;

    monkey_kick();

    schedule_buffer (3, monkey_recv, 512, monkey_out_complete);
    schedule_buffer (3, monkey_recv + 512, 512, monkey_out_complete);

    monkey_recv_pos[0].next = 0;
    monkey_recv_pos[0].end  = 0;
    monkey_recv_pos[1].next = 0;
    monkey_recv_pos[1].end  = 0;
}


void init_monkey_ssp (void)
{
    *BASE_SSP1_CLK = 0x0c000800;        // Base clock is 160MHz.

    RESET_CTRL[1] = (1 << 19) | (1 << 24); // Reset ssp1, keep m0 in reset.
    while (!(RESET_ACTIVE_STATUS[1] & (1 << 19)));

    SSP1->cpsr = 2;                     // Clock pre-scale: 160MHz / 2 = 80MHz.
    // 8-bit xfer, clock low between frames, capture on first (rising) edge of
    // frame (we'll output on falling edge).  No divide.
    SSP1->cr0 = 0x0007;
    SSP1->cr1 = 2;                      // Enable master.

    SSP1->dmacr = 2;                    // TX DMA enabled.

    // Setup pins; make CS a GPIO output, pulse it high for a bit.
    GPIO_DIR[7] |= 1 << 19;
    *CONSOLE_CS = 1;

    static const unsigned pins[] = {
        PIN_OUT_FAST(15,4,0),           // SCK is D10, PF_4 func 0.
        PIN_OUT_FAST(15,5,4),           // SSEL is E9, PF_5, GPIO7[19] func 4.
        PIN_IO_FAST (15,6,2),           // MISO is E7, PF_6 func 2.
        PIN_OUT_FAST(15,7,2),           // MOSI is B7, PF_7 func 2.
    };
    config_pins(pins, sizeof pins / sizeof pins[0]);

    // Leave CS low.
    *CONSOLE_CS = 0;

    GPDMA->config = 1;                  // Enable.

    ssp_send_pos = monkey_buffer;
    ssp_flight_pos = monkey_buffer;

    log_ssp = true;

    monkey_kick();
}


void monkey_ssp_off(void)
{
    __interrupt_disable();
    while (ssp_send_pos != ssp_flight_pos)
        __interrupt_wait_go();
    log_ssp = false;
    __interrupt_enable();

    // Wait for idle & clear out the fifo..
    while (SSP1->sr & 20)
        SSP1->dr;

    *CONSOLE_CS = 1;
}


void monkey_ssp_on(void)
{
    *CONSOLE_CS = 0;

    __interrupt_disable();
    log_ssp = true;
    monkey_kick();
    __interrupt_enable();
}


static inline unsigned current_irs (void)
{
    unsigned r;
    asm ("\tmrs %0,psr\n" : "=r"(r));
    return r & 511;
}


static inline void enter_monkey(void)
{
    __interrupt_disable();
}


static inline void leave_monkey(void)
{
    monkey_kick();
    __interrupt_enable();
}

static inline unsigned min(unsigned x, unsigned y)
{
    return x < y ? x : y;
}


static inline unsigned headroom(unsigned char * p)
{
    return (p - insert_pos - 1) & 4095;
}


static unsigned char * advance(unsigned char * p, int amount)
{
    return monkey_buffer + ((amount + (int) p) & 4095);
}


static unsigned free_monkey_space_usb(void)
{
    // Running at high priority : discard.
    endpt_complete_one(0x83);
    int allowed = headroom(usb_flight_pos);
    if (allowed != 0)
        return allowed;

    // Last resort - just bump the pointer.
    usb_flight_pos = advance(usb_flight_pos, 512);
    return 512;
}


static unsigned free_monkey_space_ssp(void)
{
    // We just did a kick, so if the buffer pointers are equal, that is
    // because ssp is off.  In that case, just advance pointers.
    if (ssp_flight_pos == ssp_send_pos) {
        ssp_send_pos = advance(insert_pos, 512);
        ssp_flight_pos = ssp_send_pos;
        return 512;
    }

    // We could just make the GPDMA interrupt higher priority?
    unsigned allowed;
    do {                                // We may spin on other interrupts...
        gpdma_interrupt();
        allowed = headroom(ssp_flight_pos);
    }
    while (allowed == 0);
    return allowed;
}


static void free_monkey_space(void)
{
    monkey_kick();

    // Recalculate buffer positions...  Note that both the free... functions
    // can enable interrupts and invalidate everything, meaning we need to
    // redo the entire calculation.
retry: ;
    unsigned allowed_usb = headroom(usb_flight_pos);
    unsigned allowed_ssp = headroom(ssp_flight_pos);

    if (allowed_usb == 0 || allowed_ssp == 0) {
        if (current_irs() == 0) {       // Low priority - wait and retry.
            __interrupt_wait_go();
            goto retry;
        }
        if (allowed_usb == 0)
            allowed_usb = free_monkey_space_usb();
        if (allowed_ssp == 0)
            allowed_ssp = free_monkey_space_ssp();
    }

    unsigned allowed = 512;
    allowed = min(allowed, allowed_usb);
    allowed = min(allowed, allowed_ssp);

    if (insert_pos == monkey_buffer_end)
        insert_pos = monkey_buffer;

    allowed = min(allowed, monkey_buffer_end - insert_pos);
    allowed = min(allowed, 512);

    limit_pos = insert_pos + allowed;
}


static void write_byte (int byte)
{
    if (__builtin_expect(insert_pos == limit_pos, 0))
        free_monkey_space();

    *insert_pos++ = byte;
}


void putchar (int byte)
{
    enter_monkey();
    write_byte (byte);
    leave_monkey();
}


void puts (const char * s)
{
    enter_monkey();
    for (; *s; s++)
        write_byte (*s);
    leave_monkey();
}


void monkey_start_ssp(void)
{
    // We should only be called when the DMA is idle.
    unsigned amount = 512;
    amount = min(amount, insert_pos - ssp_send_pos);
    amount = min(amount, monkey_buffer_end - ssp_send_pos);

    if (amount == 0)
        return;

    // With mux option 0, SSP1 TX is peripheral 12.
    volatile gpdma_channel_t * channel = &GPDMA->channel[0];
    channel->srcaddr = ssp_send_pos;
    channel->destaddr = &SSP1->dr;
    channel->lli = NULL;
    // Bit 12..14 : Src burst size 1 = 4 bytes.
    // Bit 15..17 : Dst burst size 1 = 4 bytes.
    // Bit 18..20 : Swidth 0/1/2 = 8/16/32 bit
    // Bit 21..22 : Dwidth 0/1/2 = 8/16/32 bit
    // Bit 24 : src on master1.
    // Bit 25 : dest on master1; only master1 can access peripherals.
    // Bit 26 : source address increment.
    // Bit 28 : priviledged mode.
    // Bit 31 : terminal count interrupt.
    channel->control
        = (1<<31) + (1<<28) + (1<<26) + (1<<25) + (1<<15) + (1<<12) + amount;
    // Bit 0 : Enable.
    // Bits 10..6 : Dest periph = 12.
    // Bits 13..11 : Flow control = 1, mem to perip, DMA control.
    // Bit 14 : Error interrupt enable.
    // Bit 15 : Terminal interrupt enable.
    channel->config = (1 << 15) + (1 << 14) + (1 << 11) + (12 << 6) + 1;

    ssp_send_pos = advance(ssp_send_pos, amount);
}


void gpdma_interrupt(void)
{
    // FIXME - don't forget global initialisation.
    unsigned tcstat = GPDMA->inttcstat;
    GPDMA->inttcclear = tcstat;
    unsigned tcerr = GPDMA->interrstat;
    GPDMA->interrclr = tcerr;

    if ((tcstat | tcerr) & 1) {         // Channel just finished.
        ssp_flight_pos = ssp_send_pos;
        monkey_start_ssp();
    }
}


static void monkey_kick_usb(void)
{
    if (!(endpt->ctrl[3] & 0x800000))   // Short circuit if not active.
        return;

    while (1) {
        unsigned avail = (insert_pos - usb_send_pos) & 4095;
        // Send at most 512 bytes.  Send only if either we are idle or have a
        // full 512 bytes.
        if (avail >= 512)
            avail = 512;
        else if (!avail || usb_flight_pos != usb_send_pos)
            return;

        dTD_t * dtd = get_dtd();
        dtd->buffer_page[0] = (unsigned) usb_send_pos;
        dtd->buffer_page[1] = (unsigned) monkey_buffer; // Cyclic.

        usb_send_pos = advance(usb_send_pos, avail);
        dtd->buffer_page[4] = (unsigned) usb_send_pos;

        dtd->length_and_status = avail * 65536 + 0x8080;
        dtd->completion = monkey_in_complete;
        schedule_dtd (0x83, dtd);
    }
}


void monkey_kick(void)
{
    if (log_ssp && ssp_flight_pos == ssp_send_pos)
        monkey_start_ssp();

    monkey_kick_usb();
}


bool monkey_is_empty (void)
{
    __memory_barrier();                 // Called without interrupts disabled...
    return usb_flight_pos == usb_send_pos;
}


static void monkey_in_complete (dTD_t * dtd, unsigned status, unsigned remain)
{
    // An error that doesn't kill USB assume that we want to drop the data.
    // Also, if the buffer is full, then drop the data.
    if (status != 0x80 || headroom(usb_flight_pos) == 0)
        usb_flight_pos = (unsigned char *) dtd->buffer_page[4];

    monkey_kick_usb();
}


static void format_string (const char * s, unsigned width, unsigned char fill)
{
    for (const char * e = s; *e; ++e, --width)
        if (width == 0)
            break;
    for (; width != 0; --width)
        write_byte (fill);
    for (; *s; ++s)
        write_byte (*s);
}


static void format_number (unsigned long value, unsigned base, unsigned lower,
                           bool sgn, unsigned width, unsigned char fill)
{
    unsigned char c[23];
    unsigned char * p = c;
    if (sgn && (long) value < 0)
        value = -value;
    else
        sgn = false;

    do {
        unsigned digit = value % base;
        if (digit >= 10)
            digit += 'A' - '0' - 10 + lower;
        *p++ = digit + '0';
        value /= base;
    }
    while (value);

    if (!sgn)
        ;
    else if (fill == ' ')
        *p++ = '-';
    else {
        write_byte ('-');
        if (width > 0)
            --width;
    }

    while (width > p - c) {
        write_byte (fill);
        --width;
    }

    while (p != c)
        write_byte (*--p);
}


void printf (const char * restrict f, ...)
{
    enter_monkey();
    va_list args;
    va_start (args, f);

    for (const unsigned char * s = (const unsigned char *) f; *s; ++s) {
        if (*s != '%') {
            write_byte(*s);
            continue;
        }

        ++s;
        unsigned char fill = ' ';
        if (*s == '0')
            fill = '0';

        unsigned width = 0;
        for (; *s >= '0' && *s <= '9'; ++s)
            width = width * 10 + *s - '0';
        unsigned base = 0;
        unsigned lower = 0;
        bool sgn = false;
        unsigned lng = 0;
        for (; *s == 'l'; ++s)
            ++lng;
        switch (*s) {
        case 'x':
            lower = 0x20;
        case 'X':
            base = 16;
            break;
        case 'i':
        case 'd':
            sgn = true;
        case 'u':
            base = 10;
            break;
        case 'o':
            base = 8;
            break;
        case 'p': {
            void * value = va_arg (args, void *);
            if (width == 0)
                width = 8;
            format_number ((unsigned) value, 16, 32, false, width, '0');
            break;
        }
        case 's':
            format_string (va_arg (args, const char *), width, fill);
            break;
        }
        if (base != 0) {
            unsigned long value;
            if (lng)
                value = va_arg (args, unsigned long);
            else if (sgn)
                value = va_arg (args, int);
            else
                value = va_arg (args, unsigned);
            format_number (value, base, lower, sgn, width, fill);
        }
    }

    va_end (args);

    leave_monkey();
}


int getchar (void)
{
    // In case we've done an unget...
    int r = monkey_in_next;
    monkey_in_next = -1;
    if (r >= 0)
        return r;

    unsigned char * n = monkey_recv_pos->next;
    __memory_barrier();
    if (n == NULL) {
        __interrupt_disable();
        while (monkey_recv_pos->next == NULL)
            callback_wait();
        n = monkey_recv_pos->next;
        __interrupt_enable();
    }

    r = *n++;
    monkey_recv_pos->next = n;
    if (n != monkey_recv_pos->end)
        return r;

    __interrupt_disable();
    unsigned offset = (511 & ((long) monkey_recv_pos->end - 1)) + 1;
    schedule_buffer (3, monkey_recv_pos->end - offset, 512,
                     monkey_out_complete);
    monkey_recv_pos[0] = monkey_recv_pos[1];
    monkey_recv_pos[1].next = NULL;
    monkey_recv_pos[1].end = NULL;
    __interrupt_enable();
    return r;
}


int peekchar_nb (void)
{
    int r = monkey_in_next;
    if (r == -1 && monkey_recv_pos->next)
        r = *monkey_recv_pos->next;
    return r;
}


void ungetchar (int c)
{
    monkey_in_next = c & 255;
}


static void monkey_out_complete (dTD_t * dtd, unsigned status, unsigned remain)
{
    unsigned char * buffer = (unsigned char *) dtd->buffer_page[4];
    unsigned length = 512 - remain;

    if (length == 0 || status != 0) {
        // Reschedule immediately.
        if (endpt->ctrl[3] & 0x80)
            schedule_buffer (3, buffer, 512, monkey_out_complete);
        return;
    }

    int index = 0;
    if (monkey_recv_pos->next != NULL)
        index = 1;

    monkey_recv_pos[index].next = buffer;
    monkey_recv_pos[index].end = buffer + length;
}
