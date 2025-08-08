
#define FAN_PLATFORM_RAYLIB
#ifdef  FAN_PLATFORM_RAYLIB
# include "platform_raylib.c"
#endif

#if   defined(OS_WINDOWS)
# include "os_windows.c"
#elif defined(OS_LINUX)
# include "os_linux.c"
#endif



int FanRectIsEmpty(FanRect rect) {
    int result = 1;
    if (rect.x && rect.y && rect.width && rect.height) {
        result = 0;
    }
    return result;
}
