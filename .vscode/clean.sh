#!/bin/bash
set -e

BUILD_DIR="mybuild"

if [ -d "$BUILD_DIR" ]; then
    echo "🧹 Removing build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
    echo "✅ Clean complete."
else
    echo "ℹ️ Build directory '$BUILD_DIR' does not exist. Nothing to clean."
fi
