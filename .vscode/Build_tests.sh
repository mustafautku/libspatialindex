#!/bin/bash
set -e  # ilerde hata olunca hemen sonlan, sonrai komutlari isleme manasında


mkdir -p "$(dirname "$0")/test/mybuild"   # Create the build directory if it doesn't exist
cd "$(dirname "$0")/test/mybuild"


cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=$HOME/PROGRAMMING/libspatialindex \
  -DCMAKE_INSTALL_RPATH=$HOME/PROGRAMMING/libspatialindex/lib \
  -DCMAKE_EXE_LINKER_FLAGS="-L$HOME/PROGRAMMING/libspatialindex/lib -Wl,-rpath=$HOME/PROGRAMMING/libspatialindex/lib" \
  -DCMAKE_SHARED_LINKER_FLAGS="-L$HOME/PROGRAMMING/libspatialindex/lib -Wl,-rpath=$HOME/PROGRAMMING/libspatialindex/lib"

#make clean

make -j$(nproc)
