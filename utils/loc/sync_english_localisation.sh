#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/../.." && pwd)"
source_dir="$repo_root/localisation/english"
localisation_dir="$repo_root/localisation"

locales=(
    braz_por
    french
    german
    japanese
    korean
    polish
    russian
    spanish
)

excluded_target="russian/replace/spth_vanilla_repair_l_russian.yml"
dry_run=0

usage() {
    printf 'Usage: %s [--dry-run]\n' "$(basename -- "$0")"
}

while (($# > 0)); do
    case "$1" in
        --dry-run)
            dry_run=1
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            printf 'Unknown argument: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

if [[ ! -d "$source_dir" ]]; then
    printf 'English localisation directory not found: %s\n' "$source_dir" >&2
    exit 1
fi

created=0
updated=0
unchanged=0
excluded=0

while IFS= read -r -d '' source_file; do
    relative_path="${source_file#"$source_dir"/}"

    for locale in "${locales[@]}"; do
        target_relative="$locale/${relative_path/_l_english.yml/_l_${locale}.yml}"

        if [[ "$target_relative" == "$excluded_target" ]]; then
            ((excluded += 1))
            continue
        fi

        target_file="$localisation_dir/$target_relative"
        target_directory="$(dirname -- "$target_file")"

        if ((dry_run)); then
            if [[ -e "$target_file" ]]; then
                printf 'Would update: %s\n' "$target_relative"
            else
                printf 'Would create: %s\n' "$target_relative"
            fi
            continue
        fi

        mkdir -p -- "$target_directory"
        temp_file="$(mktemp "$target_directory/.sync-english.XXXXXX")"

        sed "1s/l_english:/l_${locale}:/" "$source_file" > "$temp_file"

        if [[ -f "$target_file" ]] && cmp -s -- "$temp_file" "$target_file"; then
            rm -f -- "$temp_file"
            ((unchanged += 1))
            continue
        fi

        if [[ -e "$target_file" ]]; then
            ((updated += 1))
        else
            ((created += 1))
        fi

        mv -f -- "$temp_file" "$target_file"
    done
done < <(find "$source_dir" -type f -name '*_l_english.yml' -print0 | sort -z)

if ((dry_run)); then
    printf 'Dry run complete; excluded %d target files.\n' "$excluded"
else
    printf 'Synchronised English localisation: %d created, %d updated, %d unchanged, %d excluded.\n' \
        "$created" "$updated" "$unchanged" "$excluded"
fi
