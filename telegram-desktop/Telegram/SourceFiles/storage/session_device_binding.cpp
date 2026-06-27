/*
This file is part of Wenzgram,
a Telegram Desktop fork with session device binding protection.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "storage/session_device_binding.h"

#include "base/openssl_help.h"
#include "base/random.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSysInfo>

namespace Storage {
namespace {

constexpr auto kBindingSalt = "Wenzgram-SessionBinding-v1";
constexpr auto kInstallIdSize = 32;
constexpr auto kFingerprintSize = 32;

[[nodiscard]] QByteArray MachineFingerprint() {
	auto result = QByteArray();
	result.reserve(256);
	result.append(QSysInfo::machineUniqueId());
	result.append(QSysInfo::machineHostName().toUtf8());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	result.append(QSysInfo::bootUniqueId());
#endif
	result.append(kBindingSalt);
	return result;
}

[[nodiscard]] QByteArray LoadOrCreateInstallId(const QString &basePath) {
	const auto path = basePath + u"install_id"_q;
	QFile file(path);
	if (file.exists() && file.open(QIODevice::ReadOnly)) {
		const auto id = file.readAll();
		if (id.size() == kInstallIdSize) {
			return id;
		}
	}

	auto id = QByteArray(kInstallIdSize, Qt::Uninitialized);
	base::RandomFill(id.data(), id.size());
	if (!QDir().exists(basePath)) {
		QDir().mkpath(basePath);
	}
	if (file.open(QIODevice::WriteOnly)) {
		file.write(id);
	}
	return id;
}

} // namespace

QByteArray SessionDeviceBinding::fingerprint() {
	return openssl::Sha256(bytes::make_span(MachineFingerprint()));
}

QByteArray SessionDeviceBinding::bindingMaterial(const QString &basePath) {
	auto material = MachineFingerprint();
	material.append(LoadOrCreateInstallId(basePath));
	return openssl::Sha512(bytes::make_span(material));
}

bool SessionDeviceBinding::verifyStoredFingerprint(
		const QByteArray &stored,
		const QString &basePath) {
	if (stored.size() != kFingerprintSize) {
		return false;
	}
	return stored == storedFingerprint(basePath);
}

QByteArray SessionDeviceBinding::storedFingerprint(const QString &basePath) {
	auto material = bindingMaterial(basePath);
	return openssl::Sha256(bytes::make_span(material));
}

} // namespace Storage
