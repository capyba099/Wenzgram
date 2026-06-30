/*
This file is part of Wenzgram,
a Telegram Desktop fork with local profile NFT display.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "data/data_types.h"
#include "rpl/producer.h"

class PeerData;
class QPainter;

namespace Data {
struct SavedStarGift;
} // namespace Data

namespace Main {
class Session;
} // namespace Main

namespace Window {
class SessionController;
} // namespace Window

namespace Wenzgram::ProfileNft {

struct Entry {
	CollectibleId uniqueId = 0;
	QString slug;
	QString title;
	DocumentId stickerId = 0;
	QImage preview;
};

[[nodiscard]] bool hasOwn(not_null<Main::Session*> session);
[[nodiscard]] std::optional<Entry> own(not_null<Main::Session*> session);

void setOwn(
	not_null<Main::Session*> session,
	Entry entry);
void setOwn(
	not_null<Main::Session*> session,
	const Data::SavedStarGift &gift);
void removeOwn(not_null<Main::Session*> session);

[[nodiscard]] rpl::producer<std::optional<Entry>> ownValue(
	not_null<Main::Session*> session);

void paintOnUserpic(
	QPainter &p,
	const QRect &geometry,
	not_null<PeerData*> peer);

void showPicker(not_null<Window::SessionController*> controller);

} // namespace Wenzgram::ProfileNft
