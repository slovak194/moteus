// Copyright 2018 Josh Pieper, jjp@pobox.com.
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

/// @file
///
/// This file was developed referencing the AVR312 app note, with
/// substantial modifications, including making it work.

#include "twi_usi.h"

#include <avr/interrupt.h>
#include <avr/io.h>

// Device specific defines.

#if defined(__AVR_ATtiny26__)
    #define DDR_USI             DDRB
    #define PORT_USI            PORTB
    #define PIN_USI             PINB
    #define PORT_USI_SDA        PORTB0
    #define PORT_USI_SCL        PORTB2
    #define PIN_USI_SDA         PINB0
    #define PIN_USI_SCL         PINB2
    #define USI_START_COND_INT  USISIF
    #define READ_USIDR          USIDR
    #define USI_START_VECTOR    USI_STRT_vect
    #define USI_OVERFLOW_VECTOR USI_OVF_vect
#elif defined(__AVR_ATtiny2313__)
    #define DDR_USI             DDRB
    #define PORT_USI            PORTB
    #define PIN_USI             PINB
    #define PORT_USI_SDA        PORTB5
    #define PORT_USI_SCL        PORTB7
    #define PIN_USI_SDA         PINB5
    #define PIN_USI_SCL         PINB7
    #define USI_START_COND_INT  USISIF
    #define READ_USIDR          USIDR
    #define USI_START_VECTOR    USI_STRT_vect
    #define USI_OVERFLOW_VECTOR USI_OVF_vect
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || \
  defined(__AVR_ATtiny85__)
    #define DDR_USI             DDRB
    #define PORT_USI            PORTB
    #define PIN_USI             PINB
    #define PORT_USI_SDA        PORTB0
    #define PORT_USI_SCL        PORTB2
    #define PIN_USI_SDA         PINB0
    #define PIN_USI_SCL         PINB2
    #define USI_START_COND_INT  USICIF
    #define READ_USIDR          USIDR
    #define USI_START_VECTOR    USI_START_vect
    #define USI_OVERFLOW_VECTOR USI_OVF_vect
#elif defined(__AVR_ATmega165__) | \
  defined(__AVR_ATmega325__) | defined(__AVR_ATmega3250__) |   \
  defined(__AVR_ATmega645__) | defined(__AVR_ATmega6450__) |       \
  defined(__AVR_ATmega329__) | defined(__AVR_ATmega3290__) |       \
  defined(__AVR_ATmega649__) | defined(__AVR_ATmega6490__)
    #define DDR_USI             DDRE
    #define PORT_USI            PORTE
    #define PIN_USI             PINE
    #define PORT_USI_SDA        PORTE5
    #define PORT_USI_SCL        PORTE4
    #define PIN_USI_SDA         PINE5
    #define PIN_USI_SCL         PINE4
    #define USI_START_COND_INT  USISIF
    #define READ_USIDR          USIDR
    #define USI_START_VECTOR    USI_START_vect
    #define USI_OVERFLOW_VECTOR USI_OVERFLOW_vect
#elif (__AVR_ATmega169__)
    #define DDR_USI             DDRE
    #define PORT_USI            PORTE
    #define PIN_USI             PINE
    #define PORT_USI_SDA        PORTE5
    #define PORT_USI_SCL        PORTE4
    #define PIN_USI_SDA         PINE5
    #define PIN_USI_SCL         PINE4
    #define USI_START_COND_INT  USISIF
    #define READ_USIDR          USIDR
    #define USI_START_VECTOR    USI_STRT_vect
    #define USI_OVERFLOW_VECTOR USI_OVF_vect
#elif defined(__AVR_ATtiny44A__)
    #define DDR_USI DDRA
    #define PORT_USI PORTA
    #define PIN_USI PINA
    #define PORT_USI_SDA PA6
    #define PORT_USI_SCL PA4
    #define PIN_USI_SDA PINA6
    #define PIN_USI_SCL PINA4
    #define USI_START_COND_INIT USISIF
   // NOTE: You might think you could use USIBR here, however its
   // behavior appears to be interesting and it isn't updated when I
   // expect it to be.  Initial experiments showed it receiving data
   // shifted left one extra time versus just reading USIDR.
    #define READ_USIDR          USIDR
    #define USI_START_VECTOR USI_STR_vect
    #define USI_OVERFLOW_VECTOR USI_OVF_vect
#else
    #error "Unknown device"
#endif

typedef enum {
  kCheckAddress,
  kSendData,
  kRequestReplyFromSendData,
  kCheckReplyFromSendData,
  kRequestData,
  kGetDataAndSendAck,
} State;

_Static_assert(sizeof(State) == 1, "-fshort-enums should be used");

enum {
  kDefaultUsicrConfig = (
      // Operate in TWI mode, without an overflow clock hold.
      (1 << USIWM1) | (0 << USIWM0) |
      // Shift register clock source = external, positive edge
      (1 << USICS1) | (0 << USICS0) | (0 << USICLK) |
      // Don't toggle clock port pin.
      (0 << USITC)),
};

static volatile uint8_t g_slave_address;
static volatile State g_state;
static volatile uint8_t g_current_register;
static volatile uint8_t g_write_byte_count;

static inline void clear_interrupt_flags_except_start(uint8_t count) {
  USISR =
      (0 << USI_START_COND_INIT) |
      (1 << USIOIF) |
      (1 << USIPF) |
      (1 << USIDC) |
      (count << USICNT0)
      ;
}

static inline void set_usi_to_send_ack() {
  // Prepare ACK.
  USIDR = 0;

  // Set SDA as output.
  DDR_USI |= (1 << PORT_USI_SDA);

  // Shift out 1 bit of ack.
  clear_interrupt_flags_except_start(0x0e);
}

static inline void set_usi_to_read_ack() {
  // Set SDA as input.
  DDR_USI &= ~(1 << PORT_USI_SDA);

  // Prepare ACK.
  USIDR = 0;

  // Shift in 1 bit of ack.
  clear_interrupt_flags_except_start(0x0e);
}

static inline void set_usi_to_twi_start_condition_mode() {
  USICR =
      // Enable start condition interrupts.  Disable overflow interrupt.
      (1 << USISIE) | (0 << USIOIE) |
      kDefaultUsicrConfig
      ;

  clear_interrupt_flags_except_start(0x00);
}

static inline void set_usi_to_send_data() {
  // Set SDA as output.
  DDR_USI |= (1 << PORT_USI_SDA);
  clear_interrupt_flags_except_start(0x00);
}

static inline void set_usi_to_read_data() {
  // Set SDA as input.
  DDR_USI &= ~(1 << PORT_USI_SDA);

  clear_interrupt_flags_except_start(0x00);
}

void twi_usi_init(uint8_t slave_address) {
  g_slave_address = slave_address;
  g_current_register = 0x00;
  g_write_byte_count = 0;

  // Set SDA and SCL high.
  PORT_USI |=
      (1 << PORT_USI_SCL) |
      (1 << PORT_USI_SDA);

  // Configure SCL as output and SDA as input.
  DDR_USI |= (1 << PORT_USI_SCL);
  DDR_USI &= ~(1 << PORT_USI_SDA);

  USICR =
      // Enable start condition interrupt.  Disable overflow interrupt.
      (1 << USISIE) | (0 << USIOIE) |
      kDefaultUsicrConfig;

  // Clear all flags and reset overflow counter.
  USISR =
      (1 << USI_START_COND_INIT) |
      (1 << USIOIF) |
      (1 << USIPF) |
      (1 << USIDC);
}

ISR(USI_START_VECTOR) {
  g_state = kCheckAddress;

  if (USISR & (1 << USIPF)) {
    // We previously have seen a stop condition.
    g_current_register = 0x00;
    g_write_byte_count = 0;
    USISR = (1 << USIPF);

    twi_usi_stop();
  } else {
    twi_usi_repeated_start();
  }

  // Set SDA as an input.
  DDR_USI &= ~(1 << PORT_USI_SDA);

  // Wait for SCL to go low to ensure that the start condition has
  // completed.  However, also bail early if a stop condition is
  // detected.

  uint8_t start_received = 0;
  while (1) {
    if (!(PIN_USI & (1 << PIN_USI_SCL))) {
      start_received = 1;
      break;
    }
    if ((PIN_USI & (1 << PIN_USI_SDA)) ||
        (USISR & (1 << USIPF))) {
      break;
    }
  }

  if (start_received) {
    // We actually received a start condition.
    USICR =
        // Enable Overflow and start condition interrupt.  (We might
        // get a repeated start, so need to keep looking for start
        // conditions).
        (1 << USISIE) | (1 << USIOIE) |
        // But now that we are receiving data, we do want to hold the
        // clock low until we have a chance to read out the data.
        (1 << USIWM0) |
        kDefaultUsicrConfig;
  } else  {
    // Oops, we accidentally got a stop condition.  Resume looking for
    // another start condition.
    USICR =
        (1 << USISIE) | (0 << USIOIE) |
        kDefaultUsicrConfig;
    USISR = (1 << USIPF);
  }

  // Now clear out all the flags.
  USISR =
      (1 << USI_START_COND_INIT) |
      (1 << USIOIF) |
      (1 << USIPF) |
      (1 << USIDC) |
      (0 << USICNT0);
}

ISR(USI_OVERFLOW_VECTOR) {
  uint8_t current_usidr = READ_USIDR;

  switch (g_state) {
    case kCheckAddress: {
      // We will respond to our assigned address, or the "broadcast"
      // address of 0.
      if ((current_usidr == 0) ||
          ((current_usidr >> 1) == g_slave_address)) {
        g_state = (current_usidr & 0x01) ?
            kSendData : kRequestData;
        set_usi_to_send_ack();
        twi_usi_start();
      } else {
        // Nope, this wasn't addressed to us.  Go back and wait for
        // another start condition.
        set_usi_to_twi_start_condition_mode();
      }
      break;
    }

    case kCheckReplyFromSendData: {
      if (current_usidr) {
        // We got a NACK, so the master does not want more data.
        set_usi_to_twi_start_condition_mode();
        return;
      }
      // Fall through to kSendData.
    }
    case kSendData: {
      USIDR = twi_usi_read_register(g_current_register);
      g_current_register++;
      g_state = kRequestReplyFromSendData;
      set_usi_to_send_data();
      break;
    }

    case kRequestReplyFromSendData: {
      // Set USI to sample reply from master.
      g_state = kCheckReplyFromSendData;
      set_usi_to_read_ack();
      break;
    }

    case kRequestData: {
      // Set USI to sample data from master.
      g_state = kGetDataAndSendAck;
      set_usi_to_read_data();
      break;
    }

    case kGetDataAndSendAck: {
      // Copy data from USIDR and send ack.
      if (g_write_byte_count == 0) {
        g_current_register = current_usidr;
      } else {
        twi_usi_write_register(g_current_register, current_usidr);
        g_current_register++;
      }

      g_write_byte_count++;
      g_state = kRequestData;
      set_usi_to_send_ack();
      break;
    }
  }
}
