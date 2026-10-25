#!/usr/bin/env bash
#
# 生成并同步「舰船组件属性 / 资源成本 / 缩放系数」到 mod 的 scripted_variables。
#
#   1) 编译 C++ 生成器 component_vars        → components_attributes.log / components_resources.log / components_scales.log
#   2) 编译并运行 component_vars_sync        → 原地合并进 common/scripted_variables/spth_component_variables.txt
#
# 用法（脚本放在 <mod>/utils/ 下）：
#   ./gen_component_vars.sh                # 直接同步
#   ./gen_component_vars.sh --dry-run      # 只看差异，不写文件
#   ./gen_component_vars.sh --list         # 打印每条 更新/新增
#   ./gen_component_vars.sh --no-backup    # 不生成 .bak
#
# 可选环境变量：
#   CXX=clang++                覆盖编译器（默认 g++）
#   TARGET=<file>              覆盖目标 scripted_variables 文件

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
MOD_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

CXX="${CXX:-g++}"
TARGET="${TARGET:-$MOD_ROOT/common/scripted_variables/spth_component_variables.txt}"
COMP_SRC_DIR="$SCRIPT_DIR/comp"

die() { printf '错误: %s\n' "$1" >&2; exit 1; }

[[ -f "$TARGET" ]] || die "找不到目标 scripted_variables: $TARGET（可用 TARGET=... 覆盖）"
command -v "$CXX" >/dev/null 2>&1 || die "找不到编译器: $CXX"
[[ -f "$COMP_SRC_DIR/component_vars.cpp" ]] || die "缺少 $COMP_SRC_DIR/component_vars.cpp"
[[ -f "$SCRIPT_DIR/component_vars_sync.cpp" ]] || die "缺少 $SCRIPT_DIR/component_vars_sync.cpp"

# 编译产物放临时目录，退出（含 Ctrl-C）时自动清理
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/component_vars.XXXXXX")"
cleanup() { rm -rf -- "$TMP_DIR"; }
trap cleanup EXIT INT TERM

printf '[1/3] 编译 C++ 生成器与同步器\n'
"$CXX" -std=c++17 -O2 -o "$TMP_DIR/component_vars" "$COMP_SRC_DIR/component_vars.cpp"
"$CXX" -std=c++17 -O2 -o "$TMP_DIR/component_vars_sync" "$SCRIPT_DIR/component_vars_sync.cpp"

printf '[2/3] 生成定义文件（%s）\n' "$SCRIPT_DIR"
"$TMP_DIR/component_vars" --dir "$SCRIPT_DIR"

printf '[3/3] 合并进 %s\n' "$TARGET"
"$TMP_DIR/component_vars_sync" \
    --target "$TARGET" \
    --attr  "$SCRIPT_DIR/components_attributes.log" \
    --res   "$SCRIPT_DIR/components_resources.log" \
    --scale "$SCRIPT_DIR/components_scales.log" "$@"
