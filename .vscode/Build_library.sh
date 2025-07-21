#!/bin/bash
set -e  # ilerde hata olunca hemen sonlan, sonrai komutlari isleme manasında

cmake . -Bmybuild -DCMAKE_INSTALL_PREFIX=$HOME/PROGRAMMING/libspatialindex -DCMAKE_BUILD_TYPE=Debug 
# if mybuild does not exists,it will be generated

#   cmake — Invokes the CMake configuration tool.
#   . — Specifies the current directory as the source directory (where the CMakeLists.txt file is).
#   -Bmybuild — Tells CMake to generate all build files (like Makefile, CMakeCache.txt, etc.) in a new directory called mybuild.
#   -DCMAKE_INSTALL_PREFIX=$HOME/PROGRAMMING/libspatialindex — Sets the custom installation path to your PROGRAMMING directory under your home folder.

cmake --build mybuild -j$(nproc)         # Compiles the project
cmake --install mybuild                  # Installs it to $HOME/PROGRAMMING/libspatialindex
