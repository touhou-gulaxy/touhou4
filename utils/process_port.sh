#!/bin/bash

# process function：convert 512x512 texture to 512x1075, bottom=465
process_texture() {
    local input_file="$1"
    local output_file="$2"

    if [[ ! -f "$input_file" ]]; then
        echo "ERR: input file $input_file does not exists."
        return 1
    fi

    local dimensions=$(identify -format "%wx%h" "$input_file" 2>/dev/null)
    if [[ "$dimensions" != "512x512" ]]; then
        echo "ERR: input file dimension is $dimensions, excepting 512x512."
        return 1
    fi

    echo "Process Texture: $input_file"

    local temp_file=$(mktemp --suffix=.png)

    trap "rm -f '$temp_file'" EXIT ERR

    # 先将画布扩展到1075像素高度
    echo "步骤1: 扩展画布到1075像素高度..."
    ../../magick "$input_file" \
        -background none \
        -gravity north \
        -extent 512x1075 \
        "$temp_file"

    # 直接向上移动300像素，不进行裁剪
    echo " 步骤2: 向上移动300像素..."
    ../../magick "$temp_file" \
        -gravity south \
        -background none \
        -splice 0x300 \
        -gravity center \
        -background none \
        -extent 512x1075 \
        -define dds:compression=none \
        -define dds:fast-mipmaps=false \
        "$output_file"

    # 检查输出文件
    if [[ -f "$output_file" ]]; then
        local output_dimensions=$(identify -format "%wx%h" "$output_file" 2>/dev/null)
        echo "处理完成: $output_file (尺寸: $output_dimensions)"

        # 验证原始内容位置
        echo "原始图层已上移300像素，纹理高度扩展到1075像素"
    else
        echo "错误: 输出文件创建失败"
        return 1
    fi

    # 清理临时文件
    rm -f "$temp_file1" "$temp_file2"
    trap - EXIT ERR

    return 0
}

# 批量处理函数
batch_process() {
    local input_dir="$1"
    local output_dir="$2"

    # 创建输出目录
    mkdir -p "$output_dir"

    # 计数器
    local count=0
    local success_count=0

    echo "开始批量处理'$input_dir'目录下的DDS文件..."

    # 遍历所有512x512的DDS文件
    for input_file in "$input_dir"/*.dds; do
        if [[ -f "$input_file" ]]; then
            local dimensions=$(identify -format "%wx%h" "$input_file" 2>/dev/null)
            if [[ "$dimensions" == "512x512" ]]; then
                count=$((count + 1))
                local filename=$(basename "$input_file")
                local output_file="$output_dir/${filename%.dds}_processed.dds"

                echo ""
                echo "处理文件 $count: $filename"

                if process_texture "$input_file" "$output_file"; then
                    success_count=$((success_count + 1))
                fi
            fi
        fi
    done

    echo ""
    echo "批量处理完成!"
    echo "成功处理: $success_count/$count 个文件"
    echo "输出目录: $output_dir"
}

# batch_process "../gfx/models/portraits/THMG2Style/" "../gfx/models/portraits/THMG2New/"
# batch_process "../gfx/models/portraits/" "../gfx/models/portraits/touhou_new/"
batch_process "../gfx/models/portraits/THEclipse/" "../gfx/models/portraits/THEclipseNew/"
