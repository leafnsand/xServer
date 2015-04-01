#ifndef __X_TIMER_H__
#define __X_TIMER_H__

#include "xServerInclude.h"

void global_timer_init();

void global_timer_update();

void global_timer_register(int32_t instance_id, uint32_t tick);

uint32_t global_timer_now();

#endif