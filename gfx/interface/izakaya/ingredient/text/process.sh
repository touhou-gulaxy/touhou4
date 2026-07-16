#!/bin/bash
set -e

# 配置
TEMPLATE="../izakaya_ingredient_building_template.dds"
OUTPUT_DIR="../buildings"
X=27
Y=25

# 创建输出目录（若不存在）
mkdir -p "$OUTPUT_DIR"

# 检查模板是否存在
if [ ! -f "$TEMPLATE" ]; then
    echo "错误：模板文件 $TEMPLATE 不存在" >&2
    exit 1
fi

# 遍历当前目录下的所有 .dds 文件
for file in *.dds; do
    # 若无匹配文件，跳过（避免通配符原样输出）
    [ -e "$file" ] || continue

    # 跳过模板自身
    [ "$file" = "$TEMPLATE" ] && continue

    # 提取不带扩展名的文件名
    base="${file%.dds}"
    output="${OUTPUT_DIR}/building_izakaya_ingredient_${base}.dds"

    echo "处理: $file -> $output"
    composite -define dds:compression=none -geometry "+${X}+${Y}" "$file" "$TEMPLATE" "$output"
done

echo "全部处理完成。"
