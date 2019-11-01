// Copyright 2019 Josh Pieper, jjp@pobox.com.
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

#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

//                 --------------
//               --|VCC      GND|--
//    FET_MAIN   --|PB0      PA0|-- VSAMP_IN
//    FET_PCHG   --|PB1      PA1|-- VSAMP_OUT
//       RESET   --|PB3      PA2|-- LED1
//               --|PB2      PA3|-- LED2
//               --|PA7      PA4|-- SCK
//       MOSI    --|PA6      PA5|-- MISO
//                 --------------

#define LED1 0x04
#define LED2 0x08

#define FET_MAIN 0x01
#define FET_PCHG 0x02

int main() {
  // We leave the clock at the original 1MHz, because this application
  // really does not need high speed.

  // Disable digital inputs on our sense lines.
  DIDR0 =
      (1 << ADC1D) |
      (1 << ADC0D);

  // Set our LEDs for output.
  PORTA |= (LED1 | LED2);
  DDRA = (LED1 | LED2);

  PORTB = 0x00;
  DDRB = (FET_MAIN | FET_PCHG);

  _delay_ms(10);
  PORTB |= FET_PCHG;

  _delay_ms(100);
  PORTB |= FET_MAIN;

  for (;;) {
    PORTA |= LED1;
    _delay_ms(970);
    PORTA &= ~LED1;
    _delay_ms(30);
  }

  return 0;
}
