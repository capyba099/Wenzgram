/*
This file is part of Wenzgram,
a Telegram Desktop fork with local profile NFT display.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Window {
class SessionController;
} // namespace Window

namespace Wenzgram::ProfileNft {

void ShowProfileNftPicker(not_null<Window::SessionController*> controller);

} // namespace Wenzgram::ProfileNft
