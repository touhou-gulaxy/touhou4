#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$script_dir/build"
binary="$build_dir/scrnn-upscale"

if [[ ! -x "$binary" ]]; then
    "$script_dir/build.sh"
fi

exec "$binary" "$@"
