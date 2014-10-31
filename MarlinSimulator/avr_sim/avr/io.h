#ifndef _AVR_IO_H_
#define _AVR_IO_H_ 1

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef __cplusplus
#error "No C++ compiler, simulated IO requires C++! Force C++ on all files!"
#endif

#include "../../component/delegate.h"

typedef delegate<uint8_t, uint8_t&> registerDelegate;
typedef void (*sim_ms_callback_t)();

void sim_check_interrupts();
void sim_setup(sim_ms_callback_t callback);

class AVRRegistor
{
private:
    uint8_t value;
    registerDelegate callback;
public:
    AVRRegistor() : value(0), callback() {}
    ~AVRRegistor() {}
    
    void setCallback(registerDelegate callback) { this->callback = callback; }
    void forceValue(uint8_t value) { this->value = value; }
    
    operator uint8_t() const { return value; }
    AVRRegistor& operator = (const uint32_t v);
    AVRRegistor& operator |= (const uint32_t n) { *this = (value | n); return *this; }
    AVRRegistor& operator &= (const uint32_t n) { *this = (value & n); return *this; }

    //volatile uint8_t* operator & () __attribute__((__deprecated__)) { return &value; }
};
#define __REG_MAP_SIZE 0x200
extern AVRRegistor __reg_map[__REG_MAP_SIZE];

class AVRRegistor16
{
private:
    int index;
public:
    AVRRegistor16(int index) : index(index) {}
    ~AVRRegistor16() {}
    
    AVRRegistor16& operator = (const uint32_t v) { __reg_map[index] = v & 0xFF; __reg_map[index+1] = (v >> 8) & 0xFF; return *this; }
    operator uint16_t() const { return uint16_t(__reg_map[index]) | (uint16_t(__reg_map[index+1])<<8); /*TODO*/}
};

#define _SFR_MEM8(__n) (__reg_map[(__n)])
#define _SFR_MEM16(__n) (AVRRegistor16(__n))
#define _SFR_IO8(__n) (__reg_map[((__n) + 0x20)])
#define _VECTOR(__n) __vector_ ## __n
#define _SFR_BYTE(__n) __n

#define _BV(bit) (1 << (bit))

#define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))

#include "iom2560.h"

#endif//_AVR_IO_H_
