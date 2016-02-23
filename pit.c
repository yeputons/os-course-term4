#include "ioport.h"
#include "pit.h"

#define CONTROL_PORT 0x43
#define DATA0_PORT 0x40

void init_pit(uint16_t divisor) {
    out8(CONTROL_PORT,
          (0 << 0) // binary encoding of divisor
        | (2 << 1) // mode, Rate Generator
        | (3 << 4) // write two bytes, low and high
        | (0 << 6) // channel #0
    );
    out8(DATA0_PORT, divisor);
    out8(DATA0_PORT, divisor >> 8);
}
