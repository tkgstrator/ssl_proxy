#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
work_dir="$(mktemp -d "$repo_root/.fetch-dependencies.XXXXXX")"
trap 'rm -rf "$work_dir"' EXIT

fetch_archive() {
    local name="$1" url="$2" revision="$3" destination="$4"
    local clone_dir="$work_dir/$name.git" stage_dir="$work_dir/$name"

    if [[ -e "$destination" ]]; then
        if [[ -f "$destination/.source-commit" ]] &&
           [[ "$(cat "$destination/.source-commit")" == "$revision" ]]; then
            printf '%s: already at %s\n' "$name" "$revision"
            return
        fi
        printf '%s: existing %s has no matching revision marker; inspect it before retrying\n' "$name" "$destination" >&2
        exit 1
    fi

    printf '%s: fetching %s\n' "$name" "$revision"
    git init -q "$clone_dir"
    git -C "$clone_dir" fetch -q --depth=1 "$url" "$revision"
    if [[ "$(git -C "$clone_dir" rev-parse FETCH_HEAD)" != "$revision" ]]; then
        printf '%s: fetched revision mismatch\n' "$name" >&2
        exit 1
    fi
    mkdir -p "$stage_dir"
    git -C "$clone_dir" archive --format=tar FETCH_HEAD | tar -xf - -C "$stage_dir"
    printf '%s\n' "$revision" > "$stage_dir/.source-commit"
    mkdir -p "$(dirname "$destination")"
    mv "$stage_dir" "$destination"
}

fetch_archive "Atmosphere-libs" \
    "https://github.com/Atmosphere-NX/Atmosphere-libs.git" \
    "cf48d23375cf36742876f4b014971b2ebabc6dc8" \
    "$repo_root/Atmosphere-libs"
fetch_archive "libnx" \
    "https://github.com/switchbrew/libnx.git" \
    "b156b4774fb2df7b93402169509266da7547fc4c" \
    "$repo_root/.deps/libnx"
