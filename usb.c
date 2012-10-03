// Lets try bring ub a usb device...

#include "registers.h"

#include <stddef.h>

#define JOIN2(a,b) a##b
#define JOIN(a,b) JOIN2(a,b)
#define STATIC_ASSERT(b) int JOIN(_sa_dummy_, __LINE__)[b ? 1 : -1]

// struct dTD_s has 4-byte alignment; dTD_t has 32-byte alignment.
typedef struct dTD_s __attribute__ ((aligned (32))) dTD_t;

struct dTD_s {
    dTD_t * volatile next;
    volatile unsigned length_and_status;
    unsigned volatile buffer_page[5];
};

typedef struct dQH_t {
    // 48 byte queue head.
    volatile unsigned capabilities;
    dTD_t * volatile current;
    struct dTD_s dTD;
    volatile unsigned reserved;
    volatile unsigned setup0;
    volatile unsigned setup1;
    // 16 bytes remaining for our use...
    dTD_t * first;
    dTD_t * last;
    unsigned dummy2;
    unsigned dummy3;
} __attribute__ ((aligned (64))) dQH_t;

// OUT is host to device.
// IN is device to host.
typedef struct qh_pair_t {
    dQH_t OUT;
    dQH_t IN;
} qh_pair_t;

static qh_pair_t QH[6] __attribute__ ((aligned (2048)));

#define NUM_DTDS 52
static struct {
    dTD_t dtd __attribute__ ((aligned (32)));
    unsigned pad;
} DTD[NUM_DTDS];

static dTD_t * dtd_free_list;

#define DEVICE_DESCRIPTOR_SIZE 18
static const unsigned char device_descriptor[] = {
    DEVICE_DESCRIPTOR_SIZE,
    1,                                  // type:
    0, 2,                               // bcdUSB.
    255,                                // class - vendor specific.
    1,                                  // subclass.
    1,                                  // protocol.
    64,                                 // Max packet size.
    0x55, 0xf0,                         // Vendor-ID.
    'L', 'R',                           // Device-ID.
    0x34, 0x12,                         // Revision number.
    0,                                  // Manufacturer string index.
    0,                                  // Product string index.
    0,                                  // Serial number string index.
    1                                   // Number of configurations.
    };
STATIC_ASSERT (DEVICE_DESCRIPTOR_SIZE == sizeof (device_descriptor));


#define CONFIG_DESCRIPTOR_SIZE 32
static const unsigned char config_descriptor[] = {
    // Config.
    9,                                  // length.
    2,                                  // type: config.
    CONFIG_DESCRIPTOR_SIZE & 0xff,      // size.
    CONFIG_DESCRIPTOR_SIZE >> 8,
    1,                                  // num interfaces.
    1,                                  // configuration number.
    0,                                  // string descriptor index.
    0x80,                               // attributes, not self powered.
    250,                                // current (500mA).
    // Interface.
    9,                                  // length.
    4,                                  // type: interface.
    0,                                  // interface number.
    0,                                  // alternate setting.
    2,                                  // number of endpoints.
    255,                                // interface class.
    0,                                  // interface sub-class.
    0,                                  // protocol.
    0,                                  // interface string index.
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    1,                                  // OUT 1.
    0x2,                                // bulk
    0, 2,                               // packet size
    0,
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    0x81,                               // IN 1.
    0x2,                                // bulk
    0, 2,                               // packet size
    0
};
STATIC_ASSERT (CONFIG_DESCRIPTOR_SIZE == sizeof (config_descriptor));

#if 0
#define QUALIFIER_DESCRIPTOR_SIZE 10
const unsigned char qualifier_descriptor[] = {
    10,                                 // Length.
    6,                                  // Type
    0, 2,                               // usb version
    0x255, 1, 1,
    64, 0, 0
}
#endif


static void ser_w_byte (unsigned byte)
{
#if 1
    while (!(*USART3_LSR & 32));         // Wait for THR to be empty.
    *USART3_THR = byte;
#endif
}

static void ser_w_string (const char * s)
{
    for (; *s; s++)
        ser_w_byte (*s);
}

static void ser_w_hex (unsigned value, int nibbles, const char * term)
{
    for (int i = nibbles; i != 0; ) {
        --i;
        ser_w_byte("0123456789abcdef"[(value >> (i * 4)) & 15]);
    }
    ser_w_string (term);
}


dTD_t * get_dtd (void)
{
    dTD_t * r = dtd_free_list;
    if (r != NULL)
        dtd_free_list = r->next;
    return r;
}


void put_dtd (dTD_t * dtd)
{
    dtd->next = dtd_free_list;
    dtd_free_list = dtd;
}


void schedule_dtd (int ep, dTD_t * dtd)
{
    dQH_t * qh;
    if (ep >= 0x80) {                   // IN.
        qh = &QH[ep - 0x80].IN;
        ep = 0x10000 << (ep - 0x80);
    }
    else {                              // OUT.
        qh = &QH[ep].OUT;
        ep = 1 << ep;
    }

    dtd->next = (dTD_t *) 1;
    if (qh->last != NULL) {
        // 1. Add dTD to end of the linked list.
        qh->last->next = dtd;
        qh->last = dtd;

        // 2. Read correct prime bit in ENDPTPRIME - if '1' DONE.
        if (*ENDPTPRIME & ep)
            return;

        unsigned eps;
        do {
            // 3. Set ATDTW bit in USBCMD register to '1'.
            *USBCMD |= 1 << 14;

            // 4. Read correct status bit in ENDPTSTAT. (Store in temp variable
            // for later).
            eps = *ENDPTSTAT;

            // 5. Read ATDTW bit in USBCMD register.
            // - If '0' go to step 3.
            // - If '1' continue to step 6.
        }
        while (!(*USBCMD & (1 << 14)));

        // 6. Write ATDTW bit in USBCMD register to '0'.
        // Seems unnecessary...
        //*USBCMD &= ~(1 << 14);

        // 7. If status bit read in step 4 (ENDPSTAT reg) indicates endpoint
        // priming is DONE (corresponding ERBRx or ETBRx is one): DONE.
        if (eps & ep)
            return;

        // 8. If status bit read in step 4 is 0 then go to Linked list is empty:
        // Step 1.
    }

    if (qh->first == NULL) {
        qh->first = dtd;
        qh->last = dtd;
    }
    else
        qh->last->next = dtd;

    // 1. Write dQH next pointer AND dQH terminate bit to 0 as a single
    // DWord operation.
    qh->dTD.next = dtd;

    // 2. Clear active and halt bits in dQH (in case set from a previous
    // error).
    qh->dTD.length_and_status &= ~0xc0;

    // 3. Prime endpoint by writing '1' to correct bit position in
    // ENDPTPRIME.
    *ENDPTPRIME = ep;
    while (*ENDPTPRIME & ep);
    if (!(*ENDPTSTAT & ep))
        ser_w_string ("Oops, EPST\r\n");
}


void respond_to_setup (unsigned ep, unsigned setup1,
                       const void * descriptor, unsigned length)
{
    if ((setup1 >> 16) < length)
        length = setup1 >> 16;

    // The DMA won't take this into account...
    /* if (descriptor && (unsigned) descriptor < 0x10000000) */
    /*     descriptor += * M4MEMMAP; */

    dTD_t * dtd = get_dtd();
    if (dtd == NULL)
        return;                         // Bugger.

    // Set terminate & active bits.
    dtd->length_and_status = (length << 16) + 0x8080;
    dtd->buffer_page[0] = (unsigned) descriptor;
    dtd->buffer_page[1] = (0xfffff000 & (unsigned) descriptor) + 4096;

    schedule_dtd (ep + 0x80, dtd);

    if (*ENDPTSETUPSTAT & (1 << ep))
        ser_w_string ("Oops, EPSS\r\n");

    if (length == 0)
        return;                         // No data so no ack...

    // Now the status dtd...
    dtd = get_dtd();
    if (dtd == NULL)
        return;                         // Bugger.

    dtd->length_and_status = 0x8080;
    //static unsigned dummy0;
    dtd->buffer_page[0] = 0;
    //dtd->buffer_page[0] = (unsigned) &dummy0;
    schedule_dtd (ep, dtd);

    if (*ENDPTSETUPSTAT & (1 << ep))
        ser_w_string ("Oops, EPSS\r\n");

    /* for (int i = 0 ; i != length; ++i) */
    /*     ser_w_hex (((unsigned char *) descriptor)[i], 2, " "); */
    /* ser_w_string ("\r\n"); */
}


// FIXME - we probably only want EP 0.
static void process_setup (int i)
{
    static unsigned zero = 0;
    qh_pair_t * qh = &QH[i];

    *ENDPTCOMPLETE = 0x10001 << i;
    unsigned setup0;
    unsigned setup1;
    do {
        *USBCMD |= 1 << 13;             // Set tripwire.
        setup0 = qh->OUT.setup0;
        setup1 = qh->OUT.setup1;
    }
    while (!(*USBCMD & (1 << 13)));
    *USBCMD &= ~(1 << 13);
    while (*ENDPTSETUPSTAT & (1 << i))
        *ENDPTSETUPSTAT = 1 << i;

    // FIXME - flush old setups.

    switch (setup0 & 0xffff) {
    case 0x0080:                        // Get status.
        respond_to_setup (i, setup1, &zero, 2);
        break;
    case 0x0100:                        // Clear feature device.
        break;
    case 0x0101:                        // Clear feature interface.
        break;
    case 0x0102:                        // Clear feature endpoint.
        break;
    case 0x0880:                        // Get configuration.
        break;
    case 0x0680:                        // Get descriptor.
        switch (setup0 >> 24) {
        case 1:                         // Device.
            respond_to_setup (i, setup1, device_descriptor,
                              DEVICE_DESCRIPTOR_SIZE);
            break;
        case 2:                         // Configuration.
            respond_to_setup (i, setup1, config_descriptor,
                              CONFIG_DESCRIPTOR_SIZE);
            break;
        case 3:                         // String.
        case 6:                         // Device qualifier.
        case 7:                         // Other speed config.
        default:
            *ENDPTCTRL0 = 0x810081;     // Stall....
            break;
        }
        break;
    case 0x0a81:                        // Get interface.
        break;
    case 0x0500:                        // Set address.
        // FIXME - now or later?
        *DEVICEADDR = ((setup0 >> 16) << 25) | (1 << 24);
        respond_to_setup (i, setup1, NULL, 0);
        break;
    case 0x0900:                        // Set configuration.
        respond_to_setup (i, setup1, NULL, 0);
        break;
    case 0x0700:                        // Set descriptor.
        break;
    case 0x0300:                        // Set feature device.
        break;
    case 0x0301:                        // Set feature interface.
        break;
    case 0x0302:                        // Set feature endpoint.
        break;
    case 0x0b01:                        // Set interface.
        break;
    case 0x0c82:                        // Synch frame.
        break;
    }

    ser_w_hex (setup0, 8, " setup0\r\n");
    ser_w_hex (setup1, 8, " setup1\r\n\r\n");
}


static dTD_t * retire_dtd (dTD_t * d, dQH_t * qh)
{
    dTD_t * next = d->next;
    put_dtd (d);
    if (next == NULL || next == (dTD_t*) 1) {
        next = NULL;
        qh->last = NULL;
    }

    qh->first = next;
    return next;
}


static void endpt_in_complete (int ep, dQH_t * qh)
{
    // Clean-up the DTDs...
    if (qh->first == NULL)
        return;

    // Just clear any success...
    dTD_t * d = qh->first;
    while (!(d->length_and_status & 0x80)) {
        ser_w_hex (d->length_and_status, 8, " ok length and status\r\n");
        d = retire_dtd (d, qh);
        if (d == NULL)
            return;
    }

    if (!(d->length_and_status & 0x7f))
        return;                         // Still going.

    // FIXME - what do we actually want to do on errors?
    ser_w_hex (d->length_and_status, 8, " ERROR length and status\r\n");
    if (retire_dtd (d, qh))
        *ENDPTPRIME = ep;               // Reprime the endpoint.
}


static void endpt_out_complete (int ep, dQH_t * qh)
{
    // For now...
    endpt_in_complete (ep, qh);
}

void doit (void)
{
//    RESET_CTRL[0] = 1 << 17;
#if 0
    // Configure the clock to USB.
    // Generate 480MHz off IRC...
    // PLL0USB - mdiv = 0x06167ffa, np_div = 0x00302062
    * (v32*) 0x40050020 = 0x01000818;   // Control.
    * (v32*) 0x40050024 = 0x06167ffa;   // mdiv
    * (v32*) 0x40050028 = 0x00302062;   // np_div.
#endif

    for (int i = 0; i != 100000; ++i)
        asm volatile ("");              // FIXME - wait for lock properly.

    dtd_free_list = NULL;
    for (int i = 0; i != NUM_DTDS; ++i) {
        ser_w_hex ((unsigned) &DTD[i].dtd, 8, " is a DTD\r\n");
        put_dtd (&DTD[i].dtd);
    }

    for (int i = 0; i != 6; ++i) {
        ser_w_hex ((unsigned) &QH[i].OUT, 8, " OUT\r\n");
        ser_w_hex ((unsigned) &QH[i].IN, 8, " IN\r\n\r\n");
        QH[i].OUT.first = NULL;
        QH[i].OUT.last = NULL;
        QH[i].IN.first = NULL;
        QH[i].IN.last = NULL;
    }

    *USBCMD = 2;                        // Reset.
    while (*USBCMD & 2);

    *USBINTR = 0;
    *USBMODE = 0xa;                     // Device.  Tripwire.
    *OTGSC = 9;
    *PORTSC1 = 0x01000000;              // Only full speed for now.

    QH[0].OUT.capabilities = 0x20408000;
    QH[0].OUT.current = (void *) 1;

    QH[0].IN.capabilities = 0x20408000;
    QH[0].IN.current = (void *) 1;

    // Set the endpoint list pointer.
    *ENDPOINTLISTADDR = (unsigned) &QH;
    *DEVICEADDR = 0;

    *USBCMD = 1;                        // Run.

    while (1) {
        if (*USBSTS & 0x40) {
            *ENDPTNAK = 0xffffffff;
            *ENDPTNAKEN = 1;
            *USBSTS = 0xffffffff;
            *ENDPTSETUPSTAT = *ENDPTSETUPSTAT;
            while (*ENDPTPRIME);
            *ENDPTFLUSH = 0xffffffff;
            if (!(*PORTSC1 & 0x100))
                ser_w_string ("Bugger\r\n");
            *ENDPTCTRL0 = 0x00c000c0; // Bit 6 and 22 are undoc...
            *DEVICEADDR = 0;
            //while (*USBSTS & 0x40);
            ser_w_string ("Reset processed...\r\n");
        }

        unsigned setupstat = *ENDPTSETUPSTAT;
        *ENDPTSETUPSTAT = setupstat;

        // Only on control endpoints...
        for (int i = 0; i < 1; ++i)
            if (setupstat & (1 << i))
                process_setup (i);

        unsigned complete = *ENDPTCOMPLETE;
        *ENDPTCOMPLETE = complete;

        for (int i = 0; i < 6; ++i) {
            if (complete & (1 << i))
                endpt_out_complete(1 << i, &QH[i].OUT);
            if (complete & (0x10000 << i))
                endpt_in_complete(0x10000 << i, &QH[i].IN);
        }
    }
}

void * start[64] __attribute__ ((section (".start")));
void * start[64] = {
    (void*) 0x10089ff0,
    doit
};