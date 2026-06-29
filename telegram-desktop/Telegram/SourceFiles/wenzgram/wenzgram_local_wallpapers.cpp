/*
This file is part of Wenzgram,
a Telegram Desktop fork with local per-chat wallpapers.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_local_wallpapers.h"

#include "wenzgram/wenzgram_settings.h"

#include "base/flat_map.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "data/data_peer.h"
#include "data/data_wall_paper.h"
#include "main/main_session.h"
#include "storage/storage_account.h"
#include "ui/chat/attach/attach_extensions.h"
#include "ui/image/image.h"
#include "ui/ui_utility.h"
#include "window/window_session_controller.h"
#include "wenzgram/wenzgram_local_wallpaper_box.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QDataStream>
#include <QtCore/QFile>

namespace Wenzgram::LocalWallpapers {
namespace {

constexpr auto kMagic = quint32(0x575a5750); // 'WZWP'

struct Entry {
	Data::WallPaper paper;
	QImage image;
};

class Manager final {
public:
	explicit Manager(not_null<Main::Session*> session)
	: _session(session) {
	}

	[[nodiscard]] bool has(PeerId peerId) const {
		return _cache.contains(peerId) || diskExists(peerId);
	}

	[[nodiscard]] std::optional<Data::WallPaper> paper(PeerId peerId) {
		ensureLoaded(peerId);
		const auto i = _cache.find(peerId);
		if (i != end(_cache)) {
			return i->second.paper;
		}
		return std::nullopt;
	}

	[[nodiscard]] QImage image(PeerId peerId) {
		ensureLoaded(peerId);
		const auto i = _cache.find(peerId);
		return (i != end(_cache)) ? i->second.image : QImage();
	}

	void set(PeerId peerId, const Data::WallPaper &paper, QImage image) {
		_cache[peerId] = Entry{ paper, std::move(image) };
		write(peerId, _cache[peerId]);
		_changed.fire_copy(peerId);
	}

	void remove(PeerId peerId) {
		_cache.remove(peerId);
		removeFromDisk(peerId);
		_changed.fire_copy(peerId);
	}

	[[nodiscard]] rpl::producer<std::optional<Data::WallPaper>> paperValue(
			PeerId peerId) {
		return rpl::single(
			paper(peerId)
		) | rpl::then(
			_changed.events(
			) | rpl::filter([=](PeerId changed) {
				return changed == peerId;
			}) | rpl::map([=] {
				return paper(peerId);
			})
		);
	}

private:
	[[nodiscard]] QString databasePath() const {
		auto path = _session->local().cachePath();
		if (path.endsWith(u"cache"_q)) {
			path.chop(5);
		}
		return path;
	}

	[[nodiscard]] QString rootPath() const {
		return databasePath() + u"wenzgram/wallpapers/"_q;
	}

	[[nodiscard]] QString peerPath(PeerId peerId) const {
		return rootPath() + QString::number(peerId.value) + u"/"_q;
	}

	[[nodiscard]] QString dataPath(PeerId peerId) const {
		return peerPath(peerId) + u"data"_q;
	}

	[[nodiscard]] bool diskExists(PeerId peerId) const {
		return QFile::exists(dataPath(peerId));
	}

	void ensureLoaded(PeerId peerId) {
		if (_cache.contains(peerId) || !diskExists(peerId)) {
			return;
		}
		read(peerId);
	}

	void read(PeerId peerId) {
		QFile file(dataPath(peerId));
		if (!file.open(QIODevice::ReadOnly)) {
			return;
		}
		QDataStream stream(&file);
		stream.setVersion(QDataStream::Qt_5_1);

		quint32 magic = 0;
		QByteArray serialized;
		QByteArray imageData;
		stream >> magic >> serialized >> imageData;
		if (stream.status() != QDataStream::Ok || magic != kMagic) {
			return;
		}
		const auto paper = Data::WallPaper::FromSerialized(serialized);
		if (!paper) {
			return;
		}
		QImage image;
		if (!image.loadFromData(imageData)) {
			return;
		}
		_cache[peerId] = Entry{ *paper, std::move(image) };
	}

	void write(PeerId peerId, const Entry &entry) {
		const auto path = peerPath(peerId);
		QDir().mkpath(path);

		QByteArray imageData;
		{
			QBuffer buffer(&imageData);
			buffer.open(QIODevice::WriteOnly);
			entry.image.save(&buffer, "PNG");
		}

		QFile file(dataPath(peerId));
		if (!file.open(QIODevice::WriteOnly)) {
			return;
		}
		QDataStream stream(&file);
		stream.setVersion(QDataStream::Qt_5_1);
		stream << kMagic << entry.paper.serialize() << imageData;
	}

	void removeFromDisk(PeerId peerId) {
		const auto path = peerPath(peerId);
		QDir dir(path);
		if (dir.exists()) {
			dir.removeRecursively();
		}
	}

	const not_null<Main::Session*> _session;
	base::flat_map<PeerId, Entry> _cache;
	rpl::event_stream<PeerId> _changed;

};

[[nodiscard]] Manager &Get(not_null<Main::Session*> session) {
	static auto managers = base::flat_map<
		not_null<Main::Session*>,
		std::unique_ptr<Manager>>();
	const auto i = managers.find(session);
	if (i != end(managers)) {
		return *i->second;
	}
	auto created = std::make_unique<Manager>(session);
	const auto result = created.get();
	managers.emplace(session, std::move(created));
	return *result;
}

} // namespace

bool has(not_null<Main::Session*> session, PeerId peerId) {
	return Get(session).has(peerId);
}

std::optional<Data::WallPaper> paper(
		not_null<Main::Session*> session,
		PeerId peerId) {
	return Get(session).paper(peerId);
}

QImage image(not_null<Main::Session*> session, PeerId peerId) {
	return Get(session).image(peerId);
}

void set(
		not_null<Main::Session*> session,
		PeerId peerId,
		const Data::WallPaper &paper,
		QImage image) {
	Get(session).set(peerId, paper, std::move(image));
}

void remove(not_null<Main::Session*> session, PeerId peerId) {
	Get(session).remove(peerId);
}

rpl::producer<std::optional<Data::WallPaper>> paperValue(
		not_null<Main::Session*> session,
		PeerId peerId) {
	return Get(session).paperValue(peerId);
}

void chooseFromFile(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	if (!Wenzgram::localChatWallpapersEnabled()) {
		controller->showToast(u"Включите локальные обои в настройках Wenzgram"_q);
		return;
	}
	auto filters = QStringList(
		u"Images (*"_q
		+ Ui::ImageExtensions().join(u" *"_q)
		+ u")"_q);
	filters.push_back(FileDialog::AllFilesFilter());
	const auto callback = crl::guard(controller, [=](
			const FileDialog::OpenResult &result) {
		if (result.paths.isEmpty() && result.remoteContent.isEmpty()) {
			return;
		}
		auto loaded = Images::Read({
			.path = result.paths.isEmpty() ? QString() : result.paths.front(),
			.content = result.remoteContent,
			.forceOpaque = true,
		}).image;
		if (loaded.isNull() || loaded.width() <= 0 || loaded.height() <= 0) {
			controller->showToast(u"Не удалось открыть изображение"_q);
			return;
		}
		auto local = Data::CustomWallPaper();
		local.setLocalImageAsThumbnail(std::make_shared<Image>(
			std::move(loaded)));
		ShowLocalWallpaperBox(controller, peer, local);
	});
	FileDialog::GetOpenPath(
		Core::App().getFileDialogParent(),
		u"Выберите обои для чата"_q,
		filters.join(u";;"_q),
		crl::guard(controller, callback));
}

void showManager(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	ShowLocalWallpaperBox(
		controller,
		peer,
		has(&peer->session(), peer->id)
			? *paper(&peer->session(), peer->id)
			: Data::CustomWallPaper());
}

} // namespace Wenzgram::LocalWallpapers
