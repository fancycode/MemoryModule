#!/bin/bash
PLATFORM=$1
if [ "${PLATFORM}" = "x86_64" ]; then
    export WINEPREFIX=${HOME}/.wine64/
    WINE=wine64
else
    export WINEPREFIX=${HOME}/.wine/
    WINE=wine
fi
export WINEPATH=/usr/lib/gcc/${PLATFORM}-w64-mingw32/4.6/

read -a TEST_DLLS <<< $2

for filename in "${TEST_DLLS[@]}"
do
    :
    echo "Testing $filename"
    ${WINE} ./LoadDll.exe $filename
    if [ "$?" != "0" ]; then
        exit 1
    fi
done

echo "${#TEST_DLLS[@]} tests completed successfully"
