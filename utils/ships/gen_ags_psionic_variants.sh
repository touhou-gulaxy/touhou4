#!/usr/bin/env bash
#
# 编译并运行 ags_psionic_variants.cpp（生成 AGS 灵能舰船的颜色差分注册文件），
# 运行结束后自动删除编译出来的二进制。
#
# 用法（脚本放在 <mod>/utils/ships/ 下）：
#   ./gen_ags_psionic_variants.sh                 # 直接生成到 <mod>/gfx/models/ships/AGS_Ships
#   ./gen_ags_psionic_variants.sh --dry-run       # 只统计，不写文件
#   ./gen_ags_psionic_variants.sh --colors red    # 透传给 C++ 程序的参数都支持
#   ./gen_ags_psionic_variants.sh --list-keys AGS_psi_keys.txt
#
# 可选环境变量：
#   CXX=clang++                 指定编译器（默认 g++）
#   AGS_SHIPS_DIR=<路径>        覆盖 AGS_Ships 目录（默认 <mod>/gfx/models/ships/AGS_Ships）

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
MOD_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"

SRC="$SCRIPT_DIR/ags_psionic_variants.cpp"
AGS_DIR="${AGS_SHIPS_DIR:-$MOD_ROOT/gfx/models/ships/AGS_Ships}"
CXX="${CXX:-g++}"

die() { printf '错误: %s\n' "$1" >&2; exit 1; }

[[ -f "$SRC" ]]     || die "找不到源码: $SRC"
[[ -d "$AGS_DIR" ]] || die "找不到模型目录: $AGS_DIR（可用 AGS_SHIPS_DIR=... 覆盖）"
command -v "$CXX" >/dev/null 2>&1 || die "找不到编译器: $CXX（可用 CXX=clang++ 指定）"

# 二进制放临时目录，并保证退出（含 Ctrl-C / 出错）时删除
BIN="$(mktemp "${TMPDIR:-/tmp}/ags_psionic_variants.XXXXXX")"
cleanup() { rm -f -- "$BIN"; }
trap cleanup EXIT INT TERM

printf '[1/3] 编译 %s\n' "$SRC"
"$CXX" -std=c++17 -O2 -o "$BIN" "$SRC"

printf '[2/3] 运行（AGS_Ships: %s）\n' "$AGS_DIR"
"$BIN" --dir "$AGS_DIR" "$@"

printf '[3/3] 清理编译产物 %s\n' "$BIN"
