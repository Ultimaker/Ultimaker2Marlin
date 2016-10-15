#include <avr/io.h>
#include <avr/interrupt.h>
#include "electronics_test.h"
#include "watchdog.h"

#define I2C_SDA_PIN   20
#define I2C_SCL_PIN   21

#define I2C_FREQ 400000

#define I2C_WRITE   0x00
#define I2C_READ    0x01

static void send(const char c)
{
    while(!(UCSR0A & _BV(UDRE0))) {}
        UDR0 = c;
}

//static void sendHex(uint8_t c)
//{
//    if (((c & 0xF0) >> 4) < 10)
//        send('0' + ((c & 0xF0) >> 4));
//    else
//        send('A' - 10 + ((c & 0xF0) >> 4));
//
//    if ((c & 0x0F) < 10)
//        send('0' + (c & 0x0F));
//    else
//        send('A' - 10 + (c & 0x0F));
//}

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

static void i2c_check_and_enable()
{
    if (!(TWEN & _BV(TWEN)))
    {
        //I2C is not enabled yet, enable it so we can use it.
        SET_OUTPUT(I2C_SDA_PIN);
        SET_OUTPUT(I2C_SCL_PIN);

        WRITE(I2C_SDA_PIN, 1);
        WRITE(I2C_SCL_PIN, 1);

        TWBR = ((F_CPU / I2C_FREQ) - 16)/2*1;
        TWSR = 0x00;
    }
}

static inline void i2c_start()
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT))) {}
}

static inline void i2c_send_raw(uint8_t data)
{
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT))) {}
}

static inline uint8_t i2c_recv_raw_no_ack()
{
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT))) {}
    return TWDR;
}

static inline void i2c_end()
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
    while ((TWCR & (1<<TWSTO))) {}
}

static uint8_t addressWriteReg(uint8_t addr, uint8_t reg, uint8_t data)
{
    i2c_check_and_enable();
    i2c_start();
    i2c_send_raw((addr << 1) | I2C_WRITE);
    if (TWSR == 0x18)
    {
        i2c_send_raw(reg);
        i2c_send_raw(data);
        i2c_end();
        return 1;
    }
    i2c_end();
    return 0;
}

static uint8_t addressI2C(uint8_t addr)
{
    i2c_check_and_enable();
    i2c_start();
    i2c_send_raw((addr << 1) | I2C_WRITE);
    if (TWSR == 0x18)
    {
        i2c_end();
        return 1;
    }
    i2c_end();
    return 0;
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
        case 100: if (addressI2C(param)) { send('1'); } else { send('0'); } break;
        case 101:
            addressWriteReg(0b1100001, 0, 0x80);//MODE1
            addressWriteReg(0b1100001, 1, 0x1C);//MODE2
            addressWriteReg(0b1100001, 2, param == 1 ? 255 : 0);//PWM0=Red
            addressWriteReg(0b1100001, 3, param == 2 ? 255 : 0);//PWM1=Green
            addressWriteReg(0b1100001, 4, param == 3 ? 255 : 0);//PWM2=Blue
            addressWriteReg(0b1100001, 5, param == 4 ? 255 : 0);//PWM3=White
            addressWriteReg(0b1100001, 6, 0xFF);//GRPPWM
            addressWriteReg(0b1100001, 7, 0x00);//GRPFREQ
            addressWriteReg(0b1100001, 8, 0xAA);//LEDOUT
            break;
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

