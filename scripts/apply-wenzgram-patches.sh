#!/usr/bin/env bash
set -euo pipefail

# Applies Wenzgram patches onto a fresh tdesktop checkout.
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
	"Telegram/SourceFiles/wenzgram/wenzgram_version.h"
	"Telegram/SourceFiles/wenzgram/wenzgram_settings.h"
	"Telegram/SourceFiles/wenzgram/wenzgram_settings.cpp"
	"Telegram/SourceFiles/wenzgram/wenzgram_updater.h"
	"Telegram/SourceFiles/wenzgram/wenzgram_updater.cpp"
	"Telegram/SourceFiles/wenzgram/wenzgram_update_runner_win.cpp"
	"Telegram/SourceFiles/wenzgram/wenzgram_deleted_messages.h"
	"Telegram/SourceFiles/wenzgram/wenzgram_deleted_messages.cpp"
	"Telegram/SourceFiles/wenzgram/wenzgram_local_wallpapers.h"
	"Telegram/SourceFiles/wenzgram/wenzgram_local_wallpapers.cpp"
	"Telegram/SourceFiles/wenzgram/wenzgram_local_wallpaper_box.h"
	"Telegram/SourceFiles/wenzgram/wenzgram_local_wallpaper_box.cpp"
	"Telegram/SourceFiles/wenzgram/wenzgram_profile_nft.h"
	"Telegram/SourceFiles/wenzgram/wenzgram_profile_nft.cpp"
	"Telegram/SourceFiles/wenzgram/wenzgram_profile_nft_box.h"
	"Telegram/SourceFiles/wenzgram/wenzgram_profile_nft_box.cpp"
	"Telegram/SourceFiles/settings/sections/settings_wenzgram.h"
	"Telegram/SourceFiles/settings/sections/settings_wenzgram.cpp"
	"Telegram/SourceFiles/settings/sections/settings_main.cpp"
	"Telegram/SourceFiles/settings/sections/settings_information.cpp"
	"Telegram/SourceFiles/info/profile/info_profile_top_bar.cpp"
	"Telegram/SourceFiles/data/data_session.cpp"
	"Telegram/SourceFiles/history/history_item.cpp"
	"Telegram/SourceFiles/history/view/history_view_element.cpp"
	"Telegram/SourceFiles/history/view/history_view_message.cpp"
	"Telegram/SourceFiles/dialogs/dialogs_row.cpp"
	"Telegram/SourceFiles/window/main_window.cpp"
	"Telegram/SourceFiles/history/view/history_view_bottom_info.cpp"
	"Telegram/SourceFiles/mainwidget.cpp"
	"Telegram/SourceFiles/window/section_widget.cpp"
	"Telegram/SourceFiles/window/window_peer_menu.cpp"
	"Telegram/CMakeLists.txt"
)

for file in "${FILES[@]}"; do
	copy_file "$file"
done

echo "All Wenzgram patches applied."
