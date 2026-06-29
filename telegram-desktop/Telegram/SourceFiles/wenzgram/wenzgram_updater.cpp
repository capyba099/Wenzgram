/*
This file is part of Wenzgram,
a Telegram Desktop fork with GitHub release auto-updates.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_updater.h"

#include "wenzgram/wenzgram_settings.h"
#include "wenzgram/wenzgram_version.h"

#include "core/application.h"
#include "core/click_handler_types.h"
#include "settings.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QVersionNumber>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Wenzgram::Updater {
namespace {

constexpr auto kCheckInterval = crl::time(6 * 60 * 60 * 1000);
constexpr auto kUserAgent = "Wenzgram-Updater/1.0";

[[nodiscard]] QVersionNumber ParseVersion(QString value) {
	if (value.startsWith(u'v')) {
		value = value.mid(1);
	}
	const auto dash = value.indexOf(u'-');
	if (dash >= 0) {
		value = value.left(dash);
	}
	return QVersionNumber::fromString(value);
}

[[nodiscard]] QString UpdatesFolder() {
	return cWorkingDir() + u"tupdates/wenzgram/"_q;
}

[[nodiscard]] QString DownloadedExePath() {
	return UpdatesFolder() + u"Wenzgram.exe"_q;
}

[[nodiscard]] QString UpdaterExePath() {
	return cExeDir() + u"WenzgramUpdater.exe"_q;
}

[[nodiscard]] QNetworkRequest MakeRequest(const QUrl &url) {
	auto request = QNetworkRequest(url);
	request.setHeader(
		QNetworkRequest::UserAgentHeader,
		QByteArray(kUserAgent));
	request.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);
	return request;
}

} // namespace

Service &Service::Instance() {
	static auto instance = Service();
	return instance;
}

Service::Service()
: _checkTimer([=] { checkNow(); }) {
}

QString Service::currentVersion() const {
	return QString::fromLatin1(Wenzgram::kVersion);
}

QString Service::latestVersion() const {
	return _latestVersion;
}

State Service::state() const {
	return _state;
}

rpl::producer<> Service::checking() const {
	return _checking.events();
}

rpl::producer<> Service::isLatest() const {
	return _isLatest.events();
}

rpl::producer<> Service::failed() const {
	return _failed.events();
}

rpl::producer<> Service::ready() const {
	return _ready.events();
}

rpl::producer<float64> Service::progress() const {
	return _progress.events();
}

bool Service::isReady() const {
	return _state == State::Ready && QFile::exists(_downloadPath);
}

void Service::start(bool force) {
	if (!Wenzgram::autoUpdateEnabled()) {
		return;
	}
	if (force) {
		checkNow();
	} else {
		scheduleNextCheck();
		checkNow();
	}
}

void Service::scheduleNextCheck() {
	_checkTimer.callOnce(kCheckInterval);
}

void Service::checkNow() {
	if (_state == State::Checking || _state == State::Downloading) {
		return;
	}
	if (!_manager) {
		_manager = new QNetworkAccessManager(this);
	}
	_state = State::Checking;
	_checking.fire({});
	const auto url = QUrl(
		u"https://api.github.com/repos/"_q
		+ QLatin1String(Wenzgram::kGitHubRepo)
		+ u"/releases/latest"_q);
	_checkReply = _manager->get(MakeRequest(url));
	connect(_checkReply, &QNetworkReply::finished, this, [=] {
		const auto reply = _checkReply;
		_checkReply = nullptr;
		if (reply) {
			reply->deleteLater();
		}
		if (!reply || reply->error() != QNetworkReply::NoError) {
			fail();
			return;
		}
		handleReleaseResponse(reply->readAll());
	});
}

void Service::handleReleaseResponse(const QByteArray &bytes) {
	const auto document = QJsonDocument::fromJson(bytes);
	if (!document.isObject()) {
		fail();
		return;
	}
	const auto object = document.object();
	const auto tag = object.value(u"tag_name"_q).toString();
	if (tag.isEmpty()) {
		fail();
		return;
	}
	const auto remote = ParseVersion(tag);
	const auto local = ParseVersion(currentVersion());
	if (remote <= local) {
		_state = State::None;
		_isLatest.fire({});
		scheduleNextCheck();
		return;
	}
	_latestVersion = remote.toString();
	const auto assets = object.value(u"assets"_q).toArray();
	auto downloadUrl = QString();
	for (const auto &entry : assets) {
		const auto asset = entry.toObject();
		const auto name = asset.value(u"name"_q).toString();
		if (name.compare(u"Wenzgram.exe"_q, Qt::CaseInsensitive) == 0) {
			downloadUrl = asset.value(u"browser_download_url"_q).toString();
			break;
		}
	}
	if (downloadUrl.isEmpty()) {
		fail();
		return;
	}
	downloadAsset(downloadUrl);
}

void Service::downloadAsset(const QString &url) {
	_state = State::Downloading;
	const auto folder = UpdatesFolder();
	QDir().mkpath(folder);
	_downloadPath = DownloadedExePath();
	if (QFile::exists(_downloadPath)) {
		QFile::remove(_downloadPath);
	}
	_downloadReply = _manager->get(MakeRequest(QUrl(url)));
	connect(
		_downloadReply,
		&QNetworkReply::downloadProgress,
		this,
		[=](qint64 received, qint64 total) {
			if (total > 0) {
				const auto value = std::clamp(
					received / float64(total),
					0.,
					1.);
				_progress.fire(value);
			}
		});
	connect(_downloadReply, &QNetworkReply::finished, this, [=] {
		const auto reply = _downloadReply;
		_downloadReply = nullptr;
		if (!reply) {
			fail();
			return;
		}
		if (reply->error() != QNetworkReply::NoError) {
			reply->deleteLater();
			fail();
			return;
		}
		QFile file(_downloadPath);
		if (!file.open(QIODevice::WriteOnly)) {
			reply->deleteLater();
			fail();
			return;
		}
		file.write(reply->readAll());
		file.close();
		reply->deleteLater();
		if (file.size() < 1024 * 1024) {
			QFile::remove(_downloadPath);
			fail();
			return;
		}
		_state = State::Ready;
		_ready.fire({});
		scheduleNextCheck();
	});
}

void Service::fail() {
	_state = State::None;
	_failed.fire({});
	scheduleNextCheck();
}

void Service::installUpdate() {
	if (!isReady()) {
		return;
	}
#ifdef Q_OS_WIN
	const auto updater = UpdaterExePath();
	if (!QFile::exists(updater)) {
		fail();
		return;
	}
	const auto targetExe = QCoreApplication::applicationFilePath();
	const auto pid = QString::number(QCoreApplication::applicationPid());
	const auto args = QStringList{
		u"--update"_q,
		pid,
		targetExe,
		_downloadPath,
	};
	if (!QProcess::startDetached(updater, args, cExeDir())) {
		fail();
		return;
	}
	Core::Quit();
#else
	const auto url = u"https://github.com/"_q
		+ QLatin1String(Wenzgram::kGitHubRepo)
		+ u"/releases/latest"_q;
	UrlClickHandler::Open(url);
#endif
}

} // namespace Wenzgram::Updater
