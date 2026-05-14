#!/bin/sh

TARGET=${1:-$(uname | tr '[:upper:]' '[:lower:]')}
PLATFORM=${2:-glfw}
RENDERER=${3:-opengl}
RAYLIB_VERSION=6.0

mkdir -p bin build lib include

case "$TARGET" in
    *"linux"*)   TARGET=linux ;;
    *"windows"*) TARGET=windows ;;
    *) echo "ERROR: unsupported target '$TARGET'"; exit 1 ;;
esac

# ----------------------------
# renderer selection
# ----------------------------
case "$PLATFORM" in
    glfw)
        PLATFORM="PLATFORM_DESKTOP_GLFW"
        ;;
    rgfw)
        PLATFORM="PLATFORM_DESKTOP_RGFW"
        echo "WARN: There is a currently a bug in the game that"
        echo "      causes the game to hang when shutting down."
        echo "      Please be careful."
        ;;
    *)
        echo "ERROR: renderer must be 'glfw' or 'rgfw'."
        exit 1
        ;;
esac

case "$RENDERER" in
    soft)
        GRAPHICS="GRAPHICS_API_OPENGL_SOFTWARE"
        ;;
    *)
        GRAPHICS="GRAPHICS_API_OPENGL_33"
        ;;
esac

echo "==> Target: $TARGET"
echo "==> Platform: $PLATFORM"
echo "==> Renderer: $GRAPHICS"

cd build || exit

# ----------------------------
# clone raylib source
# ----------------------------
if [ ! -d "raylib" ]; then
    git clone --depth=1 --branch ${RAYLIB_VERSION} https://github.com/raysan5/raylib.git
    echo "Cloned raylib ${RAYLIB_VERSION}"
fi

cd raylib/src || exit

# ----------------------------
# clean previous build
# ----------------------------
make clean >/dev/null 2>&1

# ----------------------------
# build raylib
# ----------------------------
if [ "$TARGET" = "linux" ]; then
    make PLATFORM=$PLATFORM GRAPHICS=$GRAPHICS GLFW_LINUX_ENABLE_WAYLAND=TRUE RGFW_LINUX_ENABLE_WAYLAND=TRUE
elif [ "$TARGET" = "windows" ]; then
    make PLATFORM=$PLATFORM GRAPHICS=$GRAPHICS CC=x86_64-w64-mingw32-gcc
fi

# ----------------------------
# copy outputs
# ----------------------------
cp libraylib.a ../../../lib/
echo "Copied libraylib.a"

cp -r ../src/*.h ../../../include/
echo "Copied headers"

cd ../../..
