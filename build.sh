#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export N64_INST="${N64_INST:-/opt/libdragon}"
export PATH="$N64_INST/bin:$PATH"

# GNU Make e o toolchain do N64 exigem caminho sem espaços.
# Se o diretório atual contiver espaços, criamos um symlink temporário sem espaços.
if [[ "$SCRIPT_DIR" =~ [[:space:]] ]]; then
    if [ -d "/home/georges" ] && [ -w "/home/georges" ]; then
        WORK_DIR="/home/georges/veil_n64"
    else
        WORK_DIR="${TMPDIR:-/tmp}/veil_n64_${UID:-0}"
    fi
    ln -sfn "$SCRIPT_DIR" "$WORK_DIR"
else
    WORK_DIR="$SCRIPT_DIR"
fi

cd "$WORK_DIR"
export T3D_INST="${T3D_INST:-$WORK_DIR/deps/tiny3d}"
make "$@"
