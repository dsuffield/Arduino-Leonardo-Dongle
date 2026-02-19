#
# makefile for rtusb
#
# History:
#
# Version 0.4,   des 04/09/2010   Atmel ATmega32U4 USB HID example enumerates with SN.
# Version 0.5,   des 04/10/2020   Arduino Leonardo (ATmega32U4) initial converstion to rtstepper enumeration.
# Version 0.6,   des 04/12/2020   Added Microsoft/Winusb enumeration descriptors.
# Version 0.12,  des 04/14/2020   EP0 IN/OUT xfrs are convoluted, not finish...
# Version 0.13,  des 04/20/2020   Abandoned Atmel USB HID example framework. Rewrote using PIC16f USB framework. Compiles.
# Version 0.17,  des 04/22/2020   Converted old PIC16f framework to ATmega32U4 enumerates ok.
# Version 0.19,  des 04/24/2020   STEP_QUERY works reliably now. No missing data reply.
# Version 0.20,  des 04/28/2020   rt-test works, but only uses 64 byte banks.
# Version 0.21,  des 04/28/2020   Arduino Leonardo (ATmega32U4) works ok, but only with 64 byte ping-pong not 256.
# Version 0.23,  des 05/14/2020   Added AVR serial number support plus digital INPUT3 and OUTPUT2 support.
# Version 0.24,  des 05/17/2020   Added support for analog to digital converstion (ADC).
# Version 0.25,  des 05/22/2020   Fixed Empty bit and PWM output1.
# Version 0.26,  des 05/24/2020   ADC INPUT1-3 now work ok.
# Version 0.27,  des 05/26/2020   Fixed intermittent Set Configuration issue and mSetDir/mSetStep macros.
# Version 0.28,  des 06/03/2020   Fixed INPUT0 rpm debounce, fixed Sherline CNC controller 5v surge hang.
# Version 1.29,  des 07/11/2020   First public release.
# Version 1.30,  des 05/11/2022   Freebie release.

# Project name
PROJECT := rtusb_leonardo
VERSION := 1.30
PORT := /dev/ttyACM0

MCU = atmega32u4

# Source files
CSRCS = \
  main.c\
  usbctrltrf.c\
  usbdrv.c\
  usb9.c\
  usbdsc.c\
  hash.c\

HSRCS = \
  usbdrv.h\
  usbctrltrf.h\
  usbdrv.h\
  usbcfg.h\
  usb9.h\
  typedefs.h\
  usb.h\
  usbdsc.h\

OUTPUT = obj

# General Flags
TARGET = $(PROJECT).elf
CC = avr-gcc

# Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

# Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -Os -fsigned-char -ffunction-sections
CFLAGS += -MD -MP -MT $(OUTPUT)/$(*F).o -MF $(OUTPUT)/dep/$(@F).d 

# Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

# Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-Map=$(PROJECT).map,--cref,--gc-sections,--relax

# Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom

# Eeprom file production flags
HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0

# Include Directories
#INCLUDES = -I"./.." -I"../conf" -I"../../.." -I"../../../../at90usb128" -I"../../../../common" 

## Build
all: $(TARGET) $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss size

## Clean target
clean:
	rm -rf $(OUTPUT)/dep/* $(OUTPUT)/* $(PROJECT).elf $(PROJECT).hex $(PROJECT).eep $(PROJECT).map

## Compile

# Create objects files list with sources files
OBJECTS  = $(CSRCS:.c=.o)

# create object files from C source files.
%.o: %.c $(HSRCS) makefile
	@echo 'Building file: $<'
	@$(shell mkdir $(OUTPUT) 2>/dev/null)
	@$(shell mkdir $(OUTPUT)/dep 2>/dev/null)
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $(OUTPUT)/$(@F)
	
## Link
$(TARGET): $(OBJECTS)
	@echo "Linking"
	$(CC) $(LDFLAGS) $(addprefix $(OUTPUT)/,$(notdir $(OBJECTS))) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	@echo "Create hex file"
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	@echo "Create eep file"
	avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@

%.lss: $(TARGET)
	@echo "Create lss file"
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@avr-size -C --mcu=${MCU} ${TARGET}

dist:
	rm -fr $(PROJECT)-$(VERSION)
	mkdir $(PROJECT)-$(VERSION)
	cp $(CSRCS) $(HSRCS) makefile $(PROJECT)-$(VERSION)
	tar czvf $(PROJECT)-$(VERSION).tar.gz $(PROJECT)-$(VERSION)
	rm -r $(PROJECT)-$(VERSION)
#
# Notes on making a .hex file using customer's board ID.
#
#   1. Generate 8-bit hash key.
#        ./hash_gen.py -s 48313336353715130110
#   2. Create a new PID in serial.txt.
#   2. Modify usbdsc.c with generated hash (hex) and PID.
#   3. Generate the package.
#        make dist_hex CPUID=48313336353715130110
dist_hex: all
	rm -fr $(PROJECT)-$(VERSION)-$(CPUID)
	rm -fr $(PROJECT)-$(VERSION)-$(CPUID).zip
	mkdir $(PROJECT)-$(VERSION)-$(CPUID)
	cp $(PROJECT).hex $(PROJECT)-$(VERSION)-$(CPUID)
	md5sum $(PROJECT).hex > $(PROJECT).md5
	cp $(PROJECT).md5 $(PROJECT)-$(VERSION)-$(CPUID)
	zip -r $(PROJECT)-$(VERSION)-$(CPUID).zip $(PROJECT)-$(VERSION)-$(CPUID)
	rm -fr $(PROJECT)-$(VERSION)-$(CPUID)

# Zip up current bulk order. Assumes old zip files have been moved to bak_zip directory. 
dist_all_zip:
	rm -fr $(PROJECT)-$(VERSION)-pack.zip
	zip $(PROJECT)-$(VERSION)-pack.zip *.zip

flash:
	avrdude -p $(MCU) -c usbasp -u -U flash:w:$(PROJECT).hex

eeprom:
	avrdude -p $(MCU) -c usbasp -u -U eeprom:w:$(PROJECT).eep

fuse:
	avrdude -p $(MCU) -c usbasp -u -U lfuse:w:0xff:m -U hfuse:w:0xd8:m -U efuse:w:0xcb:m

flash_serial:
	@./reset.py $(PORT)
	@sleep 1
	avrdude -v -p $(MCU) -c avr109 -P $(PORT) -b57600 -D -U flash:w:$(PROJECT).hex

flash_serial_dump:
	@./reset.py $(PORT)
	@sleep 1
	avrdude -v -p $(MCU) -c avr109 -P $(PORT) -b57600 -D -U flash:r:bot.hex:i

flash_dump:
	avrdude -p $(MCU) -c usbasp -u -U flash:r:leonardo_flash_dump.hex:i

eeprom_dump:
	avrdude -p $(MCU) -c usbasp -u -U eeprom:r:$(PROJECT)_eeprom_dump.hex:i
	cat $(PROJECT)_eeprom_dump.hex

fuse_dump:
	echo -n "lfuse: " > fuse_dump.txt
	avrdude -p $(MCU) -c usbasp -u -U lfuse:r:-:h >> fuse_dump.txt
	echo -n "hfuse: " >> fuse_dump.txt
	avrdude -p $(MCU) -c usbasp -u -U hfuse:r:-:h >> fuse_dump.txt
	echo -n "efuse: " >> fuse_dump.txt
	avrdude -p $(MCU) -c usbasp -u -U efuse:r:-:h >> fuse_dump.txt
	echo -n "lock: " >> fuse_dump.txt
	avrdude -p $(MCU) -c usbasp -u -U lock:r:-:h >> fuse_dump.txt
	cat fuse_dump.txt

mem_display:
	avrdude -p $(MCU) -c usbasp -u -v

.PHONY: clean flash dist eeprom eeprom_dump mem_display fuse fuse_dump
