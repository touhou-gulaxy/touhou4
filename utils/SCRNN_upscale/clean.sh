#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$script_dir/build"

if [[ "$build_dir" != "$script_dir/build" ]]; then
    printf 'Refusing to remove unexpected path: %s\n' "$build_dir" >&2
    exit 1
fi

if [[ -d "$build_dir" ]]; then
    rm -rf -- "$build_dir"
    printf 'Removed: %s\n' "$build_dir"
else
    printf 'Nothing to clean: %s\n' "$build_dir"
fi
