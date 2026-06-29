/*
This file is part of Wenzgram,
a Telegram Desktop fork with extended client settings.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_settings.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "data/data_session.h"
#include "main/main_account.h"
#include "main/main_domain.h"

#include <QLocale>

namespace Wenzgram {
namespace {

rpl::event_stream<> Changes;

[[nodiscard]] bool Read(std::string_view key, bool fallback) {
	return Core::App().settings().readPref<bool>(key, fallback);
}

void RefreshDialogsLayout() {
	Core::App().domain().enumerateAccounts([&](not_null<Main::Account*> account) {
		if (const auto session = account->maybeSession()) {
			session->data().chatsList()->indexed()->updateHeights(0);
		}
	});
}

void ApplySetting(std::string_view key, bool value) {
	if (key == kLargeEmojiKey) {
		Core::App().settings().setLargeEmoji(value);
	} else if (key == kShowBrandingKey) {
		Core::App().updateWindowTitles();
	} else if (key == kCompactDialogsKey) {
		RefreshDialogsLayout();
	}
}

void Write(std::string_view key, bool value) {
	Core::App().settings().writePref<bool>(key, value);
	Core::App().saveSettingsDelayed();
	ApplySetting(key, value);
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

bool showDeletedMessages() {
	return Read(kShowDeletedMessagesKey, true);
}

bool autoUpdateEnabled() {
	return Read(kAutoUpdateKey, true);
}

bool profileNftEnabled() {
	return Read(kProfileNftKey, true);
}

rpl::producer<bool> localChatWallpapersEnabledValue() {
	return rpl::single(
		localChatWallpapersEnabled()
	) | rpl::then(
		settingsChanged() | rpl::map([] { return localChatWallpapersEnabled(); })
	);
}

rpl::producer<bool> profileNftEnabledValue() {
	return rpl::single(
		profileNftEnabled()
	) | rpl::then(
		settingsChanged() | rpl::map([] { return profileNftEnabled(); })
	);
}

rpl::producer<> settingsChanged() {
	return Changes.events();
}

void notifySettingsChanged() {
	Changes.fire({});
}

void syncCoreSettings() {
	Core::App().settings().setLargeEmoji(largeEmoji());
	Core::App().updateWindowTitles();
	RefreshDialogsLayout();
}

QString formatTime(const QTime &time) {
	if (showSecondsInTime()) {
		return time.toString(u"HH:mm:ss"_q);
	}
	return QLocale().toString(time, QLocale::ShortFormat);
}

QString formatTime(const QDateTime &date) {
	return formatTime(date.time());
}

} // namespace Wenzgram
