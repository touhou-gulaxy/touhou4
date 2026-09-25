#!/usr/bin/env bash
#
# add_shine_animation.sh
# 给 Stellaris 精灵图定义文件 (sprite.txt) 里每个 spriteType 加上 "闪耀" animation 块。
#
# 处理规则:
#   * animationmaskfile    := 与该 spriteType 的 texturefile 完全相同的 dds 路径
#   * animationtexturefile := 固定的 gfx/interface/spth_ui/touhou_component_shine_overlay.dds
#   * 缩进自动跟随原文件风格 (Tab 或 4 空格)
#   * 原文件里写裸路径 (textureFile = gfx/xxx.dds) 的条目, 插入的内容也写裸路径
#   * 单行写法 spriteType = { name = "..." textureFile = "..." }
#     会先展开成多行, 再插入 animation 块 (可用 --no-expand-inline 关闭)
#   * 已带 animation 块的 spriteType 会跳过, 脚本可重复执行
#   * 被注释掉 (# 开头) 的 spriteType 不会被动
#
# 用法:
#   ./add_shine_animation.sh [选项] 输入文件
#     -w, --write          原地修改输入文件 (先备份成 输入文件.bak)
#     -o, --output FILE    结果写入 FILE
#         --no-expand-inline
#                          保留单行写法, 不为其插入 animation
#     -h, --help           显示本帮助
#
# 不加 -w / -o 时, 结果写到标准输出, 例如:
#   ./add_shine_animation.sh sprite.txt > sprite.new.txt

set -euo pipefail

overlay='gfx/interface/spth_ui/touhou_component_shine_overlay.dds'

expand_inline=1
mode=stdout
out_file=''
in_file=''

# 帮助信息就是脚本顶部那段注释 (跳过 shebang, 到第一个非注释行为止)
usage() { awk 'NR > 1 { if ($0 !~ /^#/) exit; sub(/^# ?/, ""); print }' "$0"; }

while [ $# -gt 0 ]; do
	case "$1" in
		-w|--write)          mode=write ;;
		-o|--output)         shift; [ $# -gt 0 ] || { echo "缺少 -o 的参数" >&2; exit 2; }; mode=output; out_file=$1 ;;
		--no-expand-inline)  expand_inline=0 ;;
		-h|--help)           usage; exit 0 ;;
		--)                  shift; break ;;
		-*)                  echo "未知选项: $1" >&2; usage >&2; exit 2 ;;
		*)                   break ;;
	esac
	shift
done

[ $# -eq 1 ] || { usage >&2; exit 2; }
in_file=$1
[ -f "$in_file" ] || { echo "找不到输入文件: $in_file" >&2; exit 1; }

tmp=$(mktemp "${TMPDIR:-/tmp}/sprite_anim.XXXXXX")
trap 'rm -f "$tmp"' EXIT

# ---------------------------------------------------------------------------
# awk 部分: 先把文件读进内存, 再统一处理
#   - 单行写法会被展开成多行
#   - 多行块在 texturefile 之后插入 animation
#   - 插入前先向后扫描该块, 已存在 animation 的块整块跳过 (保证可重复执行)
# ---------------------------------------------------------------------------
awk -v overlay="$overlay" -v expand_inline="$expand_inline" '
function leading_ws(s,   i, c) {
	i = 1
	while (i <= length(s)) {
		c = substr(s, i, 1)
		if (c != " " && c != "\t") break
		i++
	}
	return substr(s, 1, i - 1)
}

# 返回一个缩进层级: 原文件用 Tab 就继续用 Tab, 否则用 4 个空格
function indent_unit(ws) { return index(ws, "\t") > 0 ? "\t" : "    " }

# 取出 key = 后面的值, 可能是 "带引号" 也可能是裸路径
# (re 为正则字符串, 如 "name[ \t]*=[ \t]*")
# 通过全局变量 v_quoted 告知调用者取到的是否是带引号的值
function value_after(line, re,   rest, q2, i, c) {
	v_quoted = 0
	if (match(line, re) == 0) return ""
	rest = substr(line, RSTART + RLENGTH)
	if (substr(rest, 1, 1) == "\"") {
		q2 = index(substr(rest, 2), "\"")
		if (q2 == 0) return ""
		v_quoted = 1
		return substr(rest, 2, q2 - 1)
	}
	for (i = 1; i <= length(rest); i++) {
		c = substr(rest, i, 1)
		if (c == " " || c == "\t" || c == "}" || c == "#") break
	}
	return substr(rest, 1, i - 1)
}

# 该行的净花括号数 (忽略字符串内和注释后的内容)
function net_braces(line,   i, c, n, in_str) {
	n = 0; in_str = 0
	for (i = 1; i <= length(line); i++) {
		c = substr(line, i, 1)
		if (c == "\"")      in_str = !in_str
		else if (in_str)    continue
		else if (c == "#")  break
		else if (c == "{")  n++
		else if (c == "}")  n--
	}
	return n
}

# 从块首行 from 起向后扫描, 判断该块里是否已经有 animation =
function has_animation_ahead(from,   j, l, d) {
	d = 0
	for (j = from; j <= NR; j++) {
		l = lines[j]
		if (l ~ /^[ \t]*#/) continue
		d += net_braces(l)
		if (l ~ /^[ \t]*animation[ \t]*=/) return 1
		if (d <= 0 && j > from) return 0
	}
	return 0
}

function emit_animation(base_ws, mask, quote,   u, cw, q) {
	u  = indent_unit(base_ws)
	cw = base_ws u
	q  = quote ? "\"" : ""
	printf "%sanimation = {\n", base_ws
	printf "%sanimationmaskfile = %s%s%s\n", cw, q, mask, q
	printf "%sanimationtexturefile = %s%s%s\t# <- the animated file\n", cw, q, overlay, q
	printf "%sanimationrotation = 90\t\t# -90 clockwise 90 counterclockwise(by default)\n", cw
	printf "%sanimationlooping = yes\t\t\t# yes or no ;)\n", cw
	printf "%sanimationtime = 1.6\t\t\t\t# in seconds\n", cw
	printf "%sanimationdelay = 3\t\t\t# in seconds\n", cw
	printf "%sanimationblendmode = \"add\"\t\t# add, multiply, overlay\n", cw
	printf "%sanimationtype = \"pulsing\"\t\t# scrolling, rotating, pulsing\n", cw
	printf "%sanimationrotationoffset = { x = 0.0 y = 0.0 }\n", cw
	printf "%sanimationtexturescale = { x = 1.0 y = 1.0 }\n", cw
	printf "%s}\n", base_ws
}

# 展开单行写法 spriteType = { name = "..." textureFile = "..." }
function expand_inline_block(line,   ws, u, cw, name, tex, name_q, tex_q) {
	ws   = leading_ws(line)
	u    = indent_unit(ws)
	cw   = ws u
	name = value_after(line, "[Nn]ame[ \t]*=[ \t]*")
	name_q = v_quoted
	tex  = value_after(line, "[Tt]exture[Ff]ile[ \t]*=[ \t]*")
	tex_q = v_quoted
	printf "%sspriteType = {\n", ws
	if (name != "") printf "%sname = %s%s%s\n", cw, (name_q ? "\"" : ""), name, (name_q ? "\"" : "")
	if (tex  != "") printf "%stexturefile = %s%s%s\n", cw, (tex_q ? "\"" : ""), tex, (tex_q ? "\"" : "")
	if (tex  != "") emit_animation(cw, tex, tex_q)
	printf "%s}\n", ws
}

{ lines[NR] = $0 }

END {
	depth     = 0   # 花括号嵌套层数
	in_sprite = 0   # 当前是否在 spriteType 块里
	block_done = 0  # 当前块是否已带 animation
	total = 0; added = 0; skipped = 0; no_tex = 0

	for (i = 1; i <= NR; i++) {
		line = lines[i]

		# 注释行: 原样输出, 不参与状态机
		if (line ~ /^[ \t]*#/) { print line; continue }

		# 单行写法: spriteType = { name = "..." textureFile = "..." }
		if (line ~ /^[ \t]*spriteType[ \t]*=[ \t]*\{.*\}[ \t]*$/) {
			total++
			if (expand_inline) {
				expand_inline_block(line)
				added++
			} else {
				print line
				skipped++
			}
			continue
		}

		# 多行块的开始行
		if (depth == 0 && line ~ /^[ \t]*spriteType[ \t]*=[ \t]*\{/) {
			in_sprite  = 1
			total++
			block_done = has_animation_ahead(i)
			if (block_done) skipped++
		}

		print line

		# 紧跟 texturefile 之后插入 animation (已带 animation 的块跳过)
		if (in_sprite && !block_done && line ~ /^[ \t]*[Tt]exture[Ff]ile[ \t]*=/) {
			tex = value_after(line, "[Tt]exture[Ff]ile[ \t]*=[ \t]*")
			if (tex != "") {
				emit_animation(leading_ws(line), tex, v_quoted)
				added++
				block_done = 1
			} else {
				no_tex++
			}
		}

		depth += net_braces(line)
		if (depth <= 0) { depth = 0; in_sprite = 0; block_done = 0 }
	}

	printf "add_shine_animation: 共 %d 个 spriteType, 新增 animation %d 个, 跳过 %d 个\n", total, added, skipped > "/dev/stderr"
	if (no_tex > 0)
		printf "add_shine_animation: 警告: %d 个 spriteType 没找到 texturefile, 未处理\n", no_tex > "/dev/stderr"
}
' "$in_file" > "$tmp"

case "$mode" in
	write)
		cp -p -- "$in_file" "$in_file.bak"
		cat "$tmp" > "$in_file"
		echo "已写入 $in_file (原始文件备份为 $in_file.bak)" >&2
		;;
	output)
		cat "$tmp" > "$out_file"
		echo "已写入 $out_file" >&2
		;;
	stdout)
		cat "$tmp"
		;;
esac
