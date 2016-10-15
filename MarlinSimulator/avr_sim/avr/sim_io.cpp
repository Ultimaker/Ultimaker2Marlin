#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <SDL/SDL.h>

#include "../../Marlin/configuration.h"
#include "../../Marlin/pins.h"
#include "../../Marlin/fastio.h"

AVRRegistor __reg_map[__REG_MAP_SIZE];
uint8_t __eeprom__storage[4096];
sim_ms_callback_t ms_callback;

unsigned int __bss_end;
unsigned int __heap_start;
void *__brkval;

extern void TWI_vect();
extern void TIMER0_OVF_vect();
extern void TIMER0_COMPB_vect();
extern void TIMER1_COMPA_vect();

unsigned int prevTicks = SDL_GetTicks();
unsigned int twiIntStart = 0;

//After an interrupt we need to set the interrupt flag again, but do this without calling sim_check_interrupts so the interrupt does not fire recursively
#define _sei() do { SREG.forceValue(SREG | _BV(SREG_I)); } while(0)

void sim_check_interrupts()
{
    if (!(SREG & _BV(SREG_I)))
        return;

    unsigned int ticks = SDL_GetTicks();
    int tickDiff = ticks - prevTicks;
    prevTicks = ticks;

#ifdef ENABLE_ULTILCD2
    if ((TWCR & _BV(TWEN)) && (TWCR & _BV(TWINT)) && (TWCR & _BV(TWIE)))
    {
        //Relay the TWI interrupt by 25ms one time till it gets disabled again. This fakes the LCD refresh rate.
        if (twiIntStart == 0)
            twiIntStart = SDL_GetTicks();
        if (SDL_GetTicks() - twiIntStart > 25)
        {
            cli();
            TWI_vect();
            _sei();
        }
    }
    if (!(TWCR & _BV(TWEN)) || !(TWCR & _BV(TWIE)))
    {
        twiIntStart = 0;
    }
#endif

    //if (tickDiff > 1)
    //    printf("Ticks slow! %i\n", tickDiff);
    if (tickDiff > 0)
    {
        ms_callback();

        cli();
        for(int n=0;n<tickDiff;n++)
        {
            if (TIMSK0 & _BV(OCIE0B))
                TIMER0_COMPB_vect();
            if (TIMSK0 & _BV(TOIE0))
                TIMER0_OVF_vect();
        }

        //Timer1 runs at 16Mhz / 8 ticks per second.
//        unsigned int waveformMode = ((TCCR1B & (_BV(WGM13) | _BV(WGM12))) >> 1) | (TCCR1A & (_BV(WGM11) | _BV(WGM10)));
        unsigned int clockSource = TCCR1B & (_BV(CS12) | _BV(CS11) | _BV(CS10));
        unsigned int tickCount = F_CPU * tickDiff / 1000;
        unsigned int ticks = TCNT1;
        switch(clockSource)
        {
        case 0: tickCount = 0; break;
        case 1: break;
        case 2: tickCount /= 8; break;
        case 3: tickCount /= 64; break;
        case 4: tickCount /= 256; break;
        case 5: tickCount /= 1024; break;
        case 6: tickCount = 0; break;
        case 7: tickCount = 0; break;
        }

        if (tickCount > 0 && OCR1A > 0)
        {
            ticks += tickCount;
            while(ticks > int(OCR1A))
            {
                ticks -= int(OCR1A);
                if (TIMSK1 & _BV(OCIE1A))
                    TIMER1_COMPA_vect();
            }
            TCNT1 = ticks;
        }
        _sei();
    }
}

extern void sim_setup_main();

//Assignment opperator called on every register write.
AVRRegistor& AVRRegistor::operator = (const uint32_t v)
{
    uint8_t n = v;
    if (!ms_callback) sim_setup_main();
    callback(value, n);
    value = n;
    sim_check_interrupts();
    return *this;
}

void sim_setup(sim_ms_callback_t callback)
{
    FILE* f = fopen("eeprom.save", "rb");
    if (f)
    {
        fread(__eeprom__storage, sizeof(__eeprom__storage), 1, f);
        fclose(f);
    }
    ms_callback = callback;

    UCSR0A = 0;
}
