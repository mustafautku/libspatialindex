#!/bin/bash
set -e

# Aktif dosya (örneğin bir editörden gönderildiğinde ya da terminalde elle yazıldığında)
ACTIVE_FILE="$1"

if [ -z "$ACTIVE_FILE" ]; then
    echo "Usage: $0 <relative-path-to-source-file.cpp>"
    exit 1
fi

# Kaynak dosyasından target ismini çıkarmak için basename kullan (örnek: test/spatial_test.cpp → spatial_test)
FILENAME=$(basename -- "$ACTIVE_FILE")
TARGET="test-rtree-${FILENAME%.*}"  # .cpp uzantısını kaldır

# Build dizinini belirt (senin yapında mybuild)
BUILD_DIR="mybuild"

# CMake ile hedefi derle
echo "👉 Building target: $TARGET"
cmake --build "$BUILD_DIR" --target "$TARGET" -j$(nproc)
