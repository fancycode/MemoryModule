#!/bin/bash
set -eu
PLATFORM=$1
shift

if [ "${PLATFORM}" = "x86_64" ]; then
    export WINEPREFIX=${HOME}/.wine64/
    WINE=wine64
else
    export WINEPREFIX=${HOME}/.wine/
    WINE=wine
fi
export WINEPATH=/usr/lib/gcc/${PLATFORM}-w64-mingw32/4.8/:/usr/${PLATFORM}-w64-mingw32/lib

exec ${WINE} $@
