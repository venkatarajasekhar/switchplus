
#include "monkey.h"
#include "registers.h"
#include "usbdriver.h"

static struct {
    unsigned char * insert;
    unsigned char * limit;
    unsigned char * send;
    bool outstanding;
} monkey_pos;

static unsigned char monkey_buffer[4096] __attribute__ ((aligned (4096)));

static void monkey_in_complete (int ep, dQH_t * qh, dTD_t * dtd);

bool log_serial;

void init_monkey (void)
{
    if (log_monkey) {
        monkey_pos.send = monkey_buffer;
        monkey_pos.insert = monkey_buffer;
        monkey_pos.limit = monkey_buffer + sizeof monkey_buffer;
    }
}

void ser_w_byte (unsigned byte)
{
    if (log_serial) {
        while (!(*USART3_LSR & 32));    // Wait for THR to be empty.
        *USART3_THR = byte;
    }
    if (!log_monkey)
        return;

    if (monkey_pos.insert != monkey_pos.limit) {
        *monkey_pos.insert++ = byte;
        return;
    }

    // If we're not at the end of buffer, that means we're full & have to drop.
    // If we haven't sent anything, that also means we're full.
    if (monkey_pos.limit != monkey_buffer + sizeof monkey_buffer
        || monkey_pos.send == monkey_buffer)
        return;

    // Go back to the beginning.
    monkey_pos.insert = monkey_buffer;
    monkey_pos.limit = monkey_pos.send;

    if (monkey_pos.insert != monkey_pos.limit)
        *monkey_pos.insert++ = byte;
}


void ser_w_string (const char * s)
{
    for (; *s; s++)
        ser_w_byte (*s);
    if (log_monkey)
        monkey_kick();
}


void ser_w_hex (unsigned value, int nibbles, const char * term)
{
    for (int i = nibbles; i != 0; ) {
        --i;
        ser_w_byte ("0123456789abcdef"[(value >> (i * 4)) & 15]);
    }
    ser_w_string (term);
}


void monkey_kick (void)
{
    // If the dTD is in-flight, or there is no data, or the monkey end-point is
    // not in use, then nothing to do.
    if (!log_monkey || monkey_pos.outstanding
        || monkey_pos.send == monkey_pos.insert || !(*ENDPTCTRL3 & 0x800000))
        return;

    // FIXME - we should do something to get stuff out on out-of-dtds.
    dTD_t * dtd = get_dtd();
    if (!dtd)
        return;                         // FIXME - we need better.

    dtd->buffer_page[0] = (unsigned) monkey_pos.send;
    dtd->buffer_page[1] = (unsigned) monkey_buffer; // Cyclic.
    int length = monkey_pos.insert - monkey_pos.send;
    if (length < 0)
        length += 4096;
    dtd->length_and_status = length * 65536 + 0x8080;
    dtd->completion = monkey_in_complete;
    schedule_dtd (0x83, dtd);
    monkey_pos.outstanding = true;
}


static void monkey_in_complete (int ep, dQH_t * qh, dTD_t * dtd)
{
    // FIXME - distinguish between errors and sending a full page?
    monkey_pos.outstanding = false;
    monkey_pos.send = (unsigned char *) dtd->buffer_page[0];
    if (monkey_pos.insert == monkey_buffer + sizeof monkey_buffer)
        monkey_pos.insert = monkey_buffer;
    if (monkey_pos.send == monkey_pos.insert) {
        // We're idle.  Reset the pointers.
        monkey_pos.send = monkey_buffer;
        monkey_pos.insert = monkey_buffer;
        monkey_pos.limit = monkey_buffer + sizeof monkey_buffer;
    }
    else {
        monkey_kick();
    }
}
