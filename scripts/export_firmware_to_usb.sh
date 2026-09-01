#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

board="${SENSORWATCH_BOARD:-sensorwatch_pro}"
display="${SENSORWATCH_DISPLAY:-custom}"
build_jobs="${BUILD_JOBS:-2}"
firmware_path="build/firmware.uf2"
usb_path="${1:-}"

if [[ -z "$usb_path" ]]; then
    media_root="/media/$(id -un)"
    shopt -s nullglob
    mounted_volumes=("$media_root"/*)
    shopt -u nullglob

    if [[ ${#mounted_volumes[@]} -ne 1 ]]; then
        echo "Expected one USB volume under $media_root; found ${#mounted_volumes[@]}." >&2
        echo "Pass the destination explicitly: $0 /path/to/usb" >&2
        exit 1
    fi

    usb_path="${mounted_volumes[0]}"
fi

echo "Cleaning previous build..."
make clean

echo "Building firmware for BOARD=$board DISPLAY=$display..."
make "BOARD=$board" "DISPLAY=$display" -j"$build_jobs"

if [[ ! -f "$firmware_path" ]]; then
    echo "Build completed without producing $firmware_path" >&2
    exit 1
fi

if [[ ! -d "$usb_path" || ! -w "$usb_path" ]]; then
    echo "USB destination is not a writable directory: $usb_path" >&2
    exit 1
fi

next_version=1
shopt -s nullglob
for existing_path in "$usb_path"/firmware_v[0-9][0-9].uf2; do
    existing_name="${existing_path##*/}"
    version_text="${existing_name#firmware_v}"
    version_text="${version_text%.uf2}"
    version_number=$((10#$version_text))
    if (( version_number >= next_version )); then
        next_version=$((version_number + 1))
    fi
done
shopt -u nullglob

if (( next_version > 99 )); then
    echo "Version limit reached: firmware_v99.uf2 already exists." >&2
    exit 1
fi

destination="$usb_path/$(printf 'firmware_v%02d.uf2' "$next_version")"
cp --update=none -- "$firmware_path" "$destination"
sync "$destination"

echo "Copied firmware to: $destination"
