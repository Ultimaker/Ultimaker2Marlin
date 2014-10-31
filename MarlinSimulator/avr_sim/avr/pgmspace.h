#ifndef AVR_PGMSPACE_H
#define AVR_PGMSPACE_H

#include <stdint.h>

typedef char prog_char;

#define PROGMEM __attribute__(())
#define PGM_P const prog_char *
#define PSTR(str) (str)
#define strcpy_P strcpy
#define strlen_P strlen
#define strcmp_P strcmp
#define sprintf_P sprintf
#define strcat_P strcat
#define strstr_P strstr
#define strncmp_P strncmp
#define strncpy_P strncpy
#define strchr_P strchr

static inline uint8_t pgm_read_byte(const void* ptr)
{
    return *(const uint8_t*)ptr;
}

static inline uint16_t pgm_read_word(const void* ptr)
{
    return *(const uint16_t*)ptr;
}

static inline float pgm_read_float(const void* ptr)
{
    return *(const float*)ptr;
}

#define pgm_read_byte_near(n) pgm_read_byte(n)
#define pgm_read_word_near(n) pgm_read_word(n)
#define pgm_read_float_near(n) pgm_read_float(n)

/* Cheating some none-standard C functions in here (normally provided by avr-libc), so WString.h of Arduino sees this function */
inline static char * 	ultoa (unsigned long int __val, char *__s, int __radix)
{
    char* c = __s;
    unsigned long n = __radix;
    while(n < __val)
        n *= __radix;
    n /= __radix;
    while(n)
    {
        int v = (__val / n) % __radix;
        if (v < 10)
            *c++ = '0' + v;
        else
            *c++ = 'a' + v - 10;
        n /= __radix;
    }
    *c = '\0';
    return __s;
}

inline static char * 	utoa (unsigned int __val, char *__s, int __radix)
{
    return ultoa(__val, __s, __radix);
}


inline static double square(double __x) { return __x * __x; }

#endif//AVR_PGMSPACE_H
