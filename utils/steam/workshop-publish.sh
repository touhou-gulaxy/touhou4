#!/usr/bin/env bash
set -Eeuo pipefail

readonly STELLARIS_APP_ID="281990"
readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly MOD_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd -P)"
readonly DESCRIPTOR_FILE="$MOD_ROOT/descriptor.mod"

usage() {
	cat <<'EOF'
通过 SteamCMD 直接更新或创建 Stellaris 创意工坊物品，不启动游戏或 Paradox Launcher。

用法：
  utils/steam/workshop-publish.sh [选项]

选项：
  --dry-run                 仅生成配置并显示上传计划，不访问 Steam
  --print-vdf               显示生成的 workshop VDF
  --vdf-out FILE            将生成的 VDF 保存到指定路径
  --user STEAM_USER          SteamCMD 登录名；也可设置 STEAM_USERNAME
  --steamcmd PATH           指定 steamcmd.sh 或 steamcmd；也可设置 STEAMCMD
  --install-steamcmd        下载并安装 SteamCMD 到 XDG_DATA_HOME 后退出
  --content DIR             上传此目录，默认为模组仓库根目录
  --desc FILE               工坊描述文件；第一行作为备用标题
  --preview IMAGE           上传并替换工坊封面；更新已有物品时可省略
  --title TEXT              覆盖工坊标题
  --notes TEXT              设置本次更新说明
  --notes-file FILE         从文件读取本次更新说明
  --visibility VALUE        public、friends、private、unlisted，默认 public
  --published-file-id ID    覆盖 descriptor.mod 中的 remote_file_id
  --create                  创建新工坊物品，此时必须提供 --preview
  --no-tags                 不在 VDF 中写入工坊标签
  -h, --help                显示帮助

示例：
  # 查看将上传的配置和命令
  utils/steam/workshop-publish.sh --dry-run --print-vdf

  # 更新现有工坊物品；SteamCMD 会提示输入密码和 Steam Guard
  STEAM_USERNAME=your_account \
    utils/steam/workshop-publish.sh --notes "修复若干问题"

  # 指定新的封面和更新说明
  STEAM_USERNAME=your_account utils/steam/workshop-publish.sh \
    --preview ../touhou_resources/workshop-preview.png \
    --notes-file ../touhou_resources/update-notes.txt

注意：
  * 密码不会由本脚本接收、记录或写入命令行。
  * 首次登录后 SteamCMD 可能缓存会话；账号仍须拥有 Stellaris 并有权编辑目标工坊物品。
  * contentfolder 中的全部文件都会参与上传，包括当前仓库里的文档、工具和备份目录。
  * 5.4 GB 的首次内容扫描可能耗时较长；后续上传由 SteamPipe 只传输变化内容。
EOF
}

fail() {
	printf '错误：%s\n' "$*" >&2
	exit 1
}

info() {
	printf '%s\n' "$*"
}

trim_cr() {
	local value="$1"
	printf '%s' "${value//$'\r'/}"
}

descriptor_value() {
	local wanted="$1"
	awk -v wanted="$wanted" '
		{
			line = $0
			sub(/\r$/, "", line)
		}
		line ~ "^[[:space:]]*" wanted "[[:space:]]*=" {
			sub(/^[^=]*=[[:space:]]*/, "", line)
			if (match(line, /"[^"]*"/)) {
				print substr(line, RSTART + 1, RLENGTH - 2)
			}
			exit
		}
	' "$DESCRIPTOR_FILE"
}

descriptor_tags() {
	awk '
		{
			line = $0
			sub(/\r$/, "", line)
		}
		line ~ /^[[:space:]]*tags[[:space:]]*=/ {
			inside = 1
			next
		}
		inside && line ~ /^[[:space:]]*}/ {
			exit
		}
		inside && match(line, /"[^"]*"/) {
			print substr(line, RSTART + 1, RLENGTH - 2)
		}
	' "$DESCRIPTOR_FILE"
}

description_body() {
	awk '
		NR == 1 {
			sub(/^\357\273\277/, "")
			sub(/\r$/, "")
		}
		!seen_title {
			if (NF > 0) {
				seen_title = 1
			}
			next
		}
		!body_started {
			if (NF == 0) {
				next
			}
			body_started = 1
		}
		{
			sub(/\r$/, "")
			print
		}
	' "$1"
}

description_title() {
	awk '
		NR == 1 {
			sub(/^\357\273\277/, "")
		}
		NF > 0 {
			sub(/\r$/, "")
			print
			exit
		}
	' "$1"
}

find_steamcmd() {
	local candidate
	for candidate in \
		"${STEAMCMD:-}" \
		"$(command -v steamcmd 2>/dev/null || true)" \
		"${XDG_DATA_HOME:-$HOME/.local/share}/steamcmd/steamcmd.sh" \
		"$HOME/.local/share/steamcmd/steamcmd.sh" \
		"$HOME/steamcmd/steamcmd.sh" \
		"$HOME/.steam/steam/steamcmd/steamcmd.sh" \
		"/usr/games/steamcmd" \
		"/usr/bin/steamcmd"; do
		if [[ -n "$candidate" && -f "$candidate" && -x "$candidate" ]]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done
	return 1
}

install_steamcmd() {
	local install_root="${XDG_DATA_HOME:-$HOME/.local/share}/steamcmd"
	local archive

	command -v curl >/dev/null 2>&1 || fail "缺少 curl，无法安装 SteamCMD。"
	command -v tar >/dev/null 2>&1 || fail "缺少 tar，无法安装 SteamCMD。"

	mkdir -p -- "$install_root"
	archive="$(mktemp --suffix=.tar.gz)"

	info "正在下载 SteamCMD 到 $install_root ..."
	if ! curl --fail --location --retry 3 \
		--output "$archive" \
		"https://media.steampowered.com/installer/steamcmd_linux.tar.gz"; then
		rm -f -- "$archive"
		fail "SteamCMD 下载失败。"
	fi
	if ! tar -xzf "$archive" -C "$install_root"; then
		rm -f -- "$archive"
		fail "SteamCMD 解压失败。"
	fi
	rm -f -- "$archive"

	[[ -x "$install_root/steamcmd.sh" ]] || fail "SteamCMD 安装后未找到 steamcmd.sh。"
	info "SteamCMD 已安装，正在执行首次自更新 ..."
	"$install_root/steamcmd.sh" +quit
	info "完成：$install_root/steamcmd.sh"
}

vdf_escape() {
	local value="$1"
	value="${value//\\/\\\\}"
	value="${value//\"/\\\"}"
	printf '%s' "$value"
}

write_vdf() {
	local output_file="$1"
	local index

	{
		printf '"workshopitem"\n'
		printf '{\n'
		printf '    "appid" "%s"\n' "$STELLARIS_APP_ID"
		printf '    "publishedfileid" "%s"\n' "$WORKSHOP_FILE_ID"
		printf '    "contentfolder" "%s"\n' "$(vdf_escape "$WORKSHOP_CONTENT")"
		if [[ -n "$PREVIEW_FILE" ]]; then
			printf '    "previewfile" "%s"\n' "$(vdf_escape "$PREVIEW_FILE")"
		fi
		printf '    "visibility" "%s"\n' "$VISIBILITY_NUMBER"
		printf '    "title" "%s"\n' "$(vdf_escape "$WORKSHOP_TITLE")"
		printf '    "description" "%s"\n' "$(vdf_escape "$WORKSHOP_DESCRIPTION")"
		printf '    "changenote" "%s"\n' "$(vdf_escape "$CHANGE_NOTE")"
		if (( ${#WORKSHOP_TAGS[@]} > 0 )); then
			printf '    "tags"\n'
			printf '    {\n'
			for ((index = 0; index < ${#WORKSHOP_TAGS[@]}; index++)); do
				printf '        "%d" "%s"\n' "$index" \
					"$(vdf_escape "${WORKSHOP_TAGS[$index]}")"
			done
			printf '    }\n'
		fi
		printf '}\n'
	} >"$output_file"
	chmod 600 "$output_file"
}

WORKSHOP_USER="${STEAM_USERNAME:-}"
WORKSHOP_CONTENT="$MOD_ROOT"
WORKSHOP_DESC_FILE=""
WORKSHOP_PREVIEW=""
WORKSHOP_TITLE_OVERRIDE=""
WORKSHOP_NOTES=""
WORKSHOP_NOTES_FILE=""
WORKSHOP_VISIBILITY="public"
WORKSHOP_FILE_ID_OVERRIDE=""
WORKSHOP_DRY_RUN=0
WORKSHOP_PRINT_VDF=0
WORKSHOP_VDF_OUT=""
WORKSHOP_CREATE=0
WORKSHOP_NO_TAGS=0

while (( $# > 0 )); do
	case "$1" in
		-h|--help)
			usage
			exit 0
			;;
		--install-steamcmd)
			install_steamcmd
			exit 0
			;;
		--dry-run)
			WORKSHOP_DRY_RUN=1
			;;
		--print-vdf)
			WORKSHOP_PRINT_VDF=1
			;;
		--create)
			WORKSHOP_CREATE=1
			;;
		--no-tags)
			WORKSHOP_NO_TAGS=1
			;;
		--user|--steamcmd|--content|--desc|--preview|--title|--notes|--notes-file|--visibility|--published-file-id|--vdf-out)
			(( $# >= 2 )) || fail "$1 缺少参数。"
			case "$1" in
				--user) WORKSHOP_USER="$2" ;;
				--steamcmd) STEAMCMD="$2" ;;
				--content) WORKSHOP_CONTENT="$2" ;;
				--desc) WORKSHOP_DESC_FILE="$2" ;;
				--preview) WORKSHOP_PREVIEW="$2" ;;
				--title) WORKSHOP_TITLE_OVERRIDE="$2" ;;
				--notes) WORKSHOP_NOTES="$2" ;;
				--notes-file) WORKSHOP_NOTES_FILE="$2" ;;
				--visibility) WORKSHOP_VISIBILITY="$2" ;;
				--published-file-id) WORKSHOP_FILE_ID_OVERRIDE="$2" ;;
				--vdf-out) WORKSHOP_VDF_OUT="$2" ;;
			esac
			shift
			;;
		*)
			fail "未知选项：$1（使用 --help 查看帮助）"
			;;
	esac
	shift
done

[[ -f "$DESCRIPTOR_FILE" ]] || fail "找不到 $DESCRIPTOR_FILE"
[[ -d "$WORKSHOP_CONTENT" ]] || fail "内容目录不存在：$WORKSHOP_CONTENT"
WORKSHOP_CONTENT="$(cd -- "$WORKSHOP_CONTENT" && pwd -P)"
[[ -f "$WORKSHOP_CONTENT/descriptor.mod" ]] || \
	fail "内容目录中没有 descriptor.mod：$WORKSHOP_CONTENT"

MOD_VERSION="$(descriptor_value version)"
MOD_NAME="$(descriptor_value name)"
MOD_FILE_ID="$(descriptor_value remote_file_id)"
[[ -n "$MOD_NAME" ]] || fail "descriptor.mod 缺少 name。"

if [[ -n "$WORKSHOP_DESC_FILE" ]]; then
	DESC_FILE="$WORKSHOP_DESC_FILE"
else
	DESC_FILE=""
	for candidate in \
		"$MOD_ROOT/../touhou_resources/desc_zh_latest.txt" \
		"$MOD_ROOT/../touhou_resources/desc_cn.txt" \
		"$MOD_ROOT/../touhou_resources/desc_zh.txt" \
		"$MOD_ROOT/../touhou_resources/desc.txt"; do
		if [[ -f "$candidate" ]]; then
			DESC_FILE="$candidate"
			break
		fi
	done
fi
[[ -n "$DESC_FILE" && -f "$DESC_FILE" ]] || fail "找不到工坊描述文件，请用 --desc 指定。"
DESC_FILE="$(cd -- "$(dirname -- "$DESC_FILE")" && pwd -P)/$(basename -- "$DESC_FILE")"

WORKSHOP_TITLE="${WORKSHOP_TITLE_OVERRIDE:-$MOD_NAME}"
if [[ -z "$WORKSHOP_TITLE" ]]; then
	WORKSHOP_TITLE="$(description_title "$DESC_FILE")"
fi
WORKSHOP_DESCRIPTION="$(description_body "$DESC_FILE")"
[[ -n "$WORKSHOP_DESCRIPTION" ]] || fail "工坊描述正文为空：$DESC_FILE"

if (( WORKSHOP_CREATE == 1 )); then
	WORKSHOP_FILE_ID="0"
else
	WORKSHOP_FILE_ID="${WORKSHOP_FILE_ID_OVERRIDE:-$MOD_FILE_ID}"
	[[ "$WORKSHOP_FILE_ID" =~ ^[0-9]+$ && "$WORKSHOP_FILE_ID" != "0" ]] || \
		fail "无法确定现有工坊物品 ID；更新时需要 --published-file-id，创建时请加 --create。"
fi

if [[ -n "$WORKSHOP_PREVIEW" ]]; then
	[[ -f "$WORKSHOP_PREVIEW" ]] || fail "封面文件不存在：$WORKSHOP_PREVIEW"
	PREVIEW_FILE="$(cd -- "$(dirname -- "$WORKSHOP_PREVIEW")" && pwd -P)/$(basename -- "$WORKSHOP_PREVIEW")"
else
	PREVIEW_FILE=""
fi
if (( WORKSHOP_CREATE == 1 )) && [[ -z "$PREVIEW_FILE" ]]; then
	fail "创建新工坊物品时必须用 --preview 指定封面。"
fi

case "$WORKSHOP_VISIBILITY" in
	public) VISIBILITY_NUMBER="0" ;;
	friends|friends-only) VISIBILITY_NUMBER="1" ;;
	private) VISIBILITY_NUMBER="2" ;;
	unlisted) VISIBILITY_NUMBER="3" ;;
	*) fail "--visibility 只能是 public、friends、private 或 unlisted。" ;;
esac

if [[ -n "$WORKSHOP_NOTES_FILE" ]]; then
	[[ -f "$WORKSHOP_NOTES_FILE" ]] || fail "更新说明文件不存在：$WORKSHOP_NOTES_FILE"
	[[ -z "$WORKSHOP_NOTES" ]] || fail "--notes 与 --notes-file 不能同时使用。"
	CHANGE_NOTE="$(<"$WORKSHOP_NOTES_FILE")"
elif [[ -n "$WORKSHOP_NOTES" ]]; then
	CHANGE_NOTE="$WORKSHOP_NOTES"
else
	CHANGE_NOTE="自动工坊更新：${MOD_VERSION:-未知版本}（$(date '+%F')）"
fi

WORKSHOP_TAGS=()
if (( WORKSHOP_NO_TAGS == 0 )); then
	while IFS= read -r tag; do
		tag="$(trim_cr "$tag")"
		[[ -n "$tag" ]] && WORKSHOP_TAGS+=("$tag")
	done < <(descriptor_tags)
fi

VDF_TEMP_DIR="$(mktemp -d)"
trap 'rm -rf -- "$VDF_TEMP_DIR"' EXIT
VDF_FILE="$VDF_TEMP_DIR/workshop_item.vdf"
write_vdf "$VDF_FILE"

if [[ -n "$WORKSHOP_VDF_OUT" ]]; then
	VDF_OUT_DIR="$(cd -- "$(dirname -- "$WORKSHOP_VDF_OUT")" && pwd -P)"
	install -m 600 "$VDF_FILE" "$VDF_OUT_DIR/$(basename -- "$WORKSHOP_VDF_OUT")"
	VDF_FILE="$VDF_OUT_DIR/$(basename -- "$WORKSHOP_VDF_OUT")"
fi

info "模组：$WORKSHOP_TITLE"
info "版本：${MOD_VERSION:-未设置}"
info "工坊物品 ID：$WORKSHOP_FILE_ID"
info "内容目录：$WORKSHOP_CONTENT"
info "描述文件：$DESC_FILE"
if [[ -n "$PREVIEW_FILE" ]]; then
	info "更新封面：$PREVIEW_FILE"
else
	info "更新封面：否（保留工坊现有封面）"
fi
info "可见性：$WORKSHOP_VISIBILITY"
info "标签：${WORKSHOP_TAGS[*]:-不写入}"
info "VDF：$VDF_FILE"

if (( WORKSHOP_PRINT_VDF == 1 )); then
	printf '\n'
	sed -n '1,220p' "$VDF_FILE"
	printf '\n'
fi

if ! WORKSHOP_STEAMCMD="$(find_steamcmd)"; then
	if (( WORKSHOP_DRY_RUN == 1 )); then
		WORKSHOP_STEAMCMD="<未安装>"
	else
		fail "未找到 SteamCMD。可先运行：$0 --install-steamcmd"
	fi
fi

UPLOAD_CMD=("$WORKSHOP_STEAMCMD")
if [[ "$WORKSHOP_STEAMCMD" == "<未安装>" ]]; then
	info "SteamCMD：未安装"
else
	info "SteamCMD：$WORKSHOP_STEAMCMD"
fi

if [[ -n "$WORKSHOP_USER" ]]; then
	UPLOAD_CMD+=(+login "$WORKSHOP_USER")
else
	UPLOAD_CMD+=(+login)
fi
UPLOAD_CMD+=(+workshop_build_item "$VDF_FILE" +quit)

printf '上传命令：'
printf ' %q' "${UPLOAD_CMD[@]}"
printf '\n'

if (( WORKSHOP_DRY_RUN == 1 )); then
	info "dry-run：未登录 Steam，也未上传。"
	exit 0
fi

if [[ -z "$WORKSHOP_USER" ]]; then
	info "未提供 Steam 用户名，SteamCMD 将提示输入用户名与密码。"
fi
info "开始上传；密码和 Steam Guard 由 SteamCMD 交互读取。"
UPLOAD_LOG="$VDF_TEMP_DIR/steamcmd.log"
set +e
"${UPLOAD_CMD[@]}" 2>&1 | tee "$UPLOAD_LOG"
UPLOAD_STATUS="${PIPESTATUS[0]}"
set -e

if (( UPLOAD_STATUS != 0 )); then
	fail "SteamCMD 退出码为 $UPLOAD_STATUS；完整输出如下：$(printf '\n')$(<"$UPLOAD_LOG")"
fi
if grep -Eq 'ERROR!|ClientRemoteStorage\(\)->PublishWorkshopFile\(\) failed|Login Failure|Steam Guard code is incorrect' "$UPLOAD_LOG"; then
	fail "SteamCMD 报告了上传错误；完整输出如下：$(printf '\n')$(<"$UPLOAD_LOG")"
fi

info "SteamCMD 已完成上传流程。"
if [[ "$WORKSHOP_FILE_ID" != "0" ]]; then
	info "工坊页面：https://steamcommunity.com/sharedfiles/filedetails/?id=$WORKSHOP_FILE_ID"
fi
