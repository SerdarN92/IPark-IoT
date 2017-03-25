###############################################################################
# Makefile for the project iPark
###############################################################################

## General Flags
PROJECT = iPark
MCU = atxmega128a1
TARGET = iPark.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99           -DF_CPU=32000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-script=avrxmega7.x -Wl,-Map=iPark.map
LDFLAGS += -Wl,-section-start=EXTRAM=0x804000


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = main.o system_clock.o common.o systimer.o sdram.o memtransfer.o hardware.o ringBuffer.o uart.o uart_mgmt.o communication.o crc.o globals.o aes.o coap.o coap_endpoints.o eeprom.o settings.o console.o servo.o rtc.o distance.o control.o light.o accounting.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) iPark.hex iPark.eep iPark.lss## Compile
main.o: ../main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

system_clock.o: ../system_clock.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

common.o: ../common.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

systimer.o: ../systimer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

sdram.o: ../sdram.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

memtransfer.o: ../memtransfer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

hardware.o: ../hardware.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ringBuffer.o: ../ringBuffer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

uart.o: ../uart.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

uart_mgmt.o: ../uart_mgmt.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

communication.o: ../communication.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

crc.o: ../crc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

globals.o: ../globals.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

aes.o: ../aes.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

coap.o: ../coap.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

coap_endpoints.o: ../coap_endpoints.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

eeprom.o: ../eeprom.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

settings.o: ../settings.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

console.o: ../console.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

servo.o: ../servo.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

rtc.o: ../rtc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

distance.o: ../distance.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

control.o: ../control.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

light.o: ../light.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

accounting.o: ../accounting.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) iPark.elf dep/* iPark.hex iPark.eep iPark.lss iPark.map


## Other dependencies
-include $(shell mkdir dep 2>NUL) $(wildcard dep/*)

# Add additional makefile data:
-include ../makefile.add
