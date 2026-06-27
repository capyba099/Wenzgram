/*
This file is part of Wenzgram,
a Telegram Desktop fork with session device binding protection.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Storage {

// Binds local session data to the current device and installation.
// Copied tdata cannot be decrypted on another machine or client install.
class SessionDeviceBinding final {
public:
	[[nodiscard]] static QByteArray fingerprint();
	[[nodiscard]] static QByteArray bindingMaterial(const QString &basePath);
	[[nodiscard]] static bool verifyStoredFingerprint(
		const QByteArray &stored,
		const QString &basePath);
	[[nodiscard]] static QByteArray storedFingerprint(const QString &basePath);

};

} // namespace Storage
