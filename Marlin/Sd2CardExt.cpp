/* Arduino Sdio Library
 * Copyright (C) 2014 by Munehiro Doi
 *
 * This file is an SD extension of the Arduino Sd2Card Library
 * Copyright (C) 2009 by William Greiman
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino Sd2Card Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <Arduino.h>
#include "Sd2CardExt.h"

//------------------------------------------------------------------------------
// SD extension commands
/** EXTENSION READ - Extension Register Read Command (Single Block) */
uint8_t const CMD48 = 0X30;
/** EXTENSION WRITE - Extension Register Write Command (Single Block) */
uint8_t const CMD49 = 0X31;
//------------------------------------------------------------------------------
// SD extension error codes.
/** card returned an error response for CMD48 (read extension block) */
uint8_t const SD_CARD_ERROR_CMD48 = 0X80;
/** card returned an error response for CMD49 (write extension block) */
uint8_t const SD_CARD_ERROR_CMD49 = 0X81;
//------------------------------------------------------------------------------

// functions for hardware SPI
/** Send a byte to the card */
inline void spiSend(uint8_t b) {
  SPDR = b;
  while (!(SPSR & (1 << SPIF)));
}

/** Receive a byte from the card */
inline uint8_t spiReceive(void) {
  spiSend(0xFF);
  return SPDR;
}

//------------------------------------------------------------------------------
/** Perform Extention Read. */
uint8_t Sd2CardExt::readExt(uint32_t arg, uint8_t* dst, uint16_t count) {
  uint16_t i;

  // send command and argument.
  if (cardCommand(CMD48, arg)) {
    error(SD_CARD_ERROR_CMD48);
    goto fail;
  }

  // wait for start block token.
  if (!waitStartBlock()) {
    goto fail;
  }

  // receive data
  for (i = 0; i < count; ++i) {
    dst[i] = spiReceive();
  }

  // skip dummy bytes and 16-bit crc.
  for (; i < 514; ++i) {
    spiReceive();
  }

  chipSelectHigh();
  spiSend(0xFF); // dummy clock to force FlashAir finish the command.
  return true;

 fail:
  chipSelectHigh();
  return false;
}
//------------------------------------------------------------------------------
/** Perform Extention Write. */
uint8_t Sd2CardExt::writeExt(uint32_t arg, const uint8_t* src, uint16_t count) {
  uint16_t i;
  uint8_t status;

  // send command and argument.
  if (cardCommand(CMD49, arg)) {
    error(SD_CARD_ERROR_CMD49);
    goto fail;
  }

  // send start block token.
  spiSend(DATA_START_BLOCK);

  // send data
  for (i = 0; i < count; ++i) {
    spiSend(src[i]);
  }

  // send dummy bytes until 512 bytes.
  for (; i < 512; ++i) {
    spiSend(0xFF);
  }

  // dummy 16-bit crc
  spiSend(0xFF);
  spiSend(0xFF);

  // wait a data response token
  status = spiReceive();
  if ((status & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
    error(SD_CARD_ERROR_WRITE);
    goto fail;
  }

  // wait for flash programming to complete
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
    error(SD_CARD_ERROR_WRITE_TIMEOUT);
    goto fail;
  }

  chipSelectHigh();
  return true;

 fail:
  chipSelectHigh();
  return false;
}

//------------------------------------------------------------------------------
/**
 * Read a 512 byte data port in an extension register space.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2CardExt::readExtDataPort(uint8_t mio, uint8_t func,
    uint16_t addr, uint8_t* dst) {
  uint32_t arg =
      (((uint32_t)mio & 0x1) << 31) |
	  (mio ? (((uint32_t)func & 0x7) << 28) : (((uint32_t)func & 0xF) << 27)) |
	  (((uint32_t)addr & 0x1FE00) << 9);

  return readExt(arg, dst, 512);
}
//------------------------------------------------------------------------------
/**
 * Read an extension register space.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2CardExt::readExtMemory(uint8_t mio, uint8_t func,
    uint32_t addr, uint16_t count, uint8_t* dst) {
  uint32_t offset = addr & 0x1FF;
  if (offset + count > 512) count = 512 - offset;

  if (count == 0) return true;

  uint32_t arg =
      (((uint32_t)mio & 0x1) << 31) |
	  (mio ? (((uint32_t)func & 0x7) << 28) : (((uint32_t)func & 0xF) << 27)) |
	  ((addr & 0x1FFFF) << 9) |
	  ((count - 1) & 0x1FF);

  return readExt(arg, dst, count);
}

//------------------------------------------------------------------------------
/**
 * Write a 512 byte data port into an extension register space.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2CardExt::writeExtDataPort(uint8_t mio, uint8_t func,
    uint16_t addr, const uint8_t* src) {
  uint32_t arg =
      (((uint32_t)mio & 0x1) << 31) |
	  (mio ? (((uint32_t)func & 0x7) << 28) : (((uint32_t)func & 0xF) << 27)) |
	  (((uint32_t)addr & 0x1FE00) << 9);

  return writeExt(arg, src, 512);
}
//------------------------------------------------------------------------------
/**
 * Write an extension register space.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2CardExt::writeExtMemory(uint8_t mio, uint8_t func,
    uint32_t addr, uint16_t count, const uint8_t* src) {
  uint32_t arg =
      (((uint32_t)mio & 0x1) << 31) |
	  (mio ? (((uint32_t)func & 0x7) << 28) : (((uint32_t)func & 0xF) << 27)) |
	  ((addr & 0x1FFFF) << 9) |
	  ((count - 1) & 0x1FF);

  return writeExt(arg, src, count);
}

//------------------------------------------------------------------------------
/**
 * Writes a byte-data with mask into an extension register space.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t Sd2CardExt::writeExtMask(uint8_t mio, uint8_t func,
    uint32_t addr, uint8_t mask, const uint8_t* src) {
  uint32_t arg =
      (((uint32_t)mio & 0x1) << 31) |
	  (mio ? (((uint32_t)func & 0x7) << 28) : (((uint32_t)func & 0xF) << 27)) |
	  (0x1 << 26) |
	  ((addr & 0x1FFFF) << 9) |
	  mask;

  return writeExt(arg, src, 1);
}

///**
// * Enable an SD flash memory card.
// *
// * \return The value one, true, is returned for success and
// * the value zero, false, is returned for failure.  The reason for failure
// * can be determined by calling errorCode() and errorData().
// */
//uint8_t Sd2CardExt::reset() {
//#ifdef SD_RESET
//  // 16-bit init start time allows over a minute
//  uint16_t t0 = (uint16_t)millis();
//
//  chipSelectLow();
//
//  // command to go idle in SPI mode
//  while ((status_ = cardCommand(CMD0, 0)) != R1_IDLE_STATE) {
//    if (((uint16_t)(millis() - t0)) > SD_INIT_TIMEOUT) {
//      error(SD_CARD_ERROR_CMD0);
//      return false;
//    }
//  }
//
//  chipSelectHigh();
//#endif
//  return true;
//}
