#ifndef REGISTERS_H_
#define REGISTERS_H_

#define __memory_barrier() asm volatile ("" : : : "memory")
#define __interrupt_disable() asm volatile ("cpsid i\n" ::: "memory");
#define __interrupt_enable() asm volatile ("cpsie i\n" ::: "memory");
#define __interrupt_wait() asm volatile ("wfi\n");

#define __section(s) __attribute__ ((section (s)))
#define __aligned(s) __attribute__ ((aligned (s)))

typedef volatile unsigned char v8;
typedef volatile unsigned v32;
typedef v8 v8_32[32];
typedef v32 v32_32[32];

typedef struct dTD_t dTD_t;
typedef struct dQH_t dQH_t;

typedef void dtd_completion_t (dTD_t * dtd);

// USB transfer descriptor.
struct dTD_t {
    struct dTD_t * volatile next;
    volatile unsigned length_and_status;
    unsigned volatile buffer_page[5];

    dtd_completion_t * completion;      // For our use...
};

// USB queue head.
struct dQH_t {
    // 48 byte queue head.
    volatile unsigned capabilities;
    dTD_t * volatile current;

    dTD_t * volatile next;
    volatile unsigned length_and_status;
    unsigned volatile buffer_page[5];

    volatile unsigned reserved;
    volatile unsigned setup0;
    volatile unsigned setup1;
    // 16 bytes remaining for our use...
    dTD_t * first;
    dTD_t * last;
    unsigned dummy2;
    unsigned dummy3;
};

// Ethernet DMA descriptor (short form).
typedef struct EDMA_DESC_t {
    unsigned status;
    unsigned count;
    void * buffer1;
    void * buffer2;
} EDMA_DESC_t;

#define GPIO 0x400F4000

#define GPIO_BYTE ((v8_32 *) GPIO)
#define GPIO_DIR ((v32 *) (GPIO + 0x2000))

#define USART3_THR ((v32 *) 0x400c2000)
#define USART3_RBR ((v32 *) 0x400c2000)
#define USART3_DLL ((v32 *) 0x400c2000)
#define USART3_DLM ((v32 *) 0x400c2004)
#define USART3_IER ((v32 *) 0x400c2004)
#define USART3_FCR ((v32 *) 0x400c2008)
#define USART3_LCR ((v32 *) 0x400c200c)
#define USART3_LSR ((v32 *) 0x400c2014)
#define USART3_SCR ((v32 *) 0x400c201c)
#define USART3_ACR ((v32 *) 0x400c2020)
#define USART3_ICR ((v32 *) 0x400c2024)
#define USART3_FDR ((v32 *) 0x400c2028)
#define USART3_OSR ((v32 *) 0x400c202c)
#define USART3_HDEN ((v32 *) 0x400c2040)

#define CGU 0x40050000

#define FREQ_MON ((v32 *) (CGU + 0x14))

#define PLL0USB_STAT ((v32 *) (CGU + 0x1c))
#define PLL0USB_CTRL ((v32 *) (CGU + 0x20))
#define PLL0USB_MDIV ((v32 *) (CGU + 0x24))
#define PLL0USB_NP_DIV ((v32 *) (CGU + 0x28))

#define PLL0AUDIO_STAT ((v32 *) (CGU + 0x2c))
#define PLL0AUDIO_CTRL ((v32 *) (CGU + 0x30))
#define PLL0AUDIO_MDIV ((v32 *) (CGU + 0x34))
#define PLL0AUDIO_NP_DIV ((v32 *) (CGU + 0x38))
#define PLL0AUDIO_FRAC ((v32 *) (CGU + 0x3c))

#define PLL1_STAT ((v32*) (CGU + 0x40))
#define PLL1_CTRL ((v32*) (CGU + 0x44))

#define IDIVA_CTRL ((v32*) (CGU + 0x48))
#define IDIVC_CTRL ((v32*) (CGU + 0x50))
#define BASE_M4_CLK ((v32*) (CGU + 0x6c))

#define BASE_PHY_RX_CLK ((v32 *) (CGU + 0x78))
#define BASE_PHY_TX_CLK ((v32 *) (CGU + 0x7c))
#define BASE_LCD_CLK ((v32 *) (CGU + 0x88))
#define BASE_SSP1_CLK ((v32 *) (CGU + 0x98))
#define BASE_UART3_CLK ((v32 *) (CGU + 0xa8))

#define CCU1 ((v32 *) 0x40051000)
#define CCU2 ((v32 *) 0x40052000)

#define SCU 0x40086000

#define SFSP ((v32_32 *) SCU)
#define SFSCLK ((v32 *) (SCU + 0xc00))
#define EMCDELAYCLK ((v32*) 0x40086d00)

#define OTP ((const unsigned *) 0x40045000)

typedef struct ssp_t {
    unsigned cr0;
    unsigned cr1;
    unsigned dr;
    unsigned sr;
    unsigned cpsr;
    unsigned imsc;
    unsigned ris;
    unsigned mis;
    unsigned icr;
    unsigned dmacr;
} ssp_t;

#define SSP0 ((volatile ssp_t *) 0x40083000)
#define SSP1 ((volatile ssp_t *) 0x400c5000)

#define RESET_CTRL ((v32 *) 0x40053100)
#define RESET_ACTIVE_STATUS ((v32 *) 0x40053150)

#define CREG0 ((v32 *) 0x40043004)
#define M4MEMMAP ((v32 *) 0x40043100)
#define FLASHCFGA ((v32 *) 0x40043120)
#define FLASHCFGB ((v32 *) 0x40043124)
#define CREG6 ((v32 *) 0x4004312c)

#define ENET 0x40010000
#define MAC_CONFIG ((v32 *) ENET)
#define MAC_FRAME_FILTER ((v32 *) (ENET + 4))
#define MAC_HASHTABLE_HIGH ((v32 *) (ENET + 8))
#define MAC_HASHTABLE_LOW ((v32 *) (ENET + 12))
#define MAC_MII_ADDR ((v32 *) (ENET + 16))
#define MAC_MII_DATA ((v32 *) (ENET + 20))
#define MAC_FLOW_CTRL ((v32 *) (ENET + 24))
#define MAC_VLAN_TAG ((v32 *) (ENET + 28))

#define MAC_DEBUG ((v32 *) (ENET + 36))
#define MAC_RWAKE_FRFLT ((v32 *) (ENET + 40))
#define MAC_PMT_CTRL_STAT ((v32 *) (ENET + 44))

#define MAC_INTR ((v32 *) (ENET + 56))
#define MAC_INTR_MASK ((v32 *) (ENET + 60))
#define MAC_ADDR0_HIGH ((v32 *) (ENET + 64))
#define MAC_ADDR0_LOW ((v32 *) (ENET + 68))

#define MAC_TIMESTP_CTRL ((v32 *) (ENET + 0x700))

#define EDMA_BUS_MODE ((v32 *) (ENET + 0x1000))
#define EDMA_TRANS_POLL_DEMAND ((v32 *) (ENET + 0x1000 + 4))
#define EDMA_REC_POLL_DEMAND ((v32 *) (ENET + 0x1000 + 8))
#define EDMA_REC_DES_ADDR ((v32 *) (ENET + 0x1000 + 12))
#define EDMA_TRANS_DES_ADDR ((v32 *) (ENET + 0x1000 + 16))
#define EDMA_STAT ((v32 *) (ENET + 0x1000 + 20))
#define EDMA_OP_MODE ((v32 *) (ENET + 0x1000 + 24))
#define EDMA_INT_EN ((v32 *) (ENET + 0x1000 + 28))
#define EDMA_MFRM_BUFOF ((v32 *) (ENET + 0x1000 + 32))
#define EDMA_REC_INT_WDT ((v32 *) (ENET + 0x1000 + 36))

#define EDMA_CURHOST_TRANS_DES ((v32 *) (ENET + 0x1000 + 72))
#define EDMA_CURHOST_REC_DES ((v32 *) (ENET + 0x1000 + 76))
#define EDMA_CURHOST_TRANS_BUF ((v32 *) (ENET + 0x1000 + 80))
#define EDMA_CURHOST_RECBUF ((v32 *) (ENET + 0x1000 + 84))

#define USB0 0x40006000

#define CAPLENGTH ((v32*) (USB0 + 0x100))
#define HCSPARAMS ((v32*) (USB0 + 0x104))
#define HCCPARAMS ((v32*) (USB0 + 0x108))
#define DCIVERSION ((v32*) (USB0 + 0x120))
#define DCCPARAMS ((v32*) (USB0 + 0x124))

#define USBCMD ((v32*) (USB0 + 0x140))
#define USBSTS ((v32*) (USB0 + 0x144))
#define USBINTR ((v32*) (USB0 + 0x148))
#define FRINDEX ((v32*) (USB0 + 0x14c))
#define DEVICEADDR ((v32*) (USB0 + 0x154))
#define PERIODICLISTBASE ((v32*) (USB0 + 0x154))
#define ENDPOINTLISTADDR ((v32*) (USB0 + 0x158))
#define ASYNCLISTADDR ((v32*) (USB0 + 0x158))
#define TTCTRL ((v32*) (USB0 + 0x15c))
#define BURSTSIZE ((v32*) (USB0 + 0x160))
#define TXFILLTUNING ((v32*) (USB0 + 0x164))
#define BINTERVAL ((v32*) (USB0 + 0x174))
#define ENDPTNAK ((v32*) (USB0 + 0x178))
#define ENDPTNAKEN ((v32*) (USB0 + 0x17c))
#define PORTSC1 ((v32*) (USB0 + 0x184))
#define OTGSC ((v32*) (USB0 + 0x1a4))
#define USBMODE ((v32*) (USB0 + 0x1a8))

#define ENDPTSETUPSTAT ((v32*) (USB0 + 0x1ac))
#define ENDPTPRIME ((v32*) (USB0 + 0x1b0))
#define ENDPTFLUSH ((v32*) (USB0 + 0x1b4))
#define ENDPTSTAT ((v32*) (USB0 + 0x1b8))
#define ENDPTCOMPLETE ((v32*) (USB0 + 0x1bc))
#define ENDPTCTRL0 ((v32*) (USB0 + 0x1c0))
#define ENDPTCTRL1 ((v32*) (USB0 + 0x1c4))
#define ENDPTCTRL2 ((v32*) (USB0 + 0x1c8))
#define ENDPTCTRL3 ((v32*) (USB0 + 0x1cc))
#define ENDPTCTRL4 ((v32*) (USB0 + 0x1d0))
#define ENDPTCTRL5 ((v32*) (USB0 + 0x1d4))

#define NVIC ((v32*) 0xE000E000)
#define NVIC_ISER (NVIC + 64)
#define NVIC_ICER (NVIC + 96)
#define NVIC_ISPR (NVIC + 128)
#define NVIC_ICPR (NVIC + 160)
#define NVIC_IABR (NVIC + 192)
#define NVIC_IPR (NVIC + 256)
#define NVIC_STIR (NVIC + 960)

#define CORTEX_M_AIRCR ((v32*) 0xe000ed0c)

#define EMC ((v32*) 0x40005000)

#define EMCCONTROL (EMC)
#define EMCSTATUS (EMC + 1)
#define EMCCONFIG (EMC + 2)

#define DYNAMICCONTROL (EMC + 8)
#define DYNAMICREFRESH (EMC + 9)
#define DYNAMICREADCONFIG (EMC + 10)

#define DYNAMICRP   (EMC + 12)
#define DYNAMICRAS  (EMC + 13)
#define DYNAMICSREX (EMC + 14)
#define DYNAMICAPR  (EMC + 15)
#define DYNAMICDAL  (EMC + 16)
#define DYNAMICWR   (EMC + 17)
#define DYNAMICRC   (EMC + 18)
#define DYNAMICRFC  (EMC + 19)
#define DYNAMICXSR  (EMC + 20)
#define DYNAMICRRD  (EMC + 21)
#define DYNAMICMRD  (EMC + 22)

#define DYNAMICCONFIG0 (EMC + 64)
#define DYNAMICRASCAS0 (EMC + 65)

#define DYNAMICCONFIG1 (EMC + 72)
#define DYNAMICRASCAS1 (EMC + 73)

#define DYNAMICCONFIG2 (EMC + 80)
#define DYNAMICRASCAS2 (EMC + 81)

#define DYNAMICCONFIG3 (EMC + 88)
#define DYNAMICRASCAS3 (EMC + 89)

#define LCD ((v32*) 0x40008000)

#define LCD_TIMH (LCD + 0)
#define LCD_TIMV (LCD + 1)
#define LCD_POL (LCD + 2)
#define LCD_LE (LCD + 3)
#define LCD_UPBASE (LCD + 4)
#define LCD_LPBASE (LCD + 5)
#define LCD_CTRL (LCD + 6)
#define LCD_INTMSK (LCD + 7)
#define LCD_INTRAW (LCD + 8)
#define LCD_INTSTAT (LCD + 9)
#define LCD_INTCLR (LCD + 10)
#define LCD_UPCURR (LCD + 11)
#define LCD_LPCURR (LCD + 12)

#define LCD_PAL (LCD + 128)

#define CRSR_IMG (LCD + 512)

#define CRSR (LCD + 768)

#define CRSR_CTRL (CRSR + 0)
#define CRSR_CFG (CRSR + 1)
#define CRSR_PAL0 (CRSR + 2)
#define CRSR_PAL1 (CRSR + 3)
#define CRSR_XY (CRSR + 4)
#define CRSR_CLIP (CRSR + 5)

#define CRSR_INTMSK (CRSR + 8)
#define CRSR_INTCLR (CRSR + 9)
#define CRSR_INTRAW (CRSR + 10)
#define CRSR_INTSTAT (CRSR + 11)

typedef struct i2c_t {
    unsigned conset;
    unsigned stat;
    unsigned dat;
    unsigned adr0;
    unsigned sclh;
    unsigned scll;
    unsigned conclr;
    unsigned mmctl;
    unsigned adr1;
    unsigned adr2;
    unsigned adr3;
    unsigned data_buffer;
    unsigned mask0;
    unsigned mask1;
    unsigned mask2;
    unsigned mask3;
} i2c_t;

#define I2C0 ((volatile i2c_t *) 0x400a1000)

#endif