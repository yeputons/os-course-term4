#include "ioport.h"
#include "serial.h"

#define BASE_PORT 0x3F8
#define DATA_PORT (BASE_PORT + 0)
#define INT_PORT (BASE_PORT + 1)

#define SPD_LOW_PORT (BASE_PORT + 0)
#define SPD_HIGH_PORT (BASE_PORT + 1)

#define LCR_PORT (BASE_PORT + 3)
#define STATUS_PORT (BASE_PORT + 5)

#define DLAB (1 << 7)

// Serial port config: 8 bit, no stop bit, no parity check
#define SERIAL_CONTROL 3
#define SERIAL_DIVISOR_LATCH 0x000C // 9600 bps

#define TRANSMIT_OVER_BIT (1 << 5)

void init_serial(void) {
    out8(LCR_PORT, DLAB | SERIAL_CONTROL);
    out8(SPD_LOW_PORT, SERIAL_DIVISOR_LATCH & 0xFF);
    out8(SPD_HIGH_PORT, (SERIAL_DIVISOR_LATCH >> 8) & 0xFF);
    out8(LCR_PORT, SERIAL_CONTROL);
    out8(INT_PORT, 0); // disable interruptions
}

void putchar(int c) {
    out8(DATA_PORT, c);
    while (!(in8(STATUS_PORT) & TRANSMIT_OVER_BIT));
}

void puts(char *s) {
    for (int i = 0; s[i]; i++) {
        putchar(s[i]);
    }
    putchar('\n');
}

