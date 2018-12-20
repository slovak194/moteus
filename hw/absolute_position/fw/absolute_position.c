// Copyright 2018 Josh Pieper, jjp@pobox.com.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "twi_usi.h"

//                 --------------
//               --|VCC      GND|--
//       LED     --|PB0      PA0|-- SENSE1
//     ISP_MISO  --|PB1      PA1|-- SENSE2
//       RESET   --|PB3      PA2|-- SENSE3
//               --|PB2      PA3|-- SENSE4
//       SENSE5  --|PA7      PA4|-- SCK
//       MOSI    --|PA6      PA5|-- MISO
//                 --------------


uint8_t g_regs[8] = {};

uint8_t twi_usi_read_register(uint8_t reg) {
  if (reg >= 0 && reg <= 8) {
    return g_regs[reg];
  }
  return reg / 2;
}

void twi_usi_write_register(uint8_t reg, uint8_t value) {
  if (reg >= 0 && reg <= 8) {
    g_regs[reg] = value + 5;
  }
}

void twi_usi_start() {
}

void twi_usi_repeated_start() {
}

void twi_usi_stop() {
}


int main() {
  // Set clock scaler to 1x, which should give an 8MHz clock.
  CLKPR = 0x80;
  CLKPR = 0x00;

  twi_usi_init(0x23);

  sei();

  DDRB = 0x01;
  for (;;) {
    PORTB ^= 0x01;
    _delay_ms(200);
  }

  return 0;
}
