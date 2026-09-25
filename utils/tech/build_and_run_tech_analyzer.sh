#!/usr/bin/env bash
# =============================================================================
#  build_and_run_tech_analyzer.sh
#
#  作用: 编译同目录下的 tech_dependency_analyzer.cpp -> 运行 -> 删除临时二进制。
#        所有参数都会原样传给分析器, 例如:
#            ./build_and_run_tech_analyzer.sh --keep-ai-only
#            ./build_and_run_tech_analyzer.sh --out-dir /tmp/tech_out
#
#  用法: ./build_and_run_tech_analyzer.sh [分析器的参数...]
# =============================================================================

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
src="$script_dir/tech_dependency_analyzer.cpp"

if [[ ! -f "$src" ]]; then
    echo "找不到源文件: $src" >&2
    exit 1
fi

if ! command -v g++ >/dev/null 2>&1; then
    echo "找不到 g++, 请先安装 g++ (例如 apt install g++)" >&2
    exit 1
fi

# 二进制放在临时目录, 退出时(正常结束 / 报错 / Ctrl-C)都会被清掉
tmp_dir="$(mktemp -d "${TMPDIR:-/tmp}/tech_analyzer.XXXXXX")"
bin="$tmp_dir/tech_dependency_analyzer"

cleanup() {
    rm -rf "$tmp_dir"
}
trap cleanup EXIT INT TERM

echo "[1/3] 编译: $src"
g++ -std=c++17 -O2 -Wall -Wextra -o "$bin" "$src"

echo "[2/3] 运行: $bin $*"
"$bin" "$@"

echo "[3/3] 清理临时二进制: $tmp_dir"
