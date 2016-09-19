#!/bin/bash
PLATFORM=$1

read -a TEST_DLLS <<< $2

for filename in "${TEST_DLLS[@]}"
do
    :
    echo "Testing $filename"
    ./runwine.sh "${PLATFORM}" ./LoadDll.exe $filename
    if [ "$?" != "0" ]; then
        exit 1
    fi
done

echo "${#TEST_DLLS[@]} tests completed successfully"
