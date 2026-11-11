#!/usr/bin/env bash
set -euo pipefail

# what this should do:
# 1. if not in stellaris game dir, locate stellaris game-dir ($GAMEDIR)
# 2. locate steam workshop ($STEAM_WORKSHOP)
# 3. locate paradox stellaris mod-dir (#PARADOX_MODDIR)
# 4. if step 1 is executed, self-deploy this script to game-dir
# 5. if not in stellaris game dir but already deployed, try handover -mod=<mod> argument to the deployed script
# 6. try engage stellaris particle editor
# note:
# editor should support: -mod=<mod> argument
# first, treat <mod> as mod id, and try to find $MOUNT_PATH=$STEAM_WORKSHOP/<mod> dir.
# if not find, try to find $MOUNT_PATH=$PARADOX_MODDIR/<mod> dir
# if still not find, check if $MOUNT_PATH=<mod> dir exists
# if all failed, warning and exists
# if any dir exists, engage stellaris game with:
# <stellaris-executable> -editor -filterfile=$GAMEDIR/previewer_assets/previewer_filefilter.txt -mount=$MOUNT_PATH
# and <stellaris-executable> should be an ELF file named "stellaris" in game-dir

readonly STELLARIS_APP_ID=281990

usage() {
    cat <<'EOF'
usage: editor.sh -mod=<mod>

  <mod> can be a Steam Workshop ID, a mod directory under the Paradox
  Stellaris mod directory, or an explicit mod directory path.
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
    [[ -f "$dir/previewer_assets/previewer_filefilter.txt" ]] || return 1
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

    if is_game_dir "$script_dir"; then
        printf '%s\n' "$script_dir"
        return 0
    fi

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

find_paradox_mod_dir() {
    local candidate
    local data_home="${XDG_DATA_HOME:-$HOME/.local/share}"
    local -a candidates=()

    if [[ -n "${PARADOX_MODDIR:-}" ]]; then
        candidates+=("$PARADOX_MODDIR")
    fi

    candidates+=(
        "$data_home/Paradox Interactive/Stellaris/mod"
        "$HOME/.paradoxinteractive/Stellaris/mod"
        "$HOME/Documents/Paradox Interactive/Stellaris/mod"
    )

    for candidate in "${candidates[@]}"; do
        if [[ -d "$candidate" ]]; then
            canonical_dir "$candidate"
            return 0
        fi
    done

    return 1
}

resolve_mount_path() {
    local candidate

    if [[ -n "$STEAM_WORKSHOP" ]]; then
        candidate="$STEAM_WORKSHOP/$mod_argument"
        if [[ -d "$candidate" ]]; then
            canonical_dir "$candidate"
            return 0
        fi
    fi

    if [[ -n "$PARADOX_MODDIR" ]]; then
        candidate="$PARADOX_MODDIR/$mod_argument"
        if [[ -d "$candidate" ]]; then
            canonical_dir "$candidate"
            return 0
        fi
    fi

    if [[ -d "$mod_argument" ]]; then
        canonical_dir "$mod_argument"
        return 0
    fi

    return 1
}

mod_argument=""

while (( $# > 0 )); do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        -mod=*)
            mod_argument="${1#-mod=}"
            shift
            ;;
        -mod)
            (( $# >= 2 )) || die "option -mod requires an argument"
            mod_argument="$2"
            shift 2
            ;;
        *)
            die "unknown argument: $1"
            ;;
    esac
done

[[ -n "$mod_argument" ]] || {
    usage >&2
    exit 2
}

script_path="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)/$(basename -- "${BASH_SOURCE[0]}")"
script_dir="$(dirname -- "$script_path")"

step1_executed=false

if is_game_dir "$(pwd -P)"; then
    GAMEDIR="$(pwd -P)"
elif is_game_dir "$script_dir"; then
    GAMEDIR="$script_dir"
else
    GAMEDIR="$(find_game_dir)" || die "could not locate the Stellaris game directory"
    step1_executed=true
fi

STEAM_WORKSHOP="$(find_workshop_dir || true)"
PARADOX_MODDIR="$(find_paradox_mod_dir || true)"

MOUNT_PATH="$(resolve_mount_path || true)"
if [[ -z "$MOUNT_PATH" ]]; then
    die "could not locate mod '$mod_argument' in Workshop, Paradox mod directory, or as a path"
fi

if [[ "$step1_executed" == true ]]; then
    deployed_script="$GAMEDIR/editor.sh"
    if [[ "$script_path" != "$deployed_script" ]] && ! cmp -s "$script_path" "$deployed_script"; then
        install -m 0755 "$script_path" "$deployed_script" \
            || die "could not deploy editor.sh to: $deployed_script"
        echo "deployed editor.sh to: $deployed_script"
    fi
fi

echo "game directory: $GAMEDIR"
echo "Steam Workshop: ${STEAM_WORKSHOP:-not found}"
echo "Paradox mod directory: ${PARADOX_MODDIR:-not found}"
echo "mount path: $MOUNT_PATH"

cd "$GAMEDIR"
exec "$GAMEDIR/stellaris" \
    -editor \
    "-filterfile=$GAMEDIR/previewer_assets/previewer_filefilter.txt" \
    "-mount=$MOUNT_PATH"
