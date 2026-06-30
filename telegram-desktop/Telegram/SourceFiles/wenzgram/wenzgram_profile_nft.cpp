/*
This file is part of Wenzgram,
a Telegram Desktop fork with local profile NFT display.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_profile_nft.h"

#include "wenzgram/wenzgram_profile_nft_box.h"
#include "wenzgram/wenzgram_settings.h"

#include "data/data_peer.h"
#include "data/data_document.h"
#include "data/data_document_media.h"
#include "data/data_session.h"
#include "data/data_star_gift.h"
#include "main/main_session.h"
#include "storage/storage_account.h"
#include "ui/painter.h"
#include "window/window_session_controller.h"

#include <QtCore/QBuffer>
#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QFile>

namespace Wenzgram::ProfileNft {
namespace {

constexpr auto kMagic = quint32(0x575a4e46); // 'WZNF'

[[nodiscard]] QString databasePath(not_null<Main::Session*> session) {
	auto path = session->local().cachePath();
	if (path.endsWith(u"cache"_q)) {
		path.chop(5);
	}
	return path;
}

[[nodiscard]] QString ownDataPath(not_null<Main::Session*> session) {
	return databasePath(session) + u"wenzgram/profile_nft/own/data"_q;
}

[[nodiscard]] QImage PreviewFromDocument(not_null<DocumentData*> document) {
	auto media = document->createMediaView();
	media->checkStickerSmall();
	if (!media->loaded()) {
		return {};
	}
	if (const auto sticker = media->getStickerSmall()) {
		return sticker->original();
	}
	return {};
}

[[nodiscard]] Entry EntryFromGift(const Data::SavedStarGift &gift) {
	const auto &unique = gift.info.unique;
	Expects(unique != nullptr);

	auto entry = Entry{
		.uniqueId = unique->id,
		.slug = unique->slug,
		.title = Data::UniqueGiftName(*unique),
		.stickerId = unique->model.document->id,
	};
	entry.preview = PreviewFromDocument(unique->model.document);
	return entry;
}

[[nodiscard]] std::optional<Entry> ReadFile(const QString &path) {
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return std::nullopt;
	}
	QDataStream stream(&file);
	stream.setVersion(QDataStream::Qt_5_1);
	quint32 magic = 0;
	quint64 uniqueId = 0;
	QString slug;
	QString title;
	quint64 stickerId = 0;
	QByteArray imageData;
	stream >> magic >> uniqueId >> slug >> title >> stickerId >> imageData;
	if (stream.status() != QDataStream::Ok || magic != kMagic) {
		return std::nullopt;
	}
	auto image = QImage();
	if (!imageData.isEmpty() && !image.loadFromData(imageData)) {
		return std::nullopt;
	}
	return Entry{
		.uniqueId = CollectibleId(uniqueId),
		.slug = slug,
		.title = title,
		.stickerId = DocumentId(stickerId),
		.preview = std::move(image),
	};
}

void WriteFile(const QString &path, const Entry &entry) {
	QDir().mkpath(QFileInfo(path).absolutePath());
	QByteArray imageData;
	if (!entry.preview.isNull()) {
		QBuffer buffer(&imageData);
		buffer.open(QIODevice::WriteOnly);
		entry.preview.save(&buffer, "PNG");
	}
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		return;
	}
	QDataStream stream(&file);
	stream.setVersion(QDataStream::Qt_5_1);
	stream << kMagic
		<< quint64(entry.uniqueId)
		<< entry.slug
		<< entry.title
		<< quint64(entry.stickerId)
		<< imageData;
}

void RemoveFile(const QString &path) {
	const auto dir = QFileInfo(path).absolutePath();
	QDir(dir).removeRecursively();
}

class Manager final {
public:
	explicit Manager(not_null<Main::Session*> session)
	: _session(session) {
	}

	[[nodiscard]] std::optional<Entry> own() {
		if (!_ownLoaded) {
			_own = ReadFile(ownDataPath(_session));
			_ownLoaded = true;
			ensurePreview();
		}
		return _own;
	}

	void setOwn(Entry entry) {
		if (entry.preview.isNull() && entry.stickerId) {
			if (const auto document = _session->data().document(entry.stickerId)) {
				entry.preview = PreviewFromDocument(document);
			}
		}
		_own = std::move(entry);
		_ownLoaded = true;
		WriteFile(ownDataPath(_session), *_own);
		_ownChanged.fire({});
	}

	void removeOwn() {
		_own = std::nullopt;
		_ownLoaded = true;
		RemoveFile(ownDataPath(_session));
		_ownChanged.fire({});
	}

	[[nodiscard]] rpl::producer<std::optional<Entry>> ownValue() {
		return rpl::single(own()) | rpl::then(
			_ownChanged.events() | rpl::map([=] { return own(); })
		);
	}

private:
	void ensurePreview() {
		if (!_own || !_own->preview.isNull() || !_own->stickerId) {
			return;
		}
		const auto document = _session->data().document(_own->stickerId);
		if (!document) {
			return;
		}
		auto media = document->createMediaView();
		media->checkStickerSmall();
		if (media->loaded()) {
			if (const auto sticker = media->getStickerSmall()) {
				_own->preview = sticker->original();
				WriteFile(ownDataPath(_session), *_own);
				_ownChanged.fire({});
			}
			return;
		}
		_previewLifetime = _session->downloaderTaskFinished(
		) | rpl::filter([=] {
			return media->loaded();
		}) | rpl::take(1) | rpl::on_next([=] {
			ensurePreview();
		});
	}

	const not_null<Main::Session*> _session;
	bool _ownLoaded = false;
	std::optional<Entry> _own;
	rpl::event_stream<> _ownChanged;
	rpl::lifetime _previewLifetime;

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

bool hasOwn(not_null<Main::Session*> session) {
	return Get(session).own().has_value();
}

std::optional<Entry> own(not_null<Main::Session*> session) {
	return Get(session).own();
}

void setOwn(not_null<Main::Session*> session, Entry entry) {
	Get(session).setOwn(std::move(entry));
}

void setOwn(
		not_null<Main::Session*> session,
		const Data::SavedStarGift &gift) {
	if (!gift.info.unique) {
		return;
	}
	Get(session).setOwn(EntryFromGift(gift));
}

void removeOwn(not_null<Main::Session*> session) {
	Get(session).removeOwn();
}

rpl::producer<std::optional<Entry>> ownValue(not_null<Main::Session*> session) {
	return Get(session).ownValue();
}

void paintOnUserpic(
		QPainter &p,
		const QRect &geometry,
		not_null<PeerData*> peer) {
	if (!profileNftEnabled() || !peer->isSelf()) {
		return;
	}
	const auto entry = own(&peer->session());
	if (!entry || entry->preview.isNull()) {
		return;
	}
	const auto badgeSize = geometry.width() / 3;
	const auto badgeRect = QRect(
		geometry.right() - badgeSize + 2,
		geometry.bottom() - badgeSize + 2,
		badgeSize,
		badgeSize);
	auto image = entry->preview.scaled(
		badgeSize * style::DevicePixelRatio(),
		badgeSize * style::DevicePixelRatio(),
		Qt::KeepAspectRatioByExpanding,
		Qt::SmoothTransformation);
	auto pixmap = Ui::PixmapFromImage(std::move(image));
	pixmap.setDevicePixelRatio(style::DevicePixelRatio());

	{
		auto hq = PainterHighQualityEnabler(p);
		const auto ring = 3 * style::DevicePixelRatio();
		p.setPen(QPen(QColor(168, 85, 247), ring));
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(geometry.adjusted(ring, ring, -ring, -ring));
	}
	{
		auto hq = PainterHighQualityEnabler(p);
		p.setPen(QPen(QColor(255, 255, 255), 2));
		p.setBrush(Qt::NoBrush);
		p.drawEllipse(badgeRect);
	}
	p.drawPixmap(badgeRect, pixmap);
}

void showPicker(not_null<Window::SessionController*> controller) {
	ShowProfileNftPicker(controller);
}

} // namespace Wenzgram::ProfileNft
