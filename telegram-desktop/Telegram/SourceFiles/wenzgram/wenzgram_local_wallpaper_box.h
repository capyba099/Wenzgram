/*
This file is part of Wenzgram,
a Telegram Desktop fork with local per-chat wallpaper preview.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "data/data_wall_paper.h"

namespace Window {
class SessionController;
} // namespace Window

class PeerData;

namespace Wenzgram::LocalWallpapers {

void ShowLocalWallpaperBox(
	not_null<Window::SessionController*> controller,
	not_null<PeerData*> peer,
	Data::WallPaper paper);

} // namespace Wenzgram::LocalWallpapers
