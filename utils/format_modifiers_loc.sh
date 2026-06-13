#!/bin/bash

lines=()
while IFS= read -r line || [ -n "$line" ]; do
    if [ "$line" = "EOF" ]; then
        echo "end of input." >&2       # 调试信息建议输出到 stderr，避免干扰管道
        break
    fi
    echo "input new line: $line." >&2
    lines+=("$line")
done

printf "%s\n" "${lines[@]}" | awk '
{
    # 只处理内部至少包含一个小写字母的 $...$ 模式
    while (match($0, /\$[^$]*[a-z][^$]*\$/)) {
        before = substr($0, 1, RSTART-1)
        inner  = substr($0, RSTART+1, RLENGTH-2)
        upper  = toupper(inner)
        $0 = before "$" upper "$" substr($0, RSTART+RLENGTH)
    }
    print
}'
