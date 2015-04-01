#include "xServerInclude.h"

#if defined(PLATFORM_WIN)
static int64_t frequency;
#endif

#define RECENT_TIMER_BITS 8
#define RECENT_TIMER_COUNT (1 << RECENT_TIMER_BITS)
#define RECENT_TIMER_MASK (RECENT_TIMER_COUNT - 1)
#define FURTHER_TIMER_GROUP_COUNT 4
#define FURTHER_TIMER_GROUPER_BITS ((32 - RECENT_TIMER_BITS) / FURTHER_TIMER_GROUP_COUNT)
#define FURTHER_TIMER_COUNT (1 << FURTHER_TIMER_GROUPER_BITS)
#define FURTHER_TIMER_MASK (FURTHER_TIMER_COUNT - 1)

struct x_recent_timer_node {
    int32_t id;
	struct x_recent_timer_node *next;
};

struct x_recent_timer_list {
	struct x_recent_timer_node head;
	struct x_recent_timer_node *tail;
};

struct x_further_timer_node {
	uint32_t expire_tick;
    int32_t id;
	struct x_further_timer_node *next;
};

struct x_further_timer_list {
	struct x_further_timer_node head;
	struct x_further_timer_node *tail;
};

struct x_timer {
	struct x_recent_timer_list recent[RECENT_TIMER_COUNT];
	struct x_further_timer_list further[FURTHER_TIMER_GROUP_COUNT][FURTHER_TIMER_COUNT];
	uint32_t expired_tick;
	uint64_t now;
	int32_t lock;
};

static struct x_timer *T = NULL;

static uint64_t get_time() {
    uint64_t t;
#if defined(PLATFORM_WIN)
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	t = time.QuadPart;
	t *= 100;
	t /= frequency;
#elif defined(PLATFORM_LINUX)
#ifdef CLOCK_MONOTONIC_RAW
#define CLOCK_TIMER CLOCK_MONOTONIC_RAW
#else
#define CLOCK_TIMER CLOCK_MONOTONIC
#endif
    struct timespec time;
    clock_gettime(CLOCK_TIMER, &time);
    t = (uint64_t)time.tv_sec * 100;
    t += time.tv_nsec / 10000000;
#elif defined(PLATFORM_OSX)
    struct timeval time;
    gettimeofday(&time, NULL);
    t = (uint64_t)time.tv_sec * 100;
    t += time.tv_usec / 10000;
#endif
	return t;
}

static void recent_timer_append(struct x_recent_timer_node *node, uint32_t tick) {
    uint32_t index = tick & RECENT_TIMER_MASK;
    T->recent[index].tail->next = node;
    T->recent[index].tail = node;
}

static void further_timer_append(int32_t group, struct x_further_timer_node *node) {
    uint32_t index = (node->expire_tick >> (RECENT_TIMER_BITS + group * FURTHER_TIMER_GROUPER_BITS)) & FURTHER_TIMER_MASK;
    T->further[group][index].tail->next = node;
    T->further[group][index].tail = node;
}

static void futher_timer_list_dispatch(int32_t group, int32_t index) {
	struct x_further_timer_node *node = T->further[group][index].head.next;
	while (node) {
		struct x_further_timer_node *temp = node;
		struct x_recent_timer_node *recent = x_malloc(sizeof(struct x_recent_timer_node));
        recent->id = node->id;
		recent->next = NULL;
		recent_timer_append(recent, node->expire_tick);
		node = node->next;
		x_free(temp);
	}
	T->further[group][index].head.next = NULL;
	T->further[group][index].tail = &T->further[group][index].head;
}

static void recent_timer_dispatch() {
    uint32_t index = T->expired_tick & RECENT_TIMER_MASK;
	struct x_recent_timer_node *node = T->recent[index].head.next;
    while (node) {
		struct x_recent_timer_node *temp = node;
        x_server_internal_message(INVALID_INSTANCE, message_type_timer, node->id, NULL, 0);
        node = node->next;
        x_free(temp);
    }
    T->recent[index].head.next = NULL;
    T->recent[index].tail = &T->recent[index].head;
}

static void further_timer_dispatch() {
    if (T->expired_tick == 0) {
		futher_timer_list_dispatch(FURTHER_TIMER_GROUP_COUNT - 1, 0);
	} else {
		uint32_t tick = T->expired_tick >> RECENT_TIMER_BITS;
		uint32_t mask = RECENT_TIMER_COUNT;
		int32_t group = 0;
		while ((T->expired_tick & (mask - 1)) == 0) {
			uint32_t index = tick & FURTHER_TIMER_MASK;
			if (index > 0) {
				futher_timer_list_dispatch(group, index);
				break;
			}
			group++;
			tick >>= FURTHER_TIMER_GROUPER_BITS;
			mask <<= FURTHER_TIMER_GROUPER_BITS;
		}
	}
}

static void timer_loop() {
	spin_lock_lock(&T->lock);
    T->expired_tick++;
    further_timer_dispatch();
    recent_timer_dispatch();
	spin_lock_unlock(&T->lock);
}

void global_timer_init() {
	if (T == NULL) {
#if defined(PLATFORM_WIN)
		LARGE_INTEGER time;
		QueryPerformanceFrequency(&time);
		frequency = time.QuadPart;
#endif
		T = x_malloc(sizeof(struct x_timer));
		memset(T, 0, sizeof(struct x_timer));
		T->now = get_time();
        int32_t i;
        for (i = 0; i < RECENT_TIMER_COUNT; i++) {
            T->recent[i].head.next = NULL;
            T->recent[i].tail = &T->recent[i].head;
        }
        for (i = 0; i < FURTHER_TIMER_GROUP_COUNT; i++) {
            for (int j = 0; j < FURTHER_TIMER_COUNT; j++) {
                T->further[i][j].head.next = NULL;
                T->further[i][j].tail = &T->further[i][j].head;
            }
        }
	}
}

void global_timer_update() {
    uint64_t now = get_time();
    if (now > T->now) {
        uint32_t tick = (uint32_t)(now - T->now);
        T->now = now;
        for (uint32_t i = 0; i < tick; i++) {
            timer_loop();
        }
    } else if (now < T->now) {
		x_log("timer error, time changed from %"PRIu64" to %"PRIu64".", T->now, now);
    }
}

void global_timer_register(int32_t id, uint32_t tick) {
    if (tick == 0) {
        x_server_internal_message(INVALID_INSTANCE, message_type_timer, id, NULL, 0);
    } else {
        spin_lock_lock(&T->lock);
        uint32_t expired_tick = T->expired_tick + tick;
        if ((expired_tick | RECENT_TIMER_MASK) == (T->expired_tick | RECENT_TIMER_MASK)) {
            struct x_recent_timer_node *recent_node = x_malloc(sizeof(struct x_recent_timer_node));
            recent_node->id = id;
            recent_node->next = NULL;
            recent_timer_append(recent_node, expired_tick);
        } else {
            uint32_t mask = RECENT_TIMER_COUNT << FURTHER_TIMER_GROUPER_BITS;
            for (int32_t i = 0; i < FURTHER_TIMER_GROUP_COUNT; i++) {
                if ((expired_tick | (mask -1)) == (T->expired_tick | (mask - 1))) {
                    struct x_further_timer_node *further_node = x_malloc(sizeof(struct x_further_timer_node));
                    further_node->expire_tick = expired_tick;
                    further_node->id = id;
                    further_node->next = NULL;
                    further_timer_append(i, further_node);
                    break;
                }
                mask <<= FURTHER_TIMER_GROUPER_BITS;
            }
        }
        spin_lock_unlock(&T->lock);
    }
}

uint32_t global_timer_now() {
	return T->expired_tick;
}