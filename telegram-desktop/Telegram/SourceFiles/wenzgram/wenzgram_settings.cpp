/*
This file is part of Wenzgram,
a Telegram Desktop fork with extended client settings.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_settings.h"

#include "core/application.h"
#include "core/core_settings.h"

namespace Wenzgram {
namespace {

rpl::event_stream<> Changes;

[[nodiscard]] bool Read(std::string_view key, bool fallback) {
	return Core::App().settings().readPref<bool>(key, fallback);
}

void Write(std::string_view key, bool value) {
	Core::App().settings().writePref<bool>(key, value);
	Core::App().saveSettingsDelayed();
	Changes.fire({});
}

} // namespace

bool readBool(std::string_view key, bool fallback) {
	return Read(key, fallback);
}

void writeBool(std::string_view key, bool value) {
	Write(key, value);
}

bool localChatWallpapersEnabled() {
	return Read(kLocalChatWallpapersKey, true);
}

bool showBranding() {
	return Read(kShowBrandingKey, true);
}

bool compactDialogs() {
	return Read(kCompactDialogsKey, false);
}

bool disableChatAnimations() {
	return Read(kDisableChatAnimationsKey, false);
}

bool hideEditedBadge() {
	return Read(kHideEditedBadgeKey, false);
}

bool confirmBeforeSend() {
	return Read(kConfirmBeforeSendKey, false);
}

bool showSecondsInTime() {
	return Read(kShowSecondsInTimeKey, false);
}

bool copyUsernameOnClick() {
	return Read(kCopyUsernameOnClickKey, false);
}

bool hideChatFolders() {
	return Read(kHideChatFoldersKey, false);
}

bool largeEmoji() {
	return Read(kLargeEmojiKey, false);
}

rpl::producer<bool> localChatWallpapersEnabledValue() {
	return rpl::single(
		localChatWallpapersEnabled()
	) | rpl::then(
		settingsChanged() | rpl::map([] { return localChatWallpapersEnabled(); })
	);
}

rpl::producer<> settingsChanged() {
	return Changes.events();
}

void notifySettingsChanged() {
	Changes.fire({});
}

} // namespace Wenzgram
