# Copyright 2019 Josh Pieper, jjp@pobox.com.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

PROG=precharge
OBJS=

GCC_PART=__AVR_ATtiny44A__
AVR_PART=t44

CFLAGS= \
  -g \
  -mmcu=attiny44a \
  -D$(GCC_PART) \
  -Os \
  -Winline \
  -Wall \
  -flto \
  -fshort-enums \
  -DF_CPU=1000000

LINKFLAGS =
#  -Wl,--gc-sections

all: $(PROG).srec

clean:
	rm -f *.o *.out *.srec *.hex

$(PROG).srec : $(PROG).out
	avr-objcopy -j .text -O srec $(PROG).out $(PROG).srec

$(PROG).hex : $(PROG).out
	avr-objcopy -j .text -O ihex $(PROG).out $(PROG).hex

$(PROG).out : $(PROG).o $(OBJS)
	avr-gcc $(CFLAGS) $(LINKFLAGS) $(PROG).o $(OBJS) -o $(PROG).out

%.o : %.c
	avr-gcc -c $(CFLAGS) $< -o $@


flash : $(PROG).srec
	avrdude -p $(AVR_PART) -c avrispmkII -U flash:w:$(PROG).srec

flash_dw : $(PROG).srec
	avrdude -p $(AVR_PART) -c dragon_dw -B 50kHz -U flash:w:$(PROG).srec
