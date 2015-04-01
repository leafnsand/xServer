#ifndef __X_LOCK_H__
#define __X_LOCK_H__

#include "xServerInclude.h"

#if defined(PLATFORM_WIN) && !defined(__GNUC__)

struct x_lock {
	long write_lock;
	long read_lock;
};

static X_INLINE long lock_atomic_add(long *lock) {
	return _InterlockedIncrement(lock);
}

static X_INLINE long lock_atomic_sub(long *lock) {
	return _InterlockedDecrement(lock);
}

static X_INLINE void lock_init(struct x_lock *lock) {
	lock->write_lock = 0;
	lock->read_lock = 0;
}

static X_INLINE void lock_read_lock(struct x_lock *lock) {
	for (;;) {
		while (lock->write_lock) {
			_ReadWriteBarrier();
		}
		lock_atomic_add(&lock->read_lock);
		if (lock->write_lock) {
			lock_atomic_sub(&lock->read_lock);
		} else {
			break;
		}
	}
}

static X_INLINE void lock_write_lock(struct x_lock *lock) {
	while (_InterlockedExchange(&lock->write_lock, 1)) {
	}
	while (lock->read_lock) {
		_ReadWriteBarrier();
	}
}

static X_INLINE void lock_write_unlock(struct x_lock *lock) {
	_InterlockedExchange(&lock->write_lock, 0);
}

static X_INLINE void lock_read_unlock(struct x_lock *lock) {
	lock_atomic_sub(&lock->read_lock);
}

#else

struct x_lock {
	int32_t write_lock;
	int32_t read_lock;
};

static X_INLINE int32_t lock_atomic_add(int32_t *lock) {
	return __sync_add_and_fetch(lock, 1);
}

static X_INLINE int32_t lock_atomic_sub(int32_t *lock) {
	return __sync_sub_and_fetch(lock, 1);
}

static X_INLINE void lock_init(struct x_lock *lock) {
	lock->write_lock = 0;
	lock->read_lock = 0;
}

static X_INLINE void lock_read_lock(struct x_lock *lock) {
	for (;;) {
		while (lock->write_lock) {
			__sync_synchronize();
		}
		lock_atomic_add(&lock->read_lock);
		if (lock->write_lock) {
			lock_atomic_sub(&lock->read_lock);
		} else {
			break;
		}
	}
}

static X_INLINE void lock_write_lock(struct x_lock *lock) {
	while (__sync_lock_test_and_set(&lock->write_lock, 1)) {
	}
	while (lock->read_lock) {
		__sync_synchronize();
	}
}

static X_INLINE void lock_write_unlock(struct x_lock *lock) {
	__sync_lock_release(&lock->write_lock);
}

static X_INLINE void lock_read_unlock(struct x_lock *lock) {
	lock_atomic_sub(&lock->read_lock);
}

#endif

#endif