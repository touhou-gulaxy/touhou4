#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
usage: git-tag.sh [-f] [-m <commit>] [tag]

  -m <commit>  tag the specified commit instead of the branch in dev_branch.txt
  -f           force an existing tag to point to the selected commit

If tag is omitted, v<descriptor.mod version> is used.
EOF
}

force=false
manual_commit=""

while getopts ":fm:h" option; do
    case "$option" in
        f)
            force=true
            ;;
        m)
            manual_commit="$OPTARG"
            ;;
        h)
            usage
            exit 0
            ;;
        :)
            echo "option -$OPTARG requires an argument" >&2
            usage >&2
            exit 2
            ;;
        \?)
            echo "unknown option: -$OPTARG" >&2
            usage >&2
            exit 2
            ;;
    esac
done

shift $((OPTIND - 1))

if (( $# > 1 )); then
    usage >&2
    exit 2
fi

tag="${1:-}"

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(git -C "$script_dir" rev-parse --show-toplevel)"
dev_branch_file="$repo_root/utils/git/dev_branch.txt"
descriptor_file="$repo_root/descriptor.mod"

if [[ -n "$manual_commit" ]]; then
    target_ref="$manual_commit"
else
    if [[ ! -f "$dev_branch_file" ]]; then
        echo "missing development branch file: $dev_branch_file" >&2
        exit 1
    fi

    target_ref="$(
        sed -n '1{
            s/^[[:space:]]*//
            s/[[:space:]]*$//
            p
        }' "$dev_branch_file"
    )"

    if [[ -z "$target_ref" ]]; then
        echo "development branch file is empty: $dev_branch_file" >&2
        exit 1
    fi
fi

if ! target_commit="$(git -C "$repo_root" rev-parse --verify "${target_ref}^{commit}" 2>/dev/null)"; then
    echo "target does not resolve to a commit: $target_ref" >&2
    exit 1
fi

if [[ -z "$tag" ]]; then
    if [[ ! -f "$descriptor_file" ]]; then
        echo "missing descriptor file: $descriptor_file" >&2
        exit 1
    fi

    version="$(
        awk -F'"' '
            /^[[:space:]]*version[[:space:]]*=/ {
                print $2
                exit
            }
        ' "$descriptor_file"
    )"

    if [[ -z "$version" ]]; then
        echo "could not read version from: $descriptor_file" >&2
        exit 1
    fi

    tag="v$version"
fi

tag_ref="refs/tags/$tag"

if ! git -C "$repo_root" check-ref-format "$tag_ref"; then
    echo "invalid tag name: $tag" >&2
    exit 1
fi

if existing_commit="$(git -C "$repo_root" rev-parse --verify "${tag_ref}^{commit}" 2>/dev/null)"; then
    if [[ "$existing_commit" == "$target_commit" && "$force" == false ]]; then
        git -C "$repo_root" push origin "$tag_ref"
        echo "tag already exists at $target_ref: $tag"
        exit 0
    fi

    if [[ "$force" == false ]]; then
        echo "tag already exists at another commit: $tag" >&2
        echo "  existing: $existing_commit" >&2
        echo "  target:   $target_commit ($target_ref)" >&2
        exit 1
    fi
fi

if [[ "$force" == true ]]; then
    git -C "$repo_root" tag -f -a "$tag" "$target_commit" \
        -m "Force tag $tag at $target_commit ($target_ref)"

    git -C "$repo_root" push --force origin "$tag_ref"
else
    git -C "$repo_root" tag -a "$tag" "$target_commit" \
        -m "Tag $tag at $target_commit ($target_ref)"

    git -C "$repo_root" push origin "$tag_ref"
fi

echo "tagged $target_commit ($target_ref) as $tag"
