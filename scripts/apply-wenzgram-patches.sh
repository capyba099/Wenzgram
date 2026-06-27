#!/usr/bin/env bash
set -euo pipefail

# Applies Wenzgram session-protection patches onto a fresh tdesktop checkout.
PATCH_ROOT="${1:?Usage: apply-wenzgram-patches.sh <patch-source> <tdesktop-target>}"
TARGET_ROOT="${2:?Usage: apply-wenzgram-patches.sh <patch-source> <tdesktop-target>}"

copy_file() {
	local rel="$1"
	local src="${PATCH_ROOT}/${rel}"
	local dst="${TARGET_ROOT}/${rel}"
	if [[ ! -f "$src" ]]; then
		echo "Missing patch file: $src" >&2
		exit 1
	fi
	mkdir -p "$(dirname "$dst")"
	cp "$src" "$dst"
	echo "Applied: $rel"
}

FILES=(
	"Telegram/SourceFiles/storage/session_device_binding.h"
	"Telegram/SourceFiles/storage/session_device_binding.cpp"
	"Telegram/SourceFiles/storage/details/storage_file_utilities.h"
	"Telegram/SourceFiles/storage/details/storage_file_utilities.cpp"
	"Telegram/SourceFiles/storage/storage_domain.h"
	"Telegram/SourceFiles/storage/storage_domain.cpp"
	"Telegram/SourceFiles/storage/localstorage.cpp"
	"Telegram/SourceFiles/core/application.h"
	"Telegram/SourceFiles/core/application.cpp"
	"Telegram/SourceFiles/window/window_lock_widgets.cpp"
	"Telegram/CMakeLists.txt"
)

for file in "${FILES[@]}"; do
	copy_file "$file"
done

echo "All Wenzgram patches applied."
