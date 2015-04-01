#ifndef __X_SPIN_LOCK_H__
#define __X_SPIN_LOCK_H__

#include "xServerInclude.h"

int32_t spin_lock_try_lock(int32_t *lock);

void spin_lock_lock(int32_t *lock);

void spin_lock_unlock(int32_t *lock);

void spin_lock_synchronize();

#endif