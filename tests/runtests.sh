#!/bin/bash
if [ "$1" = "x86_64" ]; then
    export WINEPREFIX=${HOME}/.wine64/
else
    export WINEPREFIX=${HOME}/.wine/
fi

read -a TEST_DLLS <<< $2

for filename in "${TEST_DLLS[@]}"
do
    :
    echo "Testing $filename"
    ./LoadDll.exe $filename
    if [ "$?" != "0" ]; then
        exit 1
    fi
done

echo "${#TEST_DLLS[@]} tests completed successfully"
