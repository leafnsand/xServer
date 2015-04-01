#ifndef __X_SERVER_INCLUDE_H__
#define __X_SERVER_INCLUDE_H__

#include "xPlatform.h"

#if defined(PLATFORM_WIN)

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <process.h>
#include <winsock2.h>

#elif defined(PLATFORM_OSX)

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <dlfcn.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/errno.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>

#elif defined(PLATFORM_LINUX)

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>

#include <arpa/inet.h>
#include <dlfcn.h>
#include <linux/tcp.h>
#include <netinet/in.h>
#include <pthread.h>

#include <sys/epoll.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>

#endif

void x_server_internal_message(int32_t instance_id, int32_t type, int32_t to, void *data, size_t size);

#include "xServer.h"
#include "xMalloc.h"
#include "xLogger.h"
#include "xLock.h"
#include "xSpinLock.h"
#include "xMessageQueue.h"
#include "xEnvironment.h"
#include "xModule.h"
#include "xInstance.h"
#include "xTimer.h"
#include "xSocket.h"
#include "xEvent.h"
#include "xNetwork.h"
#include "xMain.h"

#endif