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

#pragma once

#include <stdint.h>

/// Call this before interrupts are enabled to initialize the module.
void twi_usi_init(uint8_t slave_address);

//////////////////////////////////////////////////////////////////////
/// Extension points.
///
/// The following functions are the extension points of this module.
/// The calling code must define them.  All are invoked from interrupt
/// context.

/// This is called when the master has requested to read a given
/// register.  It must return a single value.
uint8_t twi_usi_read_register(uint8_t);

/// This is called when the master has requested to write a register.
void twi_usi_write_register(uint8_t reg, uint8_t value);

/// This is called when a TWI "start" condition is observed addressed
/// to this slave.
void twi_usi_start();

/// This is called when a TWI "repeated" start condition is observed
/// addressed to this slave.
void twi_usi_repeated_start();

/// This is called at the conclusion of a TWI cycle (including ones
/// not addressed to this slave).  Note: The current implementation
/// may delay calling this until immediately before the next start
/// cycle, so you should not rely on it being invoked in a timely
/// manner.
void twi_usi_stop();
