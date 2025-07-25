#!/bin/bash
set -e  # ilerde hata olunca hemen sonlan, sonrai komutlari isleme manasında

# cmake . -Bmybuild -DCMAKE_INSTALL_PREFIX=$HOME/PROGRAMMING/libspatialindex -DCMAKE_BUILD_TYPE=Debug 
cmake -S . -B mybuild \
    -DCMAKE_INSTALL_PREFIX=$HOME/PROGRAMMING/libspatialindex \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
    -DCMAKE_INSTALL_RPATH="$HOME/PROGRAMMING/libspatialindex/lib;$HOME/projeler/libspatialindex/mybuild/lib" \
    -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=FALSE

#   cmake — Invokes the CMake configuration tool.
#   . — Specifies the current directory as the source directory (where the CMakeLists.txt file is).
#   -Bmybuild — Tells CMake to generate all build files (like Makefile, CMakeCache.txt, etc.) in a new directory called mybuild. if mybuild does not exists,it will be generated
#   -DCMAKE_INSTALL_PREFIX=$HOME/PROGRAMMING/libspatialindex — Sets the custom installation path to your PROGRAMMING directory under your home folder.

#   the last 3 settings guarantees
#       Only to search $HOME/PROGRAMMING/libspatialindex/lib  for dynamic libraries, do not search anywhere else like usr/local..
#       the second path in DCMAKE_INSTALL_RPATH is for gtest, since its binaries are not install outside.

cmake --build mybuild -j$(nproc)         # Compiles the project
cmake --install mybuild                  # Installs it to $HOME/PROGRAMMING/libspatialindex
