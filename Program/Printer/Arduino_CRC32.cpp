/*
  Copyright (c) 2020 Arduino.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include "Arduino_CRC32.h"

/**************************************************************************************
 * PUBLIC MEMBER FUNCTIONS
 **************************************************************************************/

void Arduino_CRC32::init(uint32_t start)
{
  crc32_start = start;
}

uint32_t Arduino_CRC32::calc(uint8_t const data[], uint32_t const len)
{
  crc_t crc32;
  crc32 = crc_update(crc32_start, data, len);
  crc32 = crc_finalize(crc32);
  return crc32;
}
