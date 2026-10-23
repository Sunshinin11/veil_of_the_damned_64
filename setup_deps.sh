#!/usr/bin/env bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "$SCRIPT_DIR/deps"
cd "$SCRIPT_DIR/deps"

if [ ! -d "tiny3d" ]; then
    echo "Cloning Tiny3D..."
    git clone -c filter.lfs.smudge= -c filter.lfs.required=false https://github.com/HailToDodongo/tiny3d.git tiny3d
fi

cd tiny3d
export N64_INST="${N64_INST:-/opt/libdragon}"
export PATH="$N64_INST/bin:$PATH"
echo "Building Tiny3D..."
make -j$(nproc)
echo "Tiny3D build completed successfully!"
