#ifndef __X_PLATFORM__
#define __X_PLATFORM__

#if defined(WINDOWS) || defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)

#define PLATFORM_WIN
#define X_INLINE

#ifdef X_SERVER_MODULE

#define X_SERVER_API __declspec(dllimport)
#define X_MODULE_API __declspec(dllexport)

#else

#define X_SERVER_API __declspec(dllexport)
#define X_MODULE_API __declspec(dllimport)

#endif

#elif defined(__APPLE__)

#define X_INLINE inline

#ifdef X_SERVER_MODULE

#define PLATFORM_OSX
#define X_SERVER_API
#define X_MODULE_API __attribute__((visibility("default")))

#else

#define PLATFORM_OSX
#define X_SERVER_API __attribute__((visibility("default")))
#define X_MODULE_API

#endif

#elif defined(linux) || defined(__linux__)

#define PLATFORM_LINUX
#define X_INLINE inline
#define X_SERVER_API
#define X_MODULE_API

#endif

#endif