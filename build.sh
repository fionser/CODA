#!/bin/bash
ARGS=""
NR_THREADS=1
platform=`uname`
if [[ $platform == 'Linux' ]]; then
   echo "This platform can use mult-threads. Please set the number of threads: "
   read NR_THREADS
   ARGS="-DMULT_THREADS=on -DNR_THREADS=$NR_THREADS"
elif [[ $platform == 'Darwin' ]]; then
   ARGS="-DMULT_THREADS=off"
fi

BUILD_test=off
#!/bin/bash
mkdir -p build && cd build
cp ../script.sh .
chmod +x script.sh
./script.sh ${ARGS}
./script.sh ${ARGS} -DBUILD_core=on -DBUILD_test=$BUILD_test
./script.sh -DBUILD_ui=on
