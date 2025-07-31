#!/bin/sh
#
# only support x86_64 by default.
# usage: ./setup.sh <TARGET_OR_NONE_SPECIFIED>

TARGET=${1:-$(uname | tr '[:upper:]' '[:lower:]')}
RAYLIB_VERSION=5.5

case "$TARGET" in
    *"linux"*)
        TARGET=linux
        ;;
    *"windows"*)
        TARGET=windows
        ;;
    *)
        ;;
esac


if [ "$TARGET" = "linux" ]; then
    RAYLIB_BUILD=linux_amd64
    RAYLIB_ZIP="raylib.tar.gz"
elif [ "$TARGET" = "windows" ]; then
    RAYLIB_BUILD=win64_mingw-w64
    RAYLIB_ZIP="raylib.zip"
else
    printf "ERROR: '$TARGET' unsupported by setup script.\n"
    exit 1
fi


RAYLIB_DIR="raylib-${RAYLIB_VERSION}_${RAYLIB_BUILD}"

if [ -f "lib/libraylib.a" ]; then
    echo -n "Raylib library found. continue anyways? (y/N): "
    read confirm

    case "$confirm" in
        [Yy])
            printf "\tRM -f " && echo lib/*raylib*
            rm -f lib/*raylib*
            ;;
        *)
            exit 0
            ;;
    esac
fi

mkdir -p build lib include
cd build && printf "\tCD build\n"
    if [ "$TARGET" = "linux" ]; then
        [ ! -d "$RAYLIB_DIR" ] && \
            curl -Lo "$RAYLIB_ZIP" "https://github.com/raysan5/raylib/releases/download/${RAYLIB_VERSION}/$RAYLIB_DIR.tar.gz" && \
            printf "\tCURL $RAYLIB_ZIP\n" && \
            tar -xzf $RAYLIB_ZIP && \
            printf "\tTAR -xzf $RAYLIB_ZIP\n"
    elif [ "$TARGET" = "windows" ]; then
        [ ! -d "$RAYLIB_DIR" ] && \
            curl -Lo "$RAYLIB_ZIP" "https://github.com/raysan5/raylib/releases/download/${RAYLIB_VERSION}/$RAYLIB_DIR.zip" && \
            printf "\tCURL $RAYLIB_ZIP\n" && \
            unzip $RAYLIB_ZIP && \
            printf "\tUNZIP $RAYLIB_ZIP\n"
    fi

    echo -n "Do you want to delete dependencies after import? (y/N): "
    read confirm

    cp $RAYLIB_DIR/lib/libraylib.a ../lib/ && \
        printf "\tCP $RAYLIB_DIR/lib/libraylib.a ../lib/\n"

    cp -a $RAYLIB_DIR/include/. ../include/ && \
        printf "\tCP $RAYLIB_DIR/include/ ../include/\n"

    rm -f $RAYLIB_ZIP && \
        printf "\tRM $RAYLIB_ZIP\n"

    case "$confirm" in
        [Yy])
            rm -rf $RAYLIB_DIR &&
                printf "\tRM -rf $RAYLIB_DIR\n"
            ;;
        *)
            ;;
    esac
cd .. && printf "\tCD ..\n"

