#include "xServerInclude.h"

#if defined(PLATFORM_OSX)

static struct kevent *s_events = NULL;

#elif defined(PLATFORM_LINUX)

static struct epoll_event *s_events = NULL;

#endif

static int32_t s_max;

x_event_fd event_create(int32_t max) {
	s_max = max;
#if defined(PLATFORM_WIN)
	return -1;
#elif defined(PLATFORM_OSX)
	s_events = x_malloc(max * sizeof(struct kevent));
    return kqueue();
#elif defined(PLATFORM_LINUX)
	s_events = x_malloc(max * sizeof(struct epoll_event));
	return epoll_create(max);
#endif
}

void event_destroy(x_event_fd fd) {
#if defined(PLATFORM_OSX)
    close(fd);
#elif defined(PLATFORM_LINUX)
    close(fd);
#endif
}

int32_t event_add_read(x_event_fd fd, x_socket_fd socket_fd, void *data) {
#if defined(PLATFORM_WIN)
	return 1;
#elif defined(PLATFORM_OSX)
    struct kevent event;
	EV_SET(&event, socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, data);
    if (kevent(fd, &event, 1, NULL, 0, NULL) == -1) {
        return 0;
    }
	EV_SET(&event, socket_fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, data);
	if (kevent(fd, &event, 1, NULL, 0, NULL) == -1) {
		EV_SET(&event, socket_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
		kevent(fd, &event, 1, NULL, 0, NULL);
		return 0;
	}
    return 1;
#elif defined(PLATFORM_LINUX)
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	event.data.ptr = data;
	if (epoll_ctl(fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
		return 0;
	}
	return 1;
#endif
}

void event_remove_read(x_event_fd fd, x_socket_fd socket_fd, void *data) {
#if defined(PLATFORM_OSX)
	struct kevent event;
	EV_SET(&event, socket_fd, EVFILT_READ, EV_DELETE, 0, 0, data);
	kevent(fd, &event, 1, NULL, 0, NULL);
	EV_SET(&event, socket_fd, EVFILT_WRITE, EV_DELETE, 0, 0, data);
	kevent(fd, &event, 1, NULL, 0, NULL);
#elif defined(PLATFORM_LINUX)
	epoll_ctl(fd, EPOLL_CTL_DEL, socket_fd, NULL);
#endif
}

void event_add_write(x_event_fd fd, x_socket_fd socket_fd, void *data) {
#if defined(PLATFORM_OSX)
	struct kevent event;
	EV_SET(&event, socket_fd, EVFILT_WRITE, EV_ENABLE, 0, 0, data);
	kevent(fd, &event, 1, NULL, 0, NULL);
#elif defined(PLATFORM_LINUX)
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	event.data.ptr = data;
	epoll_ctl(fd, EPOLL_CTL_MOD, socket_fd, &event);
#endif
}

void event_remove_write(x_event_fd fd, x_socket_fd socket_fd, void *data) {
#if defined(PLATFORM_OSX)
	struct kevent event;
	EV_SET(&event, socket_fd, EVFILT_WRITE, EV_DISABLE, 0, 0, data);
	kevent(fd, &event, 1, NULL, 0, NULL);
#elif defined(PLATFORM_LINUX)
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	event.data.ptr = data;
	epoll_ctl(fd, EPOLL_CTL_MOD, socket_fd, &event);
#endif
}

int32_t event_dispatch(x_event_fd fd, struct x_event *event) {
#if defined(PLATFORM_WIN)
	return 0;
#elif defined(PLATFORM_OSX)
	int32_t count = kevent(fd, NULL, 0, s_events, s_max, NULL);
	for (int32_t i = 0; i < count; i++) {
		event[i].data = s_events[i].udata;
		event[i].type = 0;
		if (s_events[i].filter == EVFILT_READ) {
			event[i].type |= x_event_type_read;
		}
		if (s_events[i].filter == EVFILT_WRITE) {
			event[i].type |= x_event_type_write;
		}
    }
    return count;
#elif defined(PLATFORM_LINUX)
	int32_t count = epoll_wait(fd, s_events, s_max, -1);
	for (int32_t i = 0; i < count; i++) {
		event[i].data = s_events[i].data.ptr;
		event[i].type = 0;
		if (s_events[i].events & EPOLLIN) {
			event[i].type |= x_event_type_read;
		}
		if (s_events[i].events & EPOLLOUT) {
			event[i].type |= x_event_type_write;
		}
	}
	return count;
#endif
}