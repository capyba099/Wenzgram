/*
This file is part of Wenzgram,
a Telegram Desktop fork with GitHub release auto-updates.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/timer.h"
#include "rpl/producer.h"

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

namespace Wenzgram::Updater {

enum class State {
	None,
	Checking,
	Downloading,
	Ready,
};

class Service final : public QObject {
public:
	[[nodiscard]] static Service &Instance();

	[[nodiscard]] QString currentVersion() const;
	[[nodiscard]] QString latestVersion() const;
	[[nodiscard]] State state() const;

	[[nodiscard]] rpl::producer<> checking() const;
	[[nodiscard]] rpl::producer<> isLatest() const;
	[[nodiscard]] rpl::producer<> failed() const;
	[[nodiscard]] rpl::producer<> ready() const;
	[[nodiscard]] rpl::producer<float64> progress() const;

	void start(bool force = false);
	void checkNow();
	void installUpdate();
	[[nodiscard]] bool isReady() const;

private:
	Service();

	void scheduleNextCheck();
	void handleReleaseResponse(const QByteArray &bytes);
	void downloadAsset(const QString &url);
	void fail();

	QString _latestVersion;
	QString _downloadPath;
	State _state = State::None;
	QNetworkReply *_checkReply = nullptr;
	QNetworkReply *_downloadReply = nullptr;
	QNetworkAccessManager *_manager = nullptr;
	base::Timer _checkTimer;
	rpl::event_stream<> _checking;
	rpl::event_stream<> _isLatest;
	rpl::event_stream<> _failed;
	rpl::event_stream<> _ready;
	rpl::event_stream<float64> _progress;

};

} // namespace Wenzgram::Updater
