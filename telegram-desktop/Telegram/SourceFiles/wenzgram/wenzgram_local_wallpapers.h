/*
This file is part of Wenzgram,
a Telegram Desktop fork with local per-chat wallpapers.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "data/data_peer_id.h"
#include "rpl/producer.h"

class PeerData;
class QImage;

namespace Data {
class WallPaper;
} // namespace Data

namespace Main {
class Session;
} // namespace Main

namespace Window {
class SessionController;
} // namespace Window

namespace Wenzgram::LocalWallpapers {

[[nodiscard]] bool has(not_null<Main::Session*> session, PeerId peerId);
[[nodiscard]] std::optional<Data::WallPaper> paper(
	not_null<Main::Session*> session,
	PeerId peerId);
[[nodiscard]] QImage image(
	not_null<Main::Session*> session,
	PeerId peerId);

void set(
	not_null<Main::Session*> session,
	PeerId peerId,
	const Data::WallPaper &paper,
	QImage image);
void remove(not_null<Main::Session*> session, PeerId peerId);

[[nodiscard]] rpl::producer<std::optional<Data::WallPaper>> paperValue(
	not_null<Main::Session*> session,
	PeerId peerId);

void chooseFromFile(
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer);

void showManager(
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer);

} // namespace Wenzgram::LocalWallpapers
