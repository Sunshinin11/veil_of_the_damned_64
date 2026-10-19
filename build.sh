#!/usr/bin/env bash
set -e
export N64_INST=/opt/libdragon
export PATH=/opt/libdragon/bin:$PATH

# Ensure symlink without spaces exists
ln -sfn "/mnt/c/Desenvolvimento/Nintendo 64/Veil of the Damned 64" /home/georges/veil_n64

cd /home/georges/veil_n64
export T3D_INST=/home/georges/veil_n64/deps/tiny3d
make "$@"
