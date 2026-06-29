/*
This file is part of Wenzgram,
a Telegram Desktop fork with extended client settings.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "rpl/producer.h"

namespace Wenzgram {

inline constexpr auto kLocalChatWallpapersKey
	= "wenzgram-local-chat-wallpapers"_cs;
inline constexpr auto kShowBrandingKey
	= "wenzgram-show-branding"_cs;
inline constexpr auto kCompactDialogsKey
	= "wenzgram-compact-dialogs"_cs;
inline constexpr auto kDisableChatAnimationsKey
	= "wenzgram-disable-chat-animations"_cs;
inline constexpr auto kHideEditedBadgeKey
	= "wenzgram-hide-edited-badge"_cs;
inline constexpr auto kConfirmBeforeSendKey
	= "wenzgram-confirm-before-send"_cs;
inline constexpr auto kShowSecondsInTimeKey
	= "wenzgram-show-seconds-in-time"_cs;
inline constexpr auto kCopyUsernameOnClickKey
	= "wenzgram-copy-username-on-click"_cs;
inline constexpr auto kHideChatFoldersKey
	= "wenzgram-hide-chat-folders"_cs;
inline constexpr auto kLargeEmojiKey
	= "wenzgram-large-emoji"_cs;
inline constexpr auto kShowDeletedMessagesKey
	= "wenzgram-show-deleted-messages"_cs;
inline constexpr auto kAutoUpdateKey
	= "wenzgram-auto-update"_cs;
inline constexpr auto kProfileNftKey
	= "wenzgram-profile-nft"_cs;

[[nodiscard]] bool readBool(std::string_view key, bool fallback = false);
void writeBool(std::string_view key, bool value);

[[nodiscard]] bool localChatWallpapersEnabled();
[[nodiscard]] bool showBranding();
[[nodiscard]] bool compactDialogs();
[[nodiscard]] bool disableChatAnimations();
[[nodiscard]] bool hideEditedBadge();
[[nodiscard]] bool confirmBeforeSend();
[[nodiscard]] bool showSecondsInTime();
[[nodiscard]] bool copyUsernameOnClick();
[[nodiscard]] bool hideChatFolders();
[[nodiscard]] bool largeEmoji();
[[nodiscard]] bool showDeletedMessages();
[[nodiscard]] bool autoUpdateEnabled();
[[nodiscard]] bool profileNftEnabled();

[[nodiscard]] rpl::producer<bool> localChatWallpapersEnabledValue();
[[nodiscard]] rpl::producer<bool> profileNftEnabledValue();
[[nodiscard]] rpl::producer<> settingsChanged();

void notifySettingsChanged();

} // namespace Wenzgram
