#!/bin/bash
LIBS=(ntl gmp)

for lib in ${LIBS[@]}
do
    NEWNAME=BUILD_${lib}
    eval "$NEWNAME"="'on'"
done

for lib in ${LIBS[@]}
do
    if [ -e ./lib/lib${lib}.a ]; then
        NEWNAME=BUILD_${lib}
        eval "$NEWNAME"="'off'"
    fi
done

BUILD_LIB=on
LIBS_TO_BUILD=""
for lib in ${LIBS[@]}
do
    NEWNAME=BUILD_${lib}
    if [[ ${!NEWNAME} == "off" ]]; then
        BUILD_LIB=off
    fi
    LIBS_TO_BUILD=${LIBS_TO_BUILD}" "-D${NEWNAME}=${!NEWNAME}
done

echo "Runinng...."
if [ -e CMakeCache.txt ]; then
    rm CMakeCache.txt
fi

if [[ ${BUILD_LIB} == "on" ]]; then
    echo "cmake .. -DBUILD_lib=on $1 $2 $3"
    cmake .. -DBUILD_lib=on $1 $2 $3 && make -j 3
else
    echo "cmake .. ${LIBS_TO_BUILD} $1 $2 $3"
    cmake .. ${LIBS_TO_BUILD} $1 $2 $3 && make -j 3
fi
