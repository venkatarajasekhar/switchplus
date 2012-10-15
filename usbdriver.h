// USB buffer / endpoint handling.
#ifndef USBDRIVER_H_
#define USBDRIVER_H_

#include "registers.h"

#include <stdbool.h>

void usb_init (void);
void qh_init (unsigned ep, unsigned capabilities);

dTD_t * get_dtd (void);
void schedule_dtd (unsigned ep, dTD_t * dtd);
bool schedule_buffer (unsigned ep, const void * data, unsigned length,
                      dtd_completion_t * cb);
void endpt_complete (unsigned ep, bool reprime);

unsigned long long get_0_setup (void);

#endif
