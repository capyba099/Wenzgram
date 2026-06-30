/*
This file is part of Wenzgram,
a Telegram Desktop fork with local profile NFT display.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_profile_nft.h"

#include "wenzgram/wenzgram_profile_nft_box.h"
#include "wenzgram/wenzgram_settings.h"

#include "api/api_sending.h"
#include "apiwrap.h"
#include "base/random.h"
#include "data/data_histories.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "dialogs/dialogs_main_list.h"
#include "dialogs/dialogs_row.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "storage/storage_account.h"
#include "ui/painter.h"
#include "ui/ui_utility.h"

#include <QtCore/QBuffer>
#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <map>

namespace Wenzgram::ProfileNft {
namespace {

constexpr auto kMagic = quint32(0x575a4e46); // 'WZNF'
const auto kSyncPrefix = u"\u2063WZ_NFT:"_q;
const auto kRequestPrefix = u"\u2063WZ_NFT_REQ"_q;
constexpr auto kBroadcastLimit = 30;
constexpr auto kThumbSize = 96;

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

[[nodiscard]] QString peerDataPath(
		not_null<Main::Session*> session,
		PeerId peerId) {
	return databasePath(session)
		+ u"wenzgram/profile_nft/peers/"_q
		+ QString::number(peerId.value)
		+ u"/data"_q;
}

[[nodiscard]] QByteArray EncodeImage(const QImage &source) {
	auto image = source;
	if (image.isNull()) {
		return {};
	}
	if (image.width() > kThumbSize || image.height() > kThumbSize) {
		image = image.scaled(
			QSize(kThumbSize, kThumbSize),
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
	}
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, "JPG", 82);
	return bytes.toBase64(QByteArray::Base64Encoding);
}

[[nodiscard]] QImage DecodeImage(const QByteArray &encoded) {
	if (encoded.isEmpty()) {
		return {};
	}
	QImage image;
	image.loadFromData(QByteArray::fromBase64(encoded), "JPG");
	return image;
}

[[nodiscard]] QString EncodePayload(const Entry &entry) {
	auto object = QJsonObject{
		{ u"v"_q, 1 },
		{ u"id"_q, entry.id },
		{ u"title"_q, entry.title },
		{ u"img"_q, QString::fromLatin1(EncodeImage(entry.image)) },
	};
	return kSyncPrefix + QString::fromUtf8(
		QJsonDocument(object).toJson(QJsonDocument::Compact));
}

[[nodiscard]] std::optional<Entry> DecodePayload(const QString &text) {
	if (!text.startsWith(kSyncPrefix)) {
		return std::nullopt;
	}
	const auto json = text.mid(kSyncPrefix.size());
	const auto document = QJsonDocument::fromJson(json.toUtf8());
	if (!document.isObject()) {
		return std::nullopt;
	}
	const auto object = document.object();
	const auto title = object.value(u"title"_q).toString();
	const auto id = object.value(u"id"_q).toString();
	const auto image = DecodeImage(
		object.value(u"img"_q).toString().toLatin1());
	if (title.isEmpty() || image.isNull()) {
		return std::nullopt;
	}
	return Entry{
		.id = id.isEmpty() ? title : id,
		.title = title,
		.image = std::move(image),
	};
}

[[nodiscard]] std::optional<Entry> ReadFile(const QString &path) {
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return std::nullopt;
	}
	QDataStream stream(&file);
	stream.setVersion(QDataStream::Qt_5_1);
	quint32 magic = 0;
	QString id;
	QString title;
	QByteArray imageData;
	stream >> magic >> id >> title >> imageData;
	if (stream.status() != QDataStream::Ok || magic != kMagic) {
		return std::nullopt;
	}
	auto image = QImage();
	if (!image.loadFromData(imageData)) {
		return std::nullopt;
	}
	return Entry{ id, title, std::move(image) };
}

void WriteFile(const QString &path, const Entry &entry) {
	QDir().mkpath(QFileInfo(path).absolutePath());
	QByteArray imageData;
	{
		QBuffer buffer(&imageData);
		buffer.open(QIODevice::WriteOnly);
		entry.image.save(&buffer, "PNG");
	}
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		return;
	}
	QDataStream stream(&file);
	stream.setVersion(QDataStream::Qt_5_1);
	stream << kMagic << entry.id << entry.title << imageData;
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
		}
		return _own;
	}

	void setOwn(Entry entry) {
		_own = entry;
		_ownLoaded = true;
		WriteFile(ownDataPath(_session), entry);
		_ownChanged.fire({});
		broadcastOwn();
	}

	void removeOwn() {
		_own = std::nullopt;
		_ownLoaded = true;
		RemoveFile(ownDataPath(_session));
		_ownChanged.fire({});
	}

	[[nodiscard]] std::optional<Entry> forPeer(PeerId peerId) {
		ensurePeerLoaded(peerId);
		const auto i = _peers.find(peerId);
		if (i != end(_peers)) {
			return i->second;
		}
		return std::nullopt;
	}

	void setForPeer(PeerId peerId, Entry entry) {
		_peers[peerId] = std::move(entry);
		WriteFile(peerDataPath(_session, peerId), _peers[peerId]);
		_peerChanged.fire_copy(peerId);
	}

	void removeForPeer(PeerId peerId) {
		_peers.erase(peerId);
		RemoveFile(peerDataPath(_session, peerId));
		_peerChanged.fire_copy(peerId);
	}

	[[nodiscard]] rpl::producer<std::optional<Entry>> ownValue() {
		return rpl::single(own()) | rpl::then(
			_ownChanged.events() | rpl::map([=] { return own(); })
		);
	}

	[[nodiscard]] rpl::producer<std::optional<Entry>> forPeerValue(PeerId peerId) {
		return rpl::single(forPeer(peerId)) | rpl::then(
			_peerChanged.events(
			) | rpl::filter([=](PeerId changed) {
				return changed == peerId;
			}) | rpl::map([=] {
				return forPeer(peerId);
			})
		);
	}

	void sendText(not_null<PeerData*> peer, const QString &text) {
		if (!peer->isUser() || peer->isSelf()) {
			return;
		}
		const auto history = _session->data().history(peer);
		auto action = Api::SendAction(history);
		action.options.silent = true;
		action.clearDraft = false;
		action.generateLocal = true;
		auto message = Api::MessageToSend(action);
		message.textWithTags = { text, TextWithTags::Tags() };
		_session->api().sendMessage(std::move(message));
	}

	void broadcastOwn() {
		if (!profileNftEnabled()) {
			return;
		}
		const auto current = own();
		if (!current) {
			return;
		}
		const auto payload = EncodePayload(*current);
		auto sent = 0;
		const auto &rows = _session->data().chatsList()->indexed()->all();
		for (const auto &row : rows) {
			if (sent >= kBroadcastLimit) {
				break;
			}
			const auto history = row->history();
			if (!history) {
				continue;
			}
			const auto peer = history->peer;
			if (!peer->isUser() || peer->isSelf()) {
				continue;
			}
			sendText(peer, payload);
			++sent;
		}
	}

	void requestFromPeer(not_null<PeerData*> peer) {
		if (!profileNftEnabled() || peer->isSelf() || !peer->isUser()) {
			return;
		}
		if (forPeer(peer->id)) {
			return;
		}
		sendText(peer, kRequestPrefix);
	}

	void processIncoming(not_null<HistoryItem*> item) {
		if (!profileNftEnabled() || !item->isRegular()) {
			return;
		}
		const auto from = item->from();
		if (!from || !from->isUser()) {
			return;
		}
		const auto text = item->originalText().text;
		if (text == kRequestPrefix) {
			const auto current = own();
			if (!current) {
				return;
			}
			sendText(from, EncodePayload(*current));
			return;
		}
		if (const auto entry = DecodePayload(text)) {
			setForPeer(from->id, *entry);
		}
	}

private:
	void ensurePeerLoaded(PeerId peerId) {
		if (_peers.contains(peerId)) {
			return;
		}
		if (const auto loaded = ReadFile(peerDataPath(_session, peerId))) {
			_peers.emplace(peerId, *loaded);
		}
	}

	const not_null<Main::Session*> _session;
	bool _ownLoaded = false;
	std::optional<Entry> _own;
	std::map<PeerId, Entry> _peers;
	rpl::event_stream<> _ownChanged;
	rpl::event_stream<PeerId> _peerChanged;

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

[[nodiscard]] std::optional<Entry> EntryForPeer(not_null<PeerData*> peer) {
	auto &manager = Get(&peer->session());
	if (peer->isSelf()) {
		return manager.own();
	}
	return manager.forPeer(peer->id);
}

} // namespace

bool isSyncMessage(not_null<HistoryItem*> item) {
	if (!item->isRegular()) {
		return false;
	}
	const auto &text = item->originalText().text;
	return text.startsWith(kSyncPrefix) || text.startsWith(kRequestPrefix);
}

void processIncoming(not_null<HistoryItem*> item) {
	Get(&item->history()->session()).processIncoming(item);
}

bool hasOwn(not_null<Main::Session*> session) {
	return Get(session).own().has_value();
}

std::optional<Entry> own(not_null<Main::Session*> session) {
	return Get(session).own();
}

std::optional<Entry> forPeer(
		not_null<Main::Session*> session,
		PeerId peerId) {
	return Get(session).forPeer(peerId);
}

void setOwn(not_null<Main::Session*> session, Entry entry) {
	Get(session).setOwn(std::move(entry));
}

void removeOwn(not_null<Main::Session*> session) {
	Get(session).removeOwn();
}

rpl::producer<std::optional<Entry>> ownValue(not_null<Main::Session*> session) {
	return Get(session).ownValue();
}

rpl::producer<std::optional<Entry>> forPeerValue(
		not_null<Main::Session*> session,
		PeerId peerId) {
	return Get(session).forPeerValue(peerId);
}

void requestFromPeer(
		not_null<Main::Session*> session,
		not_null<PeerData*> peer) {
	Get(session).requestFromPeer(peer);
}

void paintOnUserpic(
		QPainter &p,
		const QRect &geometry,
		not_null<PeerData*> peer) {
	if (!profileNftEnabled()) {
		return;
	}
	const auto entry = EntryForPeer(peer);
	if (!entry || entry->image.isNull()) {
		return;
	}
	const auto badgeSize = geometry.width() / 3;
	const auto badgeRect = QRect(
		geometry.right() - badgeSize + 2,
		geometry.bottom() - badgeSize + 2,
		badgeSize,
		badgeSize);
	auto image = entry->image.scaled(
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

void chooseFromFile(not_null<Window::SessionController*> controller) {
	ShowProfileNftBox(controller);
}

void showManager(not_null<Window::SessionController*> controller) {
	ShowProfileNftBox(controller);
}

} // namespace Wenzgram::ProfileNft
