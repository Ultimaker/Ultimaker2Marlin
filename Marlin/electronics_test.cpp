#include <avr/io.h>
#include <avr/interrupt.h>
#include "electronics_test.h"
#include "watchdog.h"

static void send(const char c)
{
    while(!(UCSR0A & _BV(UDRE0))) {}
        UDR0 = c;
}

static void send(const char* str)
{
    while(*str)
        send(*str++);
}

static void send_P(const char* str)
{
    while(pgm_read_byte(str))
        send(pgm_read_byte(str++));
}

static void sendADC(uint8_t nr)
{
    ADMUX = ((1 << REFS0) | (nr & 0x07));
    ADCSRA = _BV(ADEN) | _BV(ADIF) | 0x07;
    ADCSRB = (nr & 0x08) ? _BV(MUX5) : 0;

    ADCSRA |= _BV(ADSC);//Start conversion

    while(ADCSRA & _BV(ADSC)) {}

    send(ADCL);
    send(ADCH);
}

void handleCommand(char* command)
{
    if (command[0] == 'A')
    {
        int nr = atoi(&command[1]);
        char* param_ptr = strchr(command, ' ');
        int param = -1;
        if (param_ptr)
            param = param_ptr[1];
        switch(nr)
        {
        case 0:
            send_P(PSTR(STRING_CONFIG_H_AUTHOR));
            break;
        case 1: if (param > -1) DDRA = param; break;
        case 2: if (param > -1) DDRB = param; break;
        case 3: if (param > -1) DDRC = param; break;
        case 4: if (param > -1) DDRD = param; break;
        case 5: if (param > -1) DDRE = param; break;
        case 7: if (param > -1) DDRG = param; break;
        case 8: if (param > -1) DDRH = param; break;
        case 9: if (param > -1) DDRJ = param; break;
        case 11: if (param > -1) DDRL = param | 0x38; break; //Make sure the motor PWMs are always outputs.
        case 12: if (param > -1) PORTA = param; send(PINA); break;
        case 13: if (param > -1) PORTB = param; send(PINB); break;
        case 14: if (param > -1) PORTC = param; send(PINC); break;
        case 15: if (param > -1) PORTD = param; send(PIND); break;
        case 16: if (param > -1) PORTE = param; send(PINE); break;
        case 18: if (param > -1) PORTG = param; send(PING); break;
        case 19: if (param > -1) PORTH = param; send(PINH); break;
        case 20: if (param > -1) PORTJ = param; send(PINJ); break;
        case 22: if (param > -1) PORTL = param; send(PINL); break;
        case 23: sendADC(0); break;
        case 24: sendADC(1); break;
        case 25: sendADC(2); break;
        case 26: sendADC(3); break;
        case 27: sendADC(4); break;
        case 28: sendADC(5); break;
        case 29: sendADC(6); break;
        case 30: sendADC(7); break;
        case 31: sendADC(8); break;
        case 32: sendADC(9); break;
        case 33: sendADC(10); break;
        case 34: sendADC(11); break;
        case 35: sendADC(12); break;
        case 36: sendADC(13); break;
        case 37: sendADC(14); break;
        case 38: sendADC(15); break;
        default:
            send_P(PSTR("?"));
            break;
        }
        send_P(PSTR("\r\n"));
    }else if (strlen(command) > 0)
    {
        send_P(PSTR("Unknown command: "));
        send(command);
        send_P(PSTR("\r\n"));
    }
}

void run_electronics_test()
{
    cli();

    //Disable times and PWM, except for timer5, which controls the motor current PWM levels.
    TCCR1A = 0;
    TCCR2A = 0;
    TCCR3A = 0;
    TCCR4A = 0;
    //Disable I2C and SPI
    SPCR = 0;
    TWCR = 0;

    uint8_t idx = 0;
    char command_buffer[32];
    send_P(PSTR("TEST\r\n"));
    while(true)
    {
        watchdog_reset();
        if (UCSR0A & _BV(RXC0))
        {
            uint8_t recv = UDR0;
            if (recv == '\n' || recv == '\r')
            {
                command_buffer[idx] = 0;
                idx = 0;

                handleCommand(command_buffer);
            }
            else
            {
                command_buffer[idx++] = recv;
            }
        }
    }
}

