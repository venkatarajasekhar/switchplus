#include "monkey.h"
#include "registers.h"
#include "spirom.h"

#define JOIN2(a,b) a##b
#define JOIN(a,b) JOIN2(a,b)
#define STATIC_ASSERT(b) extern int JOIN(_sa_dummy_, __LINE__)[b ? 1 : -1]
#define CONSOLE_CS (&GPIO_BYTE[7][19])
#define ROM_CS (&GPIO_BYTE[3][8])
#define CLR "\r\e[K"

#define PAGE_LEN 264

static bool buffer_select;

static void spi_start (volatile ssp_t * ssp, volatile unsigned char * cs,
                       unsigned op)
{
    log_ssp = false;
    __memory_barrier();

    // Wait for idle & clear out the fifo..
    while (ssp->sr & 20)
        ssp->dr;

    *CONSOLE_CS = 1;
    SSP1->cr0 = 0x4f07;                 // divide-by-80 (1MHz), 8 bits.

    *cs = 0;
    ssp->dr = op;
}


static void spi_end (volatile ssp_t * ssp, volatile unsigned char * cs)
{
    while (ssp->sr & 16);              // Wait for idle.
    *cs = 1;

    SSP1->cr0 = 0x0007;                 // divide-by-1.
    *CONSOLE_CS = 0;
    __memory_barrier();
    log_ssp = true;
}


static void op_address (volatile ssp_t * ssp, volatile unsigned char * cs,
                        unsigned op, unsigned address)
{
    spi_start(ssp, cs, op);
    ssp->dr = address >> 16;
    ssp->dr = address >> 8;
    ssp->dr = address;
}


static bool spirom_idle(volatile ssp_t * ssp, volatile unsigned char * cs)
{
    spi_start(ssp, cs, 0xd7);
    while (!(ssp->sr & 4));
    ssp->dr;
    for (int i = 0; i != 1048576; ++i) {
        ssp->dr = 0;
        while (!(ssp->sr & 4));
        if (ssp->dr & 0x80) {
            spi_end(ssp, cs);
            return true;
        }
    }
    spi_end(ssp, cs);
    return false;
}


static void write_page (volatile ssp_t * ssp, volatile unsigned char * cs,
                        unsigned page, const unsigned char * data)
{
    unsigned buffer_write = buffer_select ? 0x84 : 0x87;

    op_address(ssp, cs, buffer_write, 0);

    for (int i = 0; i != PAGE_LEN; ++i) {
        while (!(ssp->sr & 2));
        ssp->dr = data[i];
    }
    spi_end(ssp, cs);

    // Wait for idle.
    if (!spirom_idle(ssp, cs)) {
        printf("*** Idle wait failed for page 0x%0x\n", page);
        return;
    }

    unsigned page_write = buffer_select ? 0x83 : 0x86;
    buffer_select = !buffer_select;

    op_address(ssp, cs, page_write, page * 512);
    spi_end(ssp, cs);
}


void spirom_init(void)
{
    // 8 bits.
    // Format = spi.
    // cpol - clock high between frames.
    // cpha - capture data on second clock transition.
    // We rely on the ssp log having set up the SSP.

    // Set up B16, P7_0 as function 0, GPIO3_8.  High slew rate.
    SFSP[7][0] = 0x20;
    GPIO_DIR[3] |= 1 << 8;

    printf("\nGet id:");

    // Wait for idle.
    spi_start(SSP1, ROM_CS, 0x9f);
    for (int i = 0; i != 6; ++i)
        SSP1->dr = 0;

    unsigned char bytes[7];
    for (int i = 0; i != 7; ++i) {
        while (!(SSP1->sr & 4));
        bytes[i] = SSP1->dr;
    }

    spi_end(SSP1, ROM_CS);

    for (int i = 0; i != 7; ++i)
        printf(" %02x", bytes[i]);
    printf("\n");

    spi_start(SSP1, ROM_CS, 0xd7);
    SSP1->dr = 0;
    SSP1->dr = 0;
    while (SSP1->sr & 16);              // Wait for idle.
    SSP1->dr;
    int s = SSP1->dr;
    int t = SSP1->dr;
    spi_end (SSP1, ROM_CS);
    printf("Status = %02x %02x\n", s, t);
}


static int hex_nibble (int c)
{
    if (c >= '0' && c <= '9') {
        if (verbose_flag)
            putchar(c);
        return c - '0';
    }
    c &= ~32;
    if (c >= 'A' && c <= 'F') {
        if (verbose_flag)
            putchar(c);
        return c - 'A' + 10;
    }
    printf(CLR "Illegal hex character; aborting...");
    if (c != '\n')
        while (getchar() != '\n');
    putchar('\n');
    return -1;
}


static void spirom_program(void)
{
    verbose(CLR "SPIROM program page: ");
    unsigned page = 0;
    while (true) {
        int c = getchar();
        if (c == ':')
            break;
        int n = hex_nibble(c);
        if (n < 0)
            return;
        page = page * 16 + n;
    }
    verbose(CLR "SPIROM program page 0x%x: ", page);
    unsigned char data[PAGE_LEN];
    for (int i = 0; i != PAGE_LEN; ++i) {
        int n1 = hex_nibble(getchar());
        if (n1 < 0)
            return;
        int n2 = hex_nibble(getchar());
        if (n2 < 0)
            return;
        data[i] = n1 * 16 + n2;
    }
    verbose(CLR "SPIROM program page 0x%x press ENTER: ", page);
    if (getchar() == '\n')
        write_page (SSP1, ROM_CS, page, data);
    else
        printf ("Aborted\n");
}


static void spirom_read(void)
{
    verbose(CLR "SPIROM read page: ");
    unsigned page = 0;
    while (true) {
        int c = getchar();
        if (c == '\n')
            break;
        int n = hex_nibble(c);
        if (n < 0)
            return;
        page = page * 16 + n;
    }
    printf(CLR "SPIROM read page 0x%x\n", page);
    unsigned char bytes[PAGE_LEN + 5];
    op_address(SSP1, ROM_CS, 0xb, page * 512);
    int i = 0;
    int j = 0;
    while (j < PAGE_LEN + 5 || (SSP1->sr & 0x15) != 1) {
        if (i < PAGE_LEN + 1 && (SSP1->sr & 2)) {
            ++i;
            SSP1->dr = 0;
        }
        if (SSP1->sr & 4)
            bytes[j++] = SSP1->dr;
    }
    spi_end(SSP1, ROM_CS);
    for (int i = 5; i < PAGE_LEN + 5; ++i)
        printf("%02x%s", bytes[i], (i & 15) == 4 ? "\n" : " ");
    if (PAGE_LEN & 15)
        printf("\n");
}


void spirom_command(void)
{
    spirom_init();

    while (true) {
        verbose(CLR "SPIROM: <r>ead, <p>rogram, <i>dle, <s>tatus...");
        switch (getchar()) {
        case 'p':
            spirom_program();
            break;
        case 'r':
            spirom_read();
            break;
        case 's':
            spi_start(SSP1, ROM_CS, 0xd7);
            SSP1->dr = 0;
            spi_end(SSP1, ROM_CS);
            SSP1->dr;
            printf(CLR "Status = %02x\n", SSP1->dr);
            break;
        case 'i':
            verbose(CLR "Idle wait");
            if (!spirom_idle(SSP1, ROM_CS))
                printf(CLR "Idle wait failed!\n");
            else
                verbose(CLR "SPIROM idle\n");
            break;
        default:
            printf(CLR "SPIROM exit...\n");
            return;
        }
    }
}