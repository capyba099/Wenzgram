/*
This file is part of Wenzgram,
a Telegram Desktop fork with local profile NFT display.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "rpl/producer.h"

class HistoryItem;
class PeerData;
class QPainter;

namespace Data {
class WallPaper;
} // namespace Data

namespace Main {
class Session;
} // namespace Main

namespace Window {
class SessionController;
} // namespace Window

namespace Wenzgram::ProfileNft {

struct Entry {
	QString id;
	QString title;
	QImage image;
};

[[nodiscard]] bool isSyncMessage(not_null<HistoryItem*> item);
void processIncoming(not_null<HistoryItem*> item);

[[nodiscard]] bool hasOwn(not_null<Main::Session*> session);
[[nodiscard]] std::optional<Entry> own(not_null<Main::Session*> session);
[[nodiscard]] std::optional<Entry> forPeer(
	not_null<Main::Session*> session,
	PeerId peerId);

void setOwn(
	not_null<Main::Session*> session,
	Entry entry);
void removeOwn(not_null<Main::Session*> session);

[[nodiscard]] rpl::producer<std::optional<Entry>> ownValue(
	not_null<Main::Session*> session);
[[nodiscard]] rpl::producer<std::optional<Entry>> forPeerValue(
	not_null<Main::Session*> session,
	PeerId peerId);

void requestFromPeer(
	not_null<Main::Session*> session,
	not_null<PeerData*> peer);
void paintOnUserpic(
	QPainter &p,
	const QRect &geometry,
	not_null<PeerData*> peer);

void chooseFromFile(
	not_null<Window::SessionController*> controller);
void showManager(not_null<Window::SessionController*> controller);

} // namespace Wenzgram::ProfileNft
