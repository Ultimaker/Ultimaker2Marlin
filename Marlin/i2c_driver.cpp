#include <avr/io.h>
#include <util/twi.h>
#include <util/atomic.h>

#include "Configuration.h"
#include "pins.h"
#include "fastio.h"
#include "i2c_driver.h"

#ifndef I2C_SDA_PIN
//I2C pin numbers for the ArduinoMega, which we assume by default if the pins are not set by pins.h.
#define I2C_SDA_PIN   20
#define I2C_SCL_PIN   21
#endif

static uint8_t current_command_buffer_index;
static i2cCommand* current_command;
static i2cCommand* command_queue;

void i2cDriverInit()
{
    //Set the pins high as we are bus master.
    SET_OUTPUT(I2C_SDA_PIN);
    SET_OUTPUT(I2C_SCL_PIN);
    //TODO: We could generate a fake stop condition here.
    
    //Enable the internal pullups for the I2C driver. While the board has external pullups as well, this does not harm the functionality.
    WRITE(I2C_SCL_PIN, 1);
    WRITE(I2C_SDA_PIN, 1);
    
    //Set the clock frequency for the I2C by the following formula:
    //ClockFreq = (F_CPU) / (16 + 2*TWBR * 4^TWPS)
    //TWPS is set in TWSR to 0 for a prescaler of *1.
    TWBR = ((F_CPU / i2cClockFrequency) - 16)/(2*1);
    TWSR = 0x00;    //TWPS=0, other bits are read-only status bits.
}

static void i2cDriverExecuteNextCommand()
{
    //Called from interrupt context
    if (command_queue)
    {
        current_command = command_queue;
        command_queue = command_queue->next;
        current_command_buffer_index = 0;
        TWCR = _BV(TWIE) | _BV(TWEN) | _BV(TWSTA) | _BV(TWINT); //START will be transmitted, interrupt will be generated.
    }else{
        current_command = NULL;
        TWCR = _BV(TWIE) | _BV(TWEN) | _BV(TWSTO) | _BV(TWINT); //STOP condition will be transmitted and TWSTO Flag will be reset
    }
}

void i2cDriverPlan(i2cCommand* command)
{
    command->finished = false;
    
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (current_command == NULL)
        {
            // Wait till stop condition is transmitted, while unlikely to happen often,
            // but it could be that the stop condition is still being generated after the previous command was send,
            // at which point we want to wait for this before we initiate a new START action.
            loop_until_bit_is_clear(TWCR, TWSTO);
            
            current_command = command;
            current_command_buffer_index = 0;
            TWCR = _BV(TWIE) | _BV(TWIE) | _BV(TWEN) | _BV(TWSTA) | _BV(TWINT); //START will be transmitted, interrupt will be generated.
        }else{
            if (command_queue == NULL || command_queue->priority < command->priority)
            {
                command->next = NULL;
                command_queue = command;
            }else{
                for(i2cCommand* c = command_queue; ; c = c->next)
                {
                    if (c->next == NULL || c->next->priority < command->priority)
                    {
                        command->next = c->next;
                        c->next = command;
                        break;
                    }
                }
            }
        }
    }
}

void i2cDriverExecuteAndWait(i2cCommand* command)
{
    i2cDriverPlan(command);
    while(!command->finished) {}
}

//TODO: Research of ISR_NOBLOCK is possible so other interrupts can interrupt the I2C driver.
ISR(TWI_vect, ISR_BLOCK)
{
    //Called after the TWI has transmitted a START/REPEATED START condition
    //Called after the TWI has transmitted SLA+R/W
    //Called after the TWI has transmitted an address byte
    //Called after the TWI has lost arbitration
    //Called after the TWI has been addressed by own slave address or general call
    //Called after the TWI has received a data byte
    //Called after a STOP or REPEATED START has been received while still addressed as a Slave
    //Called when a bus error has occurred due to an illegal START or STOP condition
    switch(TWSR & TW_STATUS_MASK)
    {
        /* Master */
    case TW_START: //0x08
    case TW_REP_START: //0x10
        /** start condition transmitted */
        /** repeated start condition transmitted */
        TWDR = current_command->slave_address_rw;
        TWCR = _BV(TWIE) | _BV(TWEN) | _BV(TWINT); //SLA+R/W will be transmitted. Depending on R/W, will be in transmit or receive.
        break;
        /* Master Transmitter */
    case TW_MT_SLA_ACK: //0x18
    case TW_MT_SLA_NACK: //0x20
    case TW_MT_DATA_ACK: //0x28
    case TW_MT_DATA_NACK: //0x30
        /** SLA+W transmitted, ACK received */
        /** SLA+W transmitted, NACK received */
        /** data transmitted, ACK received */
        /** data transmitted, NACK received */
        if (current_command_buffer_index < current_command->buffer_size)
        {
            TWDR = current_command->buffer[current_command_buffer_index++];
            TWCR = _BV(TWIE) | _BV(TWEN) | _BV(TWINT); //Data byte will be transmitted and ACK or NOT ACK will be received
        }else{
            //i2cDriverExecuteNextCommand will initiate a repeated START or a normal STOP action.
            current_command->finished = true;
            i2cDriverExecuteNextCommand();
        }
        break;
        /* Master Receiver */
    case TW_MR_SLA_ACK: //0x40
        /** SLA+R transmitted, ACK received */
        TWCR = _BV(TWIE) | _BV(TWEN) | _BV(TWINT) | _BV(TWEA); //Data byte will be received and ACK will be returned
        break;
    case TW_MR_SLA_NACK: //0x48
        /** SLA+R transmitted, NACK received */
        TWCR = _BV(TWIE) | _BV(TWEN) | _BV(TWSTA) | _BV(TWINT); //Repeated START will be transmitted
        break;
    case TW_MR_DATA_ACK: //0x50
    case TW_MR_DATA_NACK: //0x58
        /** data received, ACK returned */
        /** data received, NACK returned */
        current_command->buffer[current_command_buffer_index++] = TWDR;
        if (current_command_buffer_index < current_command->buffer_size)
        {
            TWCR = _BV(TWIE) | _BV(TWEN) | _BV(TWINT) | _BV(TWEA); //Data byte will be received and ACK will be returned
        }else{
            //i2cDriverExecuteNextCommand will initiate a repeated START or a normal STOP action.
            current_command->finished = true;
            i2cDriverExecuteNextCommand();
        }
        break;

    /* Misc */
    case TW_MT_ARB_LOST: //0x38
    //case TW_MR_ARB_LOST: //0x38
    case TW_BUS_ERROR: //0x00
    default:
        /** arbitration lost in SLA+W or data */
        /** arbitration lost in SLA+R or NACK */
        /** illegal start or stop condition */
        //Normal TWI action is to become a slave on arbitration lost. But we cannot become a slave, as there is no other master.
        //In case of a BUS error or any other state that we cannot handle, reset the whole TWI module and start from the beginning.
        //Release the TWI module and restart it. So we can become a new master again. (This can happen due to noise on the line)
        TWCR = 0;
        TWCR = _BV(TWIE) | _BV(TWEN);
        TWCR = _BV(TWIE) | _BV(TWEN) | _BV(TWSTA) | _BV(TWINT); //START will be transmitted
        current_command_buffer_index = 0;
        break;
    }
}
