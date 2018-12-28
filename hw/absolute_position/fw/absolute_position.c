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

#include <avr/cpufunc.h>
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


#define NUM_REGISTERS 16

// We store the register data in two places, so we can swap in the new
// values while having interrupts disabled for the minimum amount of
// time.
volatile uint8_t g_registers1[NUM_REGISTERS] = {};
volatile uint8_t g_registers2[NUM_REGISTERS] = {};

volatile uint8_t* volatile g_current_registers;
volatile uint8_t* volatile g_next_registers;

volatile uint8_t g_sampled_registers[NUM_REGISTERS] = {};

uint8_t twi_usi_read_register(uint8_t reg) {
  if (reg > NUM_REGISTERS) { return 0; }
  return g_sampled_registers[reg];
}

void twi_usi_write_register(uint8_t reg, uint8_t value) {
  // We current support no writable registers.
}

void twi_usi_start() {
  for (uint8_t i = 0; i < NUM_REGISTERS; i++) {
    g_sampled_registers[i] = g_current_registers[i];
  }
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

  g_current_registers = &g_registers1[0];
  g_next_registers = &g_registers2[0];

  uint16_t current_adc[5] = {};
  uint16_t count = 0;

  // NOTE: I haven't figured out why initializing this array inline
  // results in gcc failing to get it properly initialized at runtime
  // yet.  Thus we just fill it in by hand.
  uint8_t adc_channel[5];
  adc_channel[0] = 0;  // PA0
  adc_channel[1] = 1;  // PA1;
  adc_channel[2] = 2;  // PA2;
  adc_channel[3] = 3;  // PA3;
  adc_channel[4] = 7;  // PA7;

  uint8_t prescaler = 0x06;  // 64x, 8000000 / 64 = 125000

  // Disable digital inputs on our sense lines.
  DIDR0 =
      (1 << ADC7D) |
      (1 << ADC3D) |
      (1 << ADC2D) |
      (1 << ADC1D) |
      (1 << ADC0D);

  PRR &= ~(1 << PRADC);

  // Enable the ADC.
  ADCSRA =
      (1 << ADEN) |
      (prescaler << ADPS0);

  // Set our LED for output.
  DDRB = 0x01;

  for (;;) {
    count++;

    PORTB = (count >> 8) & 0x01;;

    // Read all sense channels.
    for (uint8_t i = 0; i < 5; i++) {
      // Configure the multiplexer.
      ADMUX =
          (0 << REFS1) | (0 << REFS0) | // Vcc as reference
          adc_channel[i];

      for (uint8_t j = 0; j < 3; j++) {
        // Start a conversion and make sure our completion flag is
        // cleared.
        ADCSRA |= (1 << ADSC);
        while ((ADCSRA & (1 << ADIF)) == 0);

        (void)ADC;
      }

      // Read the result.
      current_adc[i] = ADC;

      ADCSRA |= (1 << ADIF);
    }

    // TODO(jpieper): Calculate output angle.

    // Store the results in the shadow registers.
    g_next_registers[2] = count >> 8;
    g_next_registers[3] = count & 0xff;

    for (uint8_t i = 0; i < 5; i++) {
      g_next_registers[4 + i * 2] = 0x80 | ((current_adc[i] >> 8) & 0xff);
      g_next_registers[4 + i * 2 + 1] = current_adc[i] & 0xff;
    }

    // Swap in the new register set.
    {
      volatile uint8_t* tmp = g_current_registers;
      g_current_registers = g_next_registers;
      g_next_registers = tmp;
    }
  }

  return 0;
}
