#ifndef _SIM_INTERRUPT_H
#define _SIM_INTERRUPT_H

#include <avr/io.h>

#define SREG _SFR_IO8(0x3F)
#define SREG_I  (7)

static inline void cli() { SREG &=~_BV(SREG_I); }
static inline void sei() { SREG |= _BV(SREG_I); }

#define ISR(vector_name) void vector_name ()
#define SIGNAL(vector_name) void vector_name (void)

#endif//_SIM_INTERRUPT_H
