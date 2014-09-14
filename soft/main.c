// Lets try to bring up a usb device...

#include "callback.h"
#include "freq.h"
#include "jtag.h"
#include "lcd.h"
#include "monkey.h"
#include "pin.h"
#include "registers.h"
#include "sdram.h"
#include "spirom.h"
#include "square.h"
#include "switch.h"
#include "usb.h"

#include <stdbool.h>
#include <stddef.h>

// Buffer space for each packet transfer.
#define BUF_SIZE 2048

#define EDMA_COUNT 4
#define EDMA_MASK 3
static volatile EDMA_DESC_t tx_dma[EDMA_COUNT] __section ("ahb0.tx_dma");
static volatile EDMA_DESC_t rx_dma[EDMA_COUNT] __section ("ahb0.rx_dma");
static unsigned tx_dma_retire;
static unsigned tx_dma_insert;
static unsigned rx_dma_retire;
static unsigned rx_dma_insert;

// While the USB is idle, keep a linked list of the idle tx buffers.
static void * idle_tx_buffers;

extern unsigned char bss_start;
extern unsigned char bss_end;
extern unsigned char rw_data_start;
extern unsigned char rw_data_end;
extern const unsigned char rw_data_load;

enum string_descs_t {
    sd_lang,
    sd_ralph,
    sd_switch,
    sd_0001,
    sd_eth_addr,
    sd_eth_mgmt,
    sd_eth_idle,
    sd_eth_showtime,
    sd_monkey,
    sd_dfu,
};

static void endpt_tx_complete (dTD_t * dtd, unsigned status, unsigned remain);


// String0 - lang. descs.
static const unsigned short string_lang[2] = u"\x0304\x0409";
static const unsigned short string_ralph[6] = u"\x030c""Ralph";
static const unsigned short string_switch[7] = u"\x030e""Switch";
static const unsigned short string_0001[5] = u"\x030a""0001";
static const unsigned short string_eth_addr[13] = u"\x031a""42baffc46ee9";
static const unsigned short string_eth_mgmt[20] =
    u"\x0328""Ethernet Management";
static const unsigned short string_eth_idle[14] = u"\x031c""Ethernet Idle";
static const unsigned short string_eth_showtime[18] =
    u"\x0324""Ethernet Showtime";
static const unsigned short string_monkey[7] = u"\x030e""Monkey";
static const unsigned short string_dfu[4] = u"\x0308""DFU";

static const unsigned short * const string_descriptors[] = {
    string_lang,
    [sd_ralph] = string_ralph,
    [sd_switch] = string_switch,
    [sd_0001] = string_0001,
    [sd_eth_addr] = string_eth_addr,
    [sd_eth_mgmt] = string_eth_mgmt,
    [sd_eth_idle] = string_eth_idle,
    [sd_eth_showtime] = string_eth_showtime,
    [sd_monkey] = string_monkey,
    [sd_dfu] = string_dfu,
};


#define DEVICE_DESCRIPTOR_SIZE 18
static const unsigned char device_descriptor[] = {
    DEVICE_DESCRIPTOR_SIZE,
    1,                                  // type: device
    0, 2,                               // bcdUSB.
    0,                                  // class - compound.
    0,                                  // subclass.
    0,                                  // protocol.
    64,                                 // Max packet size.
    0x55, 0xf0,                         // Vendor-ID.
    'L', 'R',                           // Device-ID.
    0x34, 0x12,                         // Revision number.
    sd_ralph,                           // Manufacturer string index.
    sd_switch,                          // Product string index.
    sd_0001,                            // Serial number string index.
    1                                   // Number of configurations.
};
_Static_assert (DEVICE_DESCRIPTOR_SIZE == sizeof (device_descriptor),
                "device_descriptor size");

enum usb_interfaces_t {
    usb_intf_eth_comm,
    usb_intf_eth_data,
    usb_intf_monkey,
    usb_intf_dfu,
    usb_num_intf
};


#define CONFIG_DESCRIPTOR_SIZE (9 + 9 + 5 + 13 + 5 + 7 + 9 + 9 + 7 + 7 + 9 + 7 + 7 + 9 + 7)
static unsigned char config_descriptor[] = {
    // Config.
    9,                                  // length.
    2,                                  // type: config.
    CONFIG_DESCRIPTOR_SIZE & 0xff,      // size.
    CONFIG_DESCRIPTOR_SIZE >> 8,
    usb_num_intf,                       // num interfaces.
    1,                                  // configuration number.
    0,                                  // string descriptor index.
    0x80,                               // attributes, not self powered.
    250,                                // current (500mA).
    // Interface (comm).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_eth_comm,                  // interface number.
    0,                                  // alternate setting.
    1,                                  // number of endpoints.
    2,                                  // interface class.
    6,                                  // interface sub-class.
    0,                                  // protocol.
    sd_eth_mgmt,                        // interface string index.
    // CDC header...
    5,                                  // length
    0x24,                               // CS_INTERFACE
    0,                                  // subtype = header
    0x10, 1,                            // Version number 1.10
    // Ethernet functional descriptor.
    13,
    0x24,                               // cs_interface,
    15,                                 // ethernet
    sd_eth_addr,                        // Mac address string.
    0, 0, 0, 0,                         // Statistics bitmap.
    0, 7,                               // Max segment size.
    0, 0,                               // Multicast filters.
    0,                                  // Number of power filters.
    // Union...
    5,                                  // Length
    0x24,                               // CS_INTERFACE
    6,                                  // Union
    usb_intf_eth_comm,                  // Interface 0 is control.
    usb_intf_eth_data,                  // Interface 1 is sub-ordinate.
    // Endpoint.
    7,
    5,
    0x81,                               // IN 1
    3,                                  // Interrupt
    64, 0,                              // 64 bytes
    11,                                 // binterval
    // Interface (data).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_eth_data,                  // interface number.
    0,                                  // alternate setting.
    0,                                  // number of endpoints.
    10,                                 // interface class (data).
    6,                                  // interface sub-class.
    0,                                  // protocol.
    sd_eth_idle,                        // interface string index.
    // Interface (data).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_eth_data,                  // interface number.
    1,                                  // alternate setting.
    2,                                  // number of endpoints.
    10,                                 // interface class (data).
    6,                                  // interface sub-class.
    0,                                  // protocol.
    sd_eth_showtime,                    // interface string index.
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    2,                                  // OUT 2.
    0x2,                                // bulk
    0, 2,                               // packet size
    0,
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    0x82,                               // IN 2.
    0x2,                                // bulk
    0, 2,                               // packet size
    0,
    // Interface (monkey).
    9,                                  // length.
    4,                                  // type: interface.
    usb_intf_monkey,                    // interface number.
    0,                                  // alternate setting.
    2,                                  // number of endpoints.
    0xff,                               // interface class (vendor specific).
    'S',                                // interface sub-class.
    'S',                                // protocol.
    sd_monkey,                          // interface string index.
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    3,                                  // OUT 3.
    0x2,                                // bulk
    0, 2,                               // packet size
    0,
    // Endpoint
    7,                                  // Length.
    5,                                  // Type: endpoint.
    0x83,                               // IN 3.
    0x2,                                // bulk
    0, 2,                               // packet size
    0,

    // Interface (DFU).
    9,                                  // Length.
    4,                                  // Type = Interface.
    usb_intf_dfu,                       // Interface number.
    0,                                  // Alternate.
    0,                                  // Num. endpoints.
    0xfe, 1, 0,                         // Application specific; DFU.
    sd_dfu,                             // Name...
    // Function (DFU)
    7,                                  // Length.
    0x21,                               // Type.
    9,                                  // Will detach.  Download only.
    0, 255,                             // Detach timeout.
    0, 16,                              // Transfer size.
};
_Static_assert (CONFIG_DESCRIPTOR_SIZE == sizeof (config_descriptor),
                "config_descriptor size");


static const unsigned char network_connected[] = {
    0xa1, 0, 0, 1, 0, 0, 0, 0
};
// static const unsigned char network_disconnected[] = {
//     0xa1, 0, 0, 0, 0, 0, 0, 0
// };

static const unsigned char speed_notification100[] = {
    0xa1, 0x2a, 0, 0, 0, 1, 0, 8,
    0, 0xe1, 0xf5, 0x05, 0, 0xe1, 0xf5, 0x05,
};


#define QUALIFIER_DESCRIPTOR_SIZE 10
const unsigned char qualifier_descriptor[] = {
    QUALIFIER_DESCRIPTOR_SIZE,          // Length.
    6,                                  // Type
    0, 2,                               // usb version
    255, 1, 1,                          // class / subclass / protocol
    64, 1, 0                            // packet size / num. configs / zero
};
_Static_assert (QUALIFIER_DESCRIPTOR_SIZE == sizeof (qualifier_descriptor),
                "qualifier_descriptor size");

static unsigned char rx_ring_buffer[8192] __aligned (4096) __section ("ahb1");
static unsigned char tx_ring_buffer[8192] __aligned (4096) __section ("ahb2");

#define CCU1_VALID_0 0
#define CCU1_VALID_1 0x3f
#define CCU1_VALID_2 0x1f
#define CCU1_VALID_3 1
#define CCU1_VALID_4 (0x1c0000 + 0x3e3ff)
#define CCU1_VALID_5 0xff
#define CCU1_VALID_6 0xff
#define CCU1_VALID_7 0xf

static const unsigned ccu1_disable_mask[] = {
    0,
    CCU1_VALID_1 & ~0x1,
    CCU1_VALID_2 & ~0x1,
    CCU1_VALID_3 & ~0x0,
    // M4 BUS, GPIO, LCD, Ethernet, USB0, EMC, DMA, M4 Core, EMC DIV,
    // Flash A, Flash B,
    CCU1_VALID_4 & ~0x3837d,
    // SSP0, SCU, CREG
    CCU1_VALID_5 & ~0xc8,
    // USART3, SSP1
    CCU1_VALID_6 & ~0x24,
    // Periph... (?)
    CCU1_VALID_7 & ~0x0,
};

// USB1, SPI, VADC, APLL, USART2, UART1, USART0, SDIO.
#define CCU_BASE_DISABLE 0x013a0e00

static void disable_clocks(void)
{
    for (unsigned i = 0; i < 8; ++i) {
        volatile unsigned * base = (volatile unsigned *) (0x40051000 + i * 256);
        unsigned mask = ccu1_disable_mask[i];
        do {
            if ((mask & 1) && (*base & 1))
                *base = 0;

            mask >>= 1;
            base += 2;
        }
        while (mask);
    }

    volatile unsigned * base = (volatile unsigned *) 0x40051000;
    for (unsigned mask = CCU_BASE_DISABLE; mask; mask >>= 1, base += 64)
        if ((mask & 1) && (*base & 1))
            *base = 0;
}


static void initiate_enter_dfu (dTD_t * dtd, unsigned status, unsigned remain)
{
    callback_simple (enter_dfu);
}


static void serial_byte (unsigned byte)
{
    switch (byte) {
    case 'd':
        debug_flag = !debug_flag;
        puts (debug_flag ? "Debug on\n" : "Debug off\n");
        return;
    case 'e':
        mdio_report_all();
        return;
    case 'f':
        clock_report();
        return;
    case 'h': case '?':
        puts("<d>ebug, <f>req, <j>tag, <l>cd, <m>emtest, <p>retty, <s>pi\n"
             "<r>eset, <R>eboot, df<u>, <v>erbose\n");
        return;
    case 'j':
        jtag_cmd();
        return;
    case 'l':
        lcd_init();
        return;
    case 'm':
        memtest();
        return;
    case 'p':
        square_interact();
        return;
    case 's':
        spirom_command();
        return;
    case 'r':
        puts ("Reset!\n");
        *BASE_M4_CLK = 0x0e000800;      // Back to IDIVC.
        for (int i = 0; i != 100000; ++i)
            asm volatile ("");
        while (1)
            *CORTEX_M_AIRCR = 0x05fa0004;
    case 'R':
        puts ("Cold Reboot!\n");
        *BASE_M4_CLK = 0x0e000800;      // Back to IDIVC.
        for (int i = 0; i != 100000; ++i)
            asm volatile ("");
        __interrupt_disable();
        while (1) {
            RESET_CTRL[1] = 0xffffffff;
            RESET_CTRL[0] = 0xffffffff;
        }
    case 'u':
        enter_dfu();
        return;
    case 'v':
        verbose_flag = !verbose_flag;
        puts (verbose_flag ? "Verbose ON\n" : "Verbose OFF\n");
        return;
    }

    // Don't echo escape...
    if (byte == 27)
        return;

    if (byte == '\r')
        putchar ('\n');
    else
        putchar (byte);
}


static void start_mgmt (void)
{
    if (endpt->ctrl[1] & 0x800000)
        return;                         // Already running.

    puts ("Starting mgmt...\n");

    qh_init (0x81, 0x20400000);
    // No 0-size-frame on the monkey.
    qh_init (0x03, 0x20000000);
    qh_init (0x83, 0x20000000);

    // FIXME - default mgmt packets?
    endpt->ctrl[1] = 0x00cc0000;
    endpt->ctrl[3] = 0x00c800c8;

    init_monkey_usb();
}


static void reuse_tx_buffer(void * buffer)
{
    if (endpt->ctrl[2] & 0x80)
        schedule_buffer (0x02, buffer, BUF_SIZE, endpt_tx_complete);
    else {
        * (void **) buffer = idle_tx_buffers;
        idle_tx_buffers = buffer;
    }
}


// Completion called for buffers from the OUT endpoint.  Dispatch to the
// ethernet.
static void endpt_tx_complete (dTD_t * dtd, unsigned status, unsigned remain)
{
    void * buffer = (void *) dtd->buffer_page[4];
    if (status) {                       // Errored...
        reuse_tx_buffer(buffer);
        return;
    }

    volatile EDMA_DESC_t * t = &tx_dma[tx_dma_insert++ & EDMA_MASK];

    t->count = BUF_SIZE - remain;
    t->buffer1 = buffer;
    t->buffer2 = NULL;

    if (tx_dma_insert & EDMA_MASK)
        t->status = 0xf0000000;
    else
        t->status = 0xf0200000;

    EDMA->trans_poll_demand = 0;

    debugf ("TX to MAC..: %08x %08x\n", dtd->buffer_page[0], status);
}


static void notify_network_up (dTD_t * dtd, unsigned status, unsigned remain)
{
    if (endpt->ctrl[1] & 0x800000) {
        schedule_buffer (
            0x81, (void*) network_connected, sizeof network_connected, NULL);
        schedule_buffer (0x81, (void*) speed_notification100,
                         sizeof speed_notification100, NULL);
    }
}


static void start_network (void)
{
    if (endpt->ctrl[2] & 0x80)
        return;                         // Already running.

    puts ("Starting network...\n");

    qh_init (0x02, 0);
    qh_init (0x82, 0);

    endpt->ctrl[2] = 0x00c800c8;

    while (idle_tx_buffers) {
        void * buffer = idle_tx_buffers;
        idle_tx_buffers = * (void **) buffer;
        schedule_buffer (2, buffer, BUF_SIZE, endpt_tx_complete);
    }
}


static void stop_network (void)
{
    if (!(endpt->ctrl[2] & 0x80))
        return;                         // Already stopped.

    puts ("Stopping network.\n");

    do {
        endpt->flush = 0x40004;
        while (endpt->flush & 0x40004);
    }
    while (endpt->stat & 0x40004);
    endpt->ctrl[2] = 0;
    // Cleanup any dtds.
    endpt_clear(0x02);
    endpt_clear(0x82);
}


static void stop_mgmt (void)
{
    stop_network();

    if (!(endpt->ctrl[1] & 0x800000))
        return;                         // Already stopped.

    endpt->ctrl[1] = 0;
    endpt->ctrl[3] = 0;
    do {
        endpt->flush = 0xa0008;
        while (endpt->flush & 0xa0008);
    }
    while (endpt->stat & 0xa0008);
    // Cleanup any dtds.
    endpt_clear(0x81);
    endpt_clear(3);
    endpt_clear(0x83);

    puts("Stopped mgmt.\n");
}


static void munge_usb_config(unsigned char * config,
                             unsigned length,
                             unsigned type,
                             unsigned bulk_size)
{
    config[1] = type;
    for (int i = 0; i < length; i += config[i])
        if (config[i + 1] == 5          // Endpoint.
            && config[i + 3] ==  2) {
            config[i + 4] = bulk_size;
            config[i + 5] = bulk_size >> 8;
        }
}


static void process_setup (void)
{
    unsigned setup1;
    unsigned setup0 = get_0_setup(&setup1);

    const void * response_data = NULL;
    int response_length = -1;
    dtd_completion_t * callback = NULL;

    switch (setup0 & 0xffff) {
    case 0x0021:                        // DFU detach.
        response_length = 0;
        callback = initiate_enter_dfu;
        break;
    case 0x0080:                        // Get status.
        response_data = "\0";
        response_length = 2;
        break;
    case 0x03a1: {                      // DFU status.
        static const unsigned char status[] = { 0, 100, 0, 0, 0, 0 };
        response_data = status;
        response_length = 6;
        break;
    }
    // case 0x0100:                        // Clear feature device.
    //     break;
    // case 0x0101:                        // Clear feature interface.
    //     break;
    // case 0x0102:                        // Clear feature endpoint.
    //     break;
    // case 0x0880:                        // Get configuration.
    //     break;
    case 0x0680:                        // Get descriptor.
        switch (setup0 >> 24) {
        case 1:                         // Device.
            response_data = device_descriptor;
            response_length = DEVICE_DESCRIPTOR_SIZE;
            break;
        case 2:                         // Configuration.
            munge_usb_config(config_descriptor,
                             CONFIG_DESCRIPTOR_SIZE,
                             2,         // type = config,
                             is_high_speed() ? 512 : 64);
            response_data = config_descriptor;
            response_length = CONFIG_DESCRIPTOR_SIZE;
            break;
        case 6:                         // Device qualifier.
            response_data = qualifier_descriptor;
            response_length = QUALIFIER_DESCRIPTOR_SIZE;
            break;
        case 3: {                        // String.
            unsigned index = (setup0 >> 16) & 255;
            if (index < sizeof string_descriptors / 4) {
                response_data = string_descriptors[index];
                response_length = *string_descriptors[index] & 0xff;
            }
            break;
        }
        case 7:                         // Other speed config.
        case 10:                        // Debug.
        default:
            ;
        }
        break;
    // case 0x0a81:                        // Get interface.
    //     break;
    case 0x0500:                        // Set address.
        if (((setup0 >> 16) & 127) == 0)
            stop_mgmt();                // Stop everything if back to address 0.
        *DEVICEADDR = ((setup0 >> 16) << 25) | (1 << 24);
        response_length = 0;
        break;
    case 0x0900:                        // Set configuration.
        // This leaves us in the default alternative, so always stop the
        // network.
        stop_network();
        start_mgmt();
        response_length = 0;
        callback = notify_network_up;
        break;
    // case 0x0700:                        // Set descriptor.
    //     break;
    // case 0x0300:                        // Set feature device.
    //     break;
    // case 0x0301:                        // Set feature interface.
    //     break;
    // case 0x0302:                        // Set feature endpoint.
    //     break;
    case 0x0b01:                        // Set interface.
        switch (setup1 & 0xffff) {
        case 1:                         // Interface 1 (data)
            if ((setup1 & 0xffff) == usb_intf_eth_data) {
                if (setup0 & 0xffff0000) {
                    start_network();
                    callback = notify_network_up;
                }
                else
                    stop_network();
            }
            response_length = 0;
            break;
        case usb_intf_eth_comm:         // Interface 0 (comms) - ignore.
        case usb_intf_dfu:                         // Interface 2 (DFU).
            response_length = 0;
            break;
        }
        break;

    // case 0x820c:                        // Synch frame.
    //     break;
    // case 0x4021:                        // Set ethernet multicast.
    //     break;
    // case 0x4121:                        // Set eth. power mgmt filter.
    //     break;
    // case 0x4221:                        // Get eth. power mgmt filter.
    //     break;
    case 0x4321:                        // Set eth. packet filter.
        // Just fake it for now...
        response_length = 0;
        break;
    // case 0x4421:                        // Get eth. statistic.
    //     break;
    default:
        break;
    }

    if (response_length >= 0) {
        respond_to_setup (setup1, response_data, response_length, callback);
        debugf ("Setup %s: %08x %08x\n", "OK", setup0, setup1);
    }
    else {
        endpt->ctrl[0] = 0x810081;      // Stall....
        printf ("Setup %s: %08x %08x\n", "STALL", setup0, setup1);
    }
}


static void queue_rx_dma(void * buffer)
{
    volatile EDMA_DESC_t * r = &rx_dma[rx_dma_insert++ & EDMA_MASK];

    if (rx_dma_insert & EDMA_MASK)
        r->count = BUF_SIZE;
    else
        r->count = 0x8000 + BUF_SIZE;
    r->buffer1 = buffer;
    r->buffer2 = 0;

    r->status = 0x80000000;

    EDMA->rec_poll_demand = 0;
}


static void endpt_rx_complete (dTD_t * dtd, unsigned status, unsigned remain)
{
    // Re-queue the buffer for network data.
    unsigned buffer = dtd->buffer_page[4];

    queue_rx_dma((void *) buffer);

    debugf("RX complete: %08x %08x\n",
           dtd->buffer_page[0], dtd->length_and_status);
}


static void retire_rx_dma (volatile EDMA_DESC_t * rx)
{
    // FIXME - handle errors.
    // FIXME - handle overlength packets.
    if (!(endpt->ctrl[2] & 0x80)) {     // If usb not running, put back to eth.
        queue_rx_dma(rx->buffer1);
        return;
    }

    // Give the buffer to USB.
    unsigned status = rx->status;
    void * buffer = rx->buffer1;
    schedule_buffer (0x82, buffer, (status >> 16) & 0x7ff,
                     endpt_rx_complete);
    debugf ("RX to usb..: %p %08x\n", buffer, status);
}


// Handle a completed TX entry from ethernet.
static void retire_tx_dma (volatile EDMA_DESC_t * tx)
{
    // FIXME - handle errors.
    // Give the buffer to USB...
    void * buffer = tx->buffer1;
    reuse_tx_buffer(tx->buffer1);

    debugf ("TX Complete: %p %08x\n", buffer, tx->status);
}


static void init_ethernet (void)
{
    // Pins are set-up in init-switch.

    // Set the PHY clocks.
    *BASE_PHY_TX_CLK = 0x03000800;
    *BASE_PHY_RX_CLK = 0x03000800;

    *CREG6 = 4;                         // Set ethernet to RMII.

    RESET_CTRL[0] = 1 << 22;            // Reset ethernet.
    while (!(RESET_ACTIVE_STATUS[0] & (1 << 22)));

    EDMA->bus_mode = 1;                 // Reset ethernet DMA.
    while (EDMA->bus_mode & 1);

    MAC->addr0_low = 0xc4ffba42;
    MAC->addr0_high = 0x8000e96e;

    // Set filtering options.  Promiscuous / recv all.
    MAC->frame_filter = 0x80000001;

    MAC->config = 0xc900;

    // Set-up the dma descs.
    for (int i = 0; i != EDMA_COUNT; ++i) {
        tx_dma[i].status = 0;

        rx_dma[i].status = 0x80000000;
        rx_dma[i].count = BUF_SIZE;     // Status bits?
        rx_dma[i].buffer1 = rx_ring_buffer + BUF_SIZE * i;
        rx_dma[i].buffer2 = 0;
    }

    rx_dma[EDMA_MASK].count = 0x8000 + BUF_SIZE; // End of ring.

    rx_dma_insert = EDMA_COUNT;

    EDMA->trans_des_addr = (unsigned) tx_dma;
    EDMA->rec_des_addr = (unsigned) rx_dma;

    // Start ethernet & it's dma.
    MAC->config = 0xc90c;
    EDMA->op_mode = 0x2002;
}


static void usb_interrupt (void)
{
    unsigned status = *USBSTS;
    *USBSTS = status;                   // Clear interrupts.

    unsigned complete = endpt->complete;
    endpt->complete = complete;

    // Don't log interrupts that look like they're monkey completions.
    if (debug_flag && (complete != 0x80000))
        puts ("usb interrupt...\n");

    static const unsigned char endpoints[] = {
        2, 0x82, 0x81, 0x80, 0, 3, 0x83 };
    for (int i = 0; i < sizeof endpoints; ++i)
        if (complete & ep_mask(endpoints[i]))
            endpt_complete(endpoints[i]);

    // Check for setup on 0.  FIXME - will other set-ups interrupt?
    unsigned setup = endpt->setupstat;
    endpt->setupstat = setup;
    if (setup & 1)
        process_setup();

    if (!(status & 0x40))
        return;

    // Handle a bus reset.
    while (endpt->prime);
    endpt->flush = 0xffffffff;
    while (endpt->flush);

    // Clean out any dtds.
    for (int i = 0; i < sizeof endpoints; ++i)
        endpt_clear(endpoints[i]);

    *ENDPTNAK = 0xffffffff;
    *ENDPTNAKEN = 1;

    *DEVICEADDR = 0;

    puts ("USB Reset processed...\n");
}


static void eth_interrupt (void)
{
    debugf ("eth interrupt...\n");
    EDMA->stat = 0x1ffff;               // Clear interrupts.

    while (rx_dma_retire != rx_dma_insert
           && !(rx_dma[rx_dma_retire & EDMA_MASK].status & 0x80000000))
        retire_rx_dma (&rx_dma[rx_dma_retire++ & EDMA_MASK]);

    while (tx_dma_retire != tx_dma_insert
           && !(tx_dma[tx_dma_retire & EDMA_MASK].status & 0x80000000))
        retire_tx_dma (&tx_dma[tx_dma_retire++ & EDMA_MASK]);
}


static void switch_interrupt (void)
{
    mdio_report_changed();
    EVENT_ROUTER->clr_stat = 1;
}


void main (void)
{
    // Disable all interrupts for now...
    __interrupt_disable();

    // Soft reset doesn't restore clocking.  So do it ourselves.
    *BASE_M4_CLK = 0x01000800;          // Switch to irc for a bit.

    // Restore PLL1 & IDIVC to 96MHz off IRC, just in case of soft reset.
    *PLL1_CTRL = 0x01170940;
    *IDIVC_CTRL = 0x09000808;
    while (!(*PLL1_STAT & 1));

    *BASE_M4_CLK = 0x0e000800;          // Switch back to 96MHz IDIVC.

    NVIC_ICER[0] = 0xffffffff;
    NVIC_ICER[1] = 0xffffffff;

    check_for_early_dfu();

    __memory_barrier();

    for (unsigned char * p = &bss_start; p != &bss_end; ++p)
        *p = 0;

    const unsigned char * q = &rw_data_load;
    for (unsigned char * p = &rw_data_start; p != &rw_data_end; ++p)
        *p = *q++;

    __memory_barrier();

    init_monkey_ssp();

    puts ("***********************************\n");
    puts ("**          Supa Switch          **\n");
    puts ("***********************************\n");

    init_switch();
    init_ethernet();

    // 50 MHz in from eth_tx_clk
    // Configure the clock to USB.
    // Generate 480MHz off 50MHz...
    // ndec=5, mdec=32682, pdec=0
    // selr=0, seli=28, selp=13
    // PLL0USB - mdiv = 0x06167ffa, np_div = 0x00302062
    *PLL0USB_CTRL = 0x03000819;
    *PLL0USB_MDIV = (28 << 22) + (13 << 17) + 32682;
    *PLL0USB_NP_DIV = 5 << 12;
    *PLL0USB_CTRL = 0x03000818;         // Divided in, direct out.

    // Wait for locks.
    while (!(*PLL0USB_STAT & 1));

    // Set the flash access time for 160MHz.
    *FLASHCFGA = 0x8000703a;
    *FLASHCFGB = 0x8000703a;

    // Now ramp to 160MHz.
    *IDIVA_CTRL = 0x07000808;
    *BASE_M4_CLK = 0x0c000800;

    disable_clocks();

    *BASE_SSP1_CLK = 0x0c000800;        // Take monkey SSP to 80Mb/s.

    // Build the linked list of idle tx buffers.
    idle_tx_buffers = NULL;
    for (int i = 0; i != 4; ++i) {
        void * buffer = tx_ring_buffer + BUF_SIZE * i;
        * (void **) buffer = idle_tx_buffers;
        idle_tx_buffers = buffer;
    }

    usb_init();

    // Enable the ethernet, usb and dma interrupts.
    NVIC_ISER[0] = 0x00000124;
    NVIC_ISER[1] = 1 << 10;             // Enable event router interrupt.
    *USBINTR = 0x00000041;              // Port change, reset, data.
    EDMA->stat = 0x1ffff;
    EDMA->int_en = 0x0001ffff;

    // Reset event-router WAKEUP0 & enable it (interrupt from switch).
    EVENT_ROUTER->clr_stat = 1;
    EVENT_ROUTER->set_en = 1;

    __interrupt_enable();

    lcd_init();

    square_draw9();

    while (true)
        serial_byte (getchar());
}


void * start[64] __attribute__ ((section (".start"), externally_visible));
void * start[64] = {
    [0] = (void*) 0x10089fe0,
    [1] = main,

    [18] = gpdma_interrupt,
    [21] = eth_interrupt,
    [23] = lcd_interrupt,
    [24] = usb_interrupt,
    [58] = switch_interrupt,
};
