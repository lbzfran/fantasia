
#define FAN_PLATFORM_RAYLIB
#ifdef  FAN_PLATFORM_RAYLIB
# include "platform_raylib.c"
#endif

#if   defined(OS_WINDOWS)
# include "os_windows.c"
#elif defined(OS_LINUX)
# include "os_linux.c"
#endif
