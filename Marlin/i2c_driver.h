#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stdint.h>
/**
A AVR I2C driver using the TWI hardware in the AVR controller.

It has the following design conciderations:
* Interrupt based. No busy waiting for normal operation.
* Automated retry on failure. And reporting back to the application when finished.
* User defined memory buffers, means there is no limit on buffer sizes, and the application holds the buffers.
* No dynamic memory, no dynamic configuration. All pieces of memory and settings are staticly defined at compile time.
* Priorities. Some I2C communication is more important then other. Schedule the next action on priority.
*/

const long int i2cClockFrequency = 400000;  //Clock frequency of the I2C driver. Most chips support both 100khz and 400khz operation.
const uint8_t i2cWriteBit = 0x00;
const uint8_t i2cReadBit = 0x01;

struct i2cCommand {
    uint8_t priority;           //Priority of the command (higher takes preference)
    uint8_t slave_address_rw;   //Address and Read/Write bit of the slave ship (SLA+RW)
    uint8_t* buffer;            //Pointed to the read or write buffer. This buffer is send when RW bit is i2cWrite. Or filled when the RW bit is i2cWrite
    uint8_t buffer_size;        //Size of the buffer in bytes.
    volatile uint8_t finished;  //false when the i2c command is being executed. During executing all other fields should not be accessed. True when the execution is finished.
    
    //Private, do not access, used by the priority queue
    i2cCommand* volatile next;
};

/**
    Initialize the I2C driver, called during setup.
 */
void i2cDriverInit();
/**
    Plan a i2cCommand to be executed by the I2C driver. The command is scheduled and executed with interrupts.
    You can call this function from interrupt context.
    Do not call this function when the i2cCommand.finished is false.
 */
void i2cDriverPlan(i2cCommand* command);
/**
    Execute i2cCommand, and wait for it to finish before returning.
    Do not call this function from interrupt context.
    Do not call this function when the i2cCommand.finished is false.
 */
void i2cDriverExecuteAndWait(i2cCommand* command);
/**
    Setup the I2C command structure.
    Set the priority (higher priority commands take preference over lower)
    Sets the I2C address + read/write bit (SLA+RW)
    And sets the buffer used for reading or writting and the size of that buffer.
 */
static inline void i2cDriverCommandSetup(i2cCommand& command, uint8_t address_rw, uint8_t priority, uint8_t* buffer, uint8_t buffer_size)
{
    command.priority = priority;
    command.slave_address_rw = address_rw;
    command.buffer = buffer;
    command.buffer_size = buffer_size;
    command.finished = true;
}

#endif//I2C_DRIVER_H
