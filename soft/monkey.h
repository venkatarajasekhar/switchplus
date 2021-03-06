// The monkey logger.  Sends stuff to serial and/or a USB serial.
#ifndef MONKEY_H_
#define MONKEY_H_

#include <stdbool.h>

extern bool debug_flag;
extern bool verbose_flag;

void init_monkey_usb (void);

bool monkey_is_empty (void);

void putchar (int byte);
void puts (const char * s);
void printf (const char * __restrict__ format, ...)
    __attribute__ ((format (printf, 1, 2)));

int getchar (void);
void ungetchar (int c);
int peekchar_nb (void);

int hex_nibble(int c);
void _Noreturn drop_line_restart(const char * s, int c);
void get_hex_block(unsigned char * p, int len);

void gpdma_interrupt (void);

void monkey_ssp_on(void);
void monkey_ssp_off(void);

#define CLR "\r\e[K"

#define debugf __builtin_expect(!debug_flag, 1) ? (void)0 : printf
#define verbose __builtin_expect(!verbose_flag, 1) ? (void)0 : printf

#endif
