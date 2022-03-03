#!/bin/bash
export QT_QPA_PLATFORM='offscreen'
export QTEST_FUNCTION_TIMEOUT='800000'
mkdir -p ${HOME}/图片/AlbumtestResource/
mkdir -p ${HOME}/Pictures/AlbumtestResource/
cp -r ./testResource/pic/* ${HOME}/图片/AlbumtestResource/
cp -r ./testResource/pic/* ${HOME}/Pictures/AlbumtestResource/
mkdir -p ${HOME}/second/
cp -r ./testResource/second/* ${HOME}/second/
#模拟设备导入时使用
mkdir -p ${HOME}/dev/
cp -r ./testResource/dev/* ${HOME}/dev/
#测试文件监视用
rm -rf ${HOME}/图片/Screenshots
cp -r ./testResource/pic/Draw ${HOME}/图片/Screenshots
rm -rf ${HOME}/Pictures/Screenshots
cp -r ./testResource/pic/Draw ${HOME}/Pictures/Screenshots
rm -rf ${HOME}'/视频/Screen Recordings'
cp -r ./testResource/pic/Draw ${HOME}'/视频/Screen Recordings'
rm -rf ${HOME}'/Videos/Screen Recordings'
cp -r ./testResource/pic/Draw ${HOME}'/Videos/Screen Recordings'

cd ..
rm -rf ./build-ut
rm -rf ./build
rm -rf ${HOME}/.local/share/deepin/deepin-album*
#导入数据库时使用
mkdir -p ${HOME}/.local/share/deepin/deepin-album/
#导入旧的数据库文件
cp -r ./tests/testResource/db/* ${HOME}/.local/share/deepin/deepin-album/
mkdir build-ut

cmake . -B build -D DOTEST=ON
cd build

#自动读取当前处理器核心数，但考虑到服务器上会同时存在多个构建，完全占用服务器CPU会导致构建变慢，所以限制使用的核心不超过8个
JOBS=`cat /proc/cpuinfo| grep "processor"|  wc -l`
if [ $JOBS -gt 8 ]
then JOBS=8
elif [ $JOBS -eq 0 ]
then JOBS=1
fi

echo use processor count: $JOBS
make -j$JOBS

lcov --directory ./CMakeFiles/deepin-album_test.dir --zerocounters
ASAN_OPTIONS="fast_unwind_on_malloc=1" ./src/deepin-album_test

lcov --directory . --capture --output-file ./coverageResult/deepin-album_Coverage.info
echo \ ===================\ do\ filter\ begin\ ====================\ 
lcov --remove ./coverageResult/deepin-album_Coverage.info '*/deepin-album_test_autogen/*' '*/deepin-album_autogen/*' '*/usr/include/*' '*/usr/local/*' '*/tests/*' '*/googletest/*' '*/UnionImage/*' -o ./coverageResult/deepin-album_Coverage.info
echo \ ===================\ do\ filter\ end\ ====================\ 
genhtml -o ./coverageResult/report ./coverageResult/deepin-album_Coverage.info

sleep 2

lcov --directory . --capture --output-file ./coverageResult/deepin-album_Coverage.info
echo \ ===================\ do\ filter\ begin\ ====================\ 
lcov --remove ./coverageResult/deepin-album_Coverage.info '*/deepin-album_test_autogen/*' '*/deepin-album_autogen/*' '*/usr/include/*' '*/usr/local/*' '*/tests/*' '*/googletest/*' '*/UnionImage/*' -o ./coverageResult/deepin-album_Coverage.info
echo \ ===================\ do\ filter\ end\ ====================\ 
genhtml -o ./coverageResult/report ./coverageResult/deepin-album_Coverage.info



cd ./../build-ut

cp -r ./../build/coverageResult/report/ ./
mv report html
cd html
mv index.html cov_deepin-album.html

cd ..
mkdir report
cd report
cp ./../../build/report/report_deepin-album.xml ./

cd ..
cp ./../build/asan_deepin-album.log* ./asan_deepin-album.log

exit 0
