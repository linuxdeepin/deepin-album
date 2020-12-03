#!/bin/sh
echo "start..."
cd ..
cd ..
cd build-tests-unknown-Debug/
./deepin-album-test
cd CMakeFiles/deepin-album-test.dir/data/home/zouya/album0624/deepin-album/album
#lcov --directory . --zerocounters     
lcov --directory . --capture --output-file app.info
genhtml -o ../../../../../../../../results app.info
