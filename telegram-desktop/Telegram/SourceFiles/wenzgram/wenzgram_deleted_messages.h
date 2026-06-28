/*
This file is part of Wenzgram,
a Telegram Desktop fork that can keep deleted messages visible locally.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

class HistoryItem;

namespace Wenzgram::DeletedMessages {

[[nodiscard]] bool isEnabled();
[[nodiscard]] bool isPreserved(not_null<const HistoryItem*> item);

void preserve(not_null<HistoryItem*> item);

void processDeletionList(
	const std::vector<not_null<HistoryItem*>> &items,
	Fn<void(const std::vector<not_null<HistoryItem*>> &toDestroy)> destroy);

} // namespace Wenzgram::DeletedMessages
