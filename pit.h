#ifndef _PIT_H
#define _PIT_H

#include "pic.h"

#define PIT_BASE_FREQ 1193180
#define PIT_INTERRUPT (LPIC_MASTER_ICW + 0)
void init_pit(uint16_t divisor);

#endif
