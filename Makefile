MAIN = main
SRC = usbdrv/oddebug.c usbdrv/usbdrv.c $(MAIN).c
ASMSRC = usbdrv/usbdrvasm.S
CFLAGS = -Iusbdrv -I. -DF_CPU=20000000 -Os -mmcu=attiny24a -Wall -Wextra -std=gnu99 -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions

OBJ = $(patsubst %.c,%.o,$(SRC))
ASMOBJ = $(patsubst %.S,%.o,$(ASMSRC))
VPATH = usbdrv

AVRDUDE_MCU = t24
AVRDUDE_PROG = usbasp

HFUSE = 0xdf
LFUSE = 0xef

.PHONY = all clean

all: $(MAIN).hex

$(MAIN).hex: $(MAIN).elf
	avr-objcopy -O ihex $< $@

$(MAIN).elf: $(OBJ) $(ASMOBJ)
	avr-gcc $(CFLAGS) $^ -o $@

%.o: %.c
	avr-gcc $(CFLAGS) -c $^ -o $@

%.o: %.S
	avr-gcc $(CFLAGS) -c $^ -o $@

flash: $(MAIN).hex
	avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROG) -eU flash:w:$<

fuses:
	avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROG) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m

clean:
	rm -f *.o $(VPATH)/*.o $(MAIN).elf $(MAIN).hex
