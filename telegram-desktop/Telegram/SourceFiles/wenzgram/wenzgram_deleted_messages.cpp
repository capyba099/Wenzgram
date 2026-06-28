/*
This file is part of Wenzgram,
a Telegram Desktop fork that can keep deleted messages visible locally.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_deleted_messages.h"

#include "wenzgram/wenzgram_settings.h"

#include "data/data_msg_id.h"
#include "history/history_item.h"
#include "history/view/history_view_element.h"

namespace Wenzgram::DeletedMessages {
namespace {

base::flat_set<FullMsgId> Preserved;

} // namespace

bool isEnabled() {
	return Wenzgram::showDeletedMessages();
}

bool isPreserved(not_null<const HistoryItem*> item) {
	return Preserved.contains(item->fullId());
}

void preserve(not_null<HistoryItem*> item) {
	if (!isPreserved(item)) {
		Preserved.emplace(item->fullId());
	}
	item->destroyHistoryEntry();
	item->removeFromSharedMediaIndex();
	if (const auto view = item->mainView()) {
		view->repaint();
	}
}

void processDeletionList(
		const std::vector<not_null<HistoryItem*>> &items,
		Fn<void(const std::vector<not_null<HistoryItem*>> &toDestroy)> destroy) {
	if (!isEnabled()) {
		destroy(items);
		return;
	}
	for (const auto &item : items) {
		preserve(item);
	}
}

} // namespace Wenzgram::DeletedMessages
