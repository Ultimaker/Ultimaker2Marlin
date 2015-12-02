/* Arduino Sdio Library
 * Copyright (C) 2014 by Munehiro Doi
 *
 * This file is an iSDIO extension of the Arduino Sd2Card Library
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
#ifndef Sd2CardExt_h
#define Sd2CardExt_h

#include "Sd2Card.h"

//------------------------------------------------------------------------------
/**
 * \class Sd2CardExt
 * \brief Extension for raw access to SDHC flash memory cards.
 */
class Sd2CardExt : public Sd2Card {
 public:
  /** Construct an instance of Sd2Card. */
  Sd2CardExt(void) : Sd2Card() {}

  uint8_t readExtDataPort(uint8_t mio, uint8_t func, uint16_t addr, uint8_t* dst);
  uint8_t readExtMemory(uint8_t mio, uint8_t func, uint32_t addr, uint16_t count, uint8_t* dst);
  uint8_t writeExtDataPort(uint8_t mio, uint8_t func, uint16_t addr, const uint8_t* src);
  uint8_t writeExtMemory(uint8_t mio, uint8_t func, uint32_t addr, uint16_t count, const uint8_t* src);
  uint8_t writeExtMask(uint8_t mio, uint8_t func, uint32_t addr, uint8_t mask, const uint8_t* src);

  uint8_t reset();
protected:
  uint8_t readExt(uint32_t arg, uint8_t* src, uint16_t count);
  uint8_t writeExt(uint32_t arg, const uint8_t* src, uint16_t count);
};

#endif  // Sd2CardExt_h
