#!/usr/bin/env bash
set -euo pipefail

# must try load mod id: 1623423360
# optional load mod id(if exists): 3387068412, 2841481791, 1595876588
# mod load priority: 1623423360, 1595876588, touhou_mod(project root), 2841481791, 3387068412

readonly STELLARIS_APP_ID=281990
readonly MUST_TRY_MOD_ID=1623423360
readonly -a OPTIONAL_MODS_BEFORE_REPO=(1595876588)
readonly -a OPTIONAL_MODS_AFTER_REPO=(2841481791 3387068412)

usage() {
    cat <<'EOF'
usage: run-game.sh [-f] [-d] [-c] [--inline_hook=<hook.so>[,<hook.so>...]] [game arguments...]

  -f                    also load optional Workshop mods when installed
  -d                    do not pass -mod; use the game's dlc_load.json
  -c                    continue the most recent save (--continuelastsave)
  --inline_hook=<list>  preload one or more .so files via LD_PRELOAD

The --inline_hook option may be repeated. Remaining arguments are passed
through to the Stellaris executable.
EOF
}

die() {
    echo "error: $*" >&2
    exit 1
}

canonical_dir() {
    (
        cd -- "$1" 2>/dev/null
        pwd -P
    )
}

is_elf() {
    local magic

    [[ -f "$1" ]] || return 1
    magic="$(od -An -tx1 -N4 -- "$1" 2>/dev/null | tr -d '[:space:]')"
    [[ "$magic" == "7f454c46" ]]
}

is_game_dir() {
    local dir="${1:-}"

    [[ -n "$dir" && -d "$dir" ]] || return 1
    [[ -x "$dir/stellaris" ]] || return 1
    is_elf "$dir/stellaris"
}

collect_steam_libraries() {
    local root
    local vdf
    local library
    local -a roots=()

    if [[ -n "${STEAM_ROOT:-}" ]]; then
        roots+=("$STEAM_ROOT")
    fi
    if [[ -n "${STEAM_LIBRARY:-}" ]]; then
        roots+=("$STEAM_LIBRARY")
    fi

    roots+=(
        "$HOME/.steam/steam"
        "$HOME/.steam/root"
        "$HOME/.local/share/Steam"
        "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam"
        "$HOME/SteamLibrary"
    )

    for root in "${roots[@]}"; do
        [[ -d "$root" ]] || continue
        printf '%s\n' "$root"

        vdf="$root/steamapps/libraryfolders.vdf"
        [[ -f "$vdf" ]] || continue

        while IFS= read -r library; do
            [[ -n "$library" ]] && printf '%s\n' "$library"
        done < <(
            awk -F'"' '
                /^[[:space:]]*"path"/ {
                    print $4
                }
            ' "$vdf"
        )
    done
}

find_game_dir() {
    local candidate
    local library
    local dir
    local -a steam_libraries=()

    if [[ -n "${GAMEDIR:-}" ]]; then
        if is_game_dir "$GAMEDIR"; then
            canonical_dir "$GAMEDIR"
            return 0
        fi
        echo "warning: GAMEDIR is not a valid Stellaris game directory: $GAMEDIR" >&2
    fi

    dir="$(pwd -P)"
    while :; do
        if is_game_dir "$dir"; then
            printf '%s\n' "$dir"
            return 0
        fi
        [[ "$dir" == "/" ]] && break
        dir="$(dirname -- "$dir")"
    done

    mapfile -t steam_libraries < <(
        collect_steam_libraries | awk 'NF && !seen[$0]++'
    )

    for library in "${steam_libraries[@]}"; do
        candidate="$library/steamapps/common/Stellaris"
        if is_game_dir "$candidate"; then
            canonical_dir "$candidate"
            return 0
        fi
    done

    return 1
}

find_workshop_dir() {
    local candidate
    local game_library
    local library
    local -a steam_libraries=()

    if [[ -n "${STEAM_WORKSHOP:-}" ]]; then
        if [[ -d "$STEAM_WORKSHOP" ]]; then
            canonical_dir "$STEAM_WORKSHOP"
            return 0
        fi
        echo "warning: STEAM_WORKSHOP does not exist: $STEAM_WORKSHOP" >&2
    fi

    game_library="$(canonical_dir "$GAMEDIR/../../..")"
    candidate="$game_library/steamapps/workshop/content/$STELLARIS_APP_ID"
    if [[ -d "$candidate" ]]; then
        canonical_dir "$candidate"
        return 0
    fi

    mapfile -t steam_libraries < <(
        collect_steam_libraries | awk 'NF && !seen[$0]++'
    )

    for library in "${steam_libraries[@]}"; do
        candidate="$library/steamapps/workshop/content/$STELLARIS_APP_ID"
        if [[ -d "$candidate" ]]; then
            canonical_dir "$candidate"
            return 0
        fi
    done

    return 1
}

resolve_inline_hook() {
    local hook="$1"

    if [[ -f "$hook" ]]; then
        realpath -- "$hook"
        return 0
    fi

    if [[ "$hook" != /* && -f "$repo_root/$hook" ]]; then
        realpath -- "$repo_root/$hook"
        return 0
    fi

    return 1
}

add_inline_hooks() {
    local value="$1"
    local hook
    local resolved
    local -a requested_hooks=()

    [[ -n "$value" ]] || die "option --inline_hook requires a non-empty value"

    IFS=',' read -r -a requested_hooks <<< "$value"
    for hook in "${requested_hooks[@]}"; do
        [[ -n "$hook" ]] || die "empty path in --inline_hook"
        [[ "$hook" == *.so ]] || die "inline hook is not a .so file: $hook"

        resolved="$(resolve_inline_hook "$hook")" \
            || die "cannot find inline hook: $hook"
        inline_hooks+=("$resolved")
    done
}

append_workshop_mod() {
    local mod_id="$1"
    local required="$2"
    local mod_path="$STEAM_WORKSHOP/$mod_id"

    if [[ ! -d "$mod_path" ]]; then
        if [[ "$required" == true ]]; then
            die "required Workshop mod is not installed: $mod_id"
        fi
        echo "warning: optional Workshop mod is not installed, skipping: $mod_id" >&2
        return 0
    fi

    mod_args+=("-mod=$(canonical_dir "$mod_path")")
}

script_path="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/$(basename -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname -- "$script_path")"
repo_root="$(git -C "$script_dir" rev-parse --show-toplevel 2>/dev/null)" \
    || die "could not locate the repository root"

load_optional=false
use_dlc_load=false
continue_save=false
inline_hooks=()
mod_args=()
game_args=()

while (( $# > 0 )); do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        -f)
            load_optional=true
            shift
            ;;
        -d)
            use_dlc_load=true
            shift
            ;;
        -c)
            continue_save=true
            shift
            ;;
        --inline_hook=*)
            add_inline_hooks "${1#--inline_hook=}"
            shift
            ;;
        --inline_hook)
            (( $# >= 2 )) || die "option --inline_hook requires an argument"
            add_inline_hooks "$2"
            shift 2
            ;;
        --)
            shift
            game_args+=("$@")
            break
            ;;
        *)
            game_args+=("$1")
            shift
            ;;
    esac
done

if [[ "$use_dlc_load" == true && "$load_optional" == true ]]; then
    echo "warning: -f has no effect with -d" >&2
fi

GAMEDIR="$(find_game_dir)" || die "could not locate the Stellaris game directory"
STEAM_WORKSHOP=""

if [[ "$use_dlc_load" == false ]]; then
    STEAM_WORKSHOP="$(find_workshop_dir)" \
        || die "could not locate the Stellaris Steam Workshop directory"

    append_workshop_mod "$MUST_TRY_MOD_ID" true

    if [[ "$load_optional" == true ]]; then
        for mod_id in "${OPTIONAL_MODS_BEFORE_REPO[@]}"; do
            append_workshop_mod "$mod_id" false
        done
    fi

    [[ -f "$repo_root/descriptor.mod" ]] \
        || die "project root does not contain descriptor.mod: $repo_root"
    mod_args+=("-mod=$repo_root")

    if [[ "$load_optional" == true ]]; then
        for mod_id in "${OPTIONAL_MODS_AFTER_REPO[@]}"; do
            append_workshop_mod "$mod_id" false
        done
    fi
fi

launch_args=("${mod_args[@]}")

if [[ "$continue_save" == true ]]; then
    launch_args+=(--continuelastsave)
fi

launch_args+=("${game_args[@]}")

if (( ${#inline_hooks[@]} > 0 )); then
    preload_path="${inline_hooks[0]}"
    for hook in "${inline_hooks[@]:1}"; do
        preload_path+=":$hook"
    done
    export LD_PRELOAD="${preload_path}${LD_PRELOAD:+:$LD_PRELOAD}"
fi

echo "game directory: $GAMEDIR"
if [[ "$use_dlc_load" == true ]]; then
    echo "mod loading: dlc_load.json"
else
    echo "mod loading: command line"
fi
if [[ "$continue_save" == true ]]; then
    echo "save loading: most recent save"
fi

cd "$GAMEDIR"
exec "$GAMEDIR/stellaris" "${launch_args[@]}"
