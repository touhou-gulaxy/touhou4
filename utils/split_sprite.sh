#!/bin/bash
# split_sprite.sh - 将 PNG 精灵图切割为 26×26 的 DDS 文件
# 用法: ./split_sprite.sh [-o 输出目录] 输入.png [命名列表.txt]
#       选项可放在任意位置

set -euo pipefail

# 显示帮助
show_help() {
    cat <<EOF
用法: $0 [-o 输出目录] 输入.png [命名列表.txt]

选项:
  -o 输出目录    指定输出目录（默认当前目录）
  -h, --help     显示此帮助

参数:
  输入.png        待切割的 PNG 精灵图，宽高必须是 26 的倍数
  命名列表.txt    可选，每行一个文件名（不含扩展名），按行优先顺序命名

输出:
  在输出目录下生成 <名称>.dds 文件（默认 sprite_序号.dds）
EOF
}

# 初始化变量
OUTDIR="."
INPUT=""
NAMES_FILE=""

# 手动解析所有参数（支持选项任意顺序）
while [[ $# -gt 0 ]]; do
    case "$1" in
        -o)
            if [[ -z "$2" ]]; then
                echo "错误: -o 需要指定目录" >&2
                exit 1
            fi
            OUTDIR="$2"
            echo "output dir: $OUTDIR"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        -*)
            echo "错误: 未知选项 '$1'" >&2
            exit 1
            ;;
        *)
            # 第一个非选项参数 → 输入文件
            if [[ -z "$INPUT" ]]; then
                INPUT="$1"
            # 第二个非选项参数 → 命名列表
            elif [[ -z "$NAMES_FILE" ]]; then
                NAMES_FILE="$1"
            else
                echo "错误: 多余的参数 '$1'" >&2
                exit 1
            fi
            shift
            ;;
    esac
done

# 检查是否指定了输入文件
if [[ -z "$INPUT" ]]; then
    echo "错误: 未指定输入文件" >&2
    show_help
    exit 1
fi

# 检查输入文件是否存在
if [[ ! -f "$INPUT" ]]; then
    echo "错误: 文件 '$INPUT' 不存在" >&2
    exit 1
fi

echo "input file: $INPUT, name file: $NAMES_FILE"

# 检查 ImageMagick 工具
for cmd in identify convert; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "错误: 未找到 '$cmd'，请安装 ImageMagick" >&2
        exit 1
    fi
done

# 创建输出目录
mkdir -p "$OUTDIR"

echo "正在获取图片尺寸..."

# 分别获取宽度和高度（更稳健，便于错误处理）
width=$(identify -format "%w" "$INPUT" 2>/dev/null) || {
    echo "错误: 无法获取图片宽度，请确认文件为有效图片" >&2
    exit 1
}
height=$(identify -format "%h" "$INPUT" 2>/dev/null) || {
    echo "错误: 无法获取图片高度，请确认文件为有效图片" >&2
    exit 1
}

# 检查是否获取到有效数字
if [[ ! "$width" =~ ^[0-9]+$ ]] || [[ ! "$height" =~ ^[0-9]+$ ]]; then
    echo "错误: 获取的尺寸无效 (width='$width', height='$height')" >&2
    exit 1
fi

echo "原始尺寸: ${width}x${height}"

# 验证尺寸
if (( width % 26 != 0 )) || (( height % 26 != 0 )); then
    echo "错误: 宽度和高度必须是 26 的倍数 (当前 ${width}x${height})" >&2
    exit 1
fi

cols=$((width / 26))
rows=$((height / 26))
total=$((cols * rows))
echo "将生成 ${total} 个精灵 (${cols} 列 × ${rows} 行)"
echo "输出目录: $OUTDIR"

# 读取命名列表
names=()
if [[ -n "$NAMES_FILE" ]]; then
    if [[ ! -f "$NAMES_FILE" ]]; then
        echo "警告: 命名列表 '$NAMES_FILE' 不存在，将使用默认命名" >&2
    else
        mapfile -t names < <(grep -v '^[[:space:]]*$' "$NAMES_FILE" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
        if (( ${#names[@]} < total )); then
            echo "警告: 命名列表只有 ${#names[@]} 个名称，少于 ${total} 个精灵，其余将使用默认名称" >&2
        fi
    fi
fi

# 取名称函数
get_name() {
    local idx=$1
    if (( idx < ${#names[@]} )); then
        echo "${names[idx]}"
    else
        echo "sprite_${idx}"
    fi
}

# 切割并输出
for ((y=0; y<rows; y++)); do
    for ((x=0; x<cols; x++)); do
        idx=$((y * cols + x))
        name=$(get_name "$idx")
        outfile="${OUTDIR}/${name}.dds"
        left=$((x * 26))
        top=$((y * 26))
        echo "切割 (${left},${top}) → ${outfile}"
        convert "$INPUT" -crop "26x26+${left}+${top}" +repage -define dds:compression=none "$outfile"
    done
done

echo "完成！共生成 ${total} 个 DDS 文件，位于 ${OUTDIR}"
