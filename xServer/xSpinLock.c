#include "xServerInclude.h"

int32_t spin_lock_try_lock(int32_t *lock) {
#if defined(PLATFORM_WIN)
	return _InterlockedExchange(lock, 1);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
	return __sync_lock_test_and_set(lock, 1);
#endif
}

void spin_lock_lock(int32_t *lock) {
	while (spin_lock_try_lock(lock)) {}
}

void spin_lock_unlock(int32_t *lock) {
#if defined(PLATFORM_WIN)
	_InterlockedExchange(lock, 0);
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
	__sync_lock_release(lock);
#endif
}

void spin_lock_synchronize() {
#if defined(PLATFORM_WIN)
	_ReadWriteBarrier();
#elif defined(PLATFORM_OSX) || defined(PLATFORM_LINUX)
	__sync_synchronize();
#endif
}