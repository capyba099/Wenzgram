/*
This file is part of Wenzgram,
a Telegram Desktop fork with local per-chat wallpaper preview.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_local_wallpaper_box.h"

#include "wenzgram/wenzgram_local_wallpapers.h"

#include "data/data_peer.h"
#include "lang/lang_keys.h"
#include "ui/image/image.h"
#include "ui/layers/generic_box.h"
#include "ui/painter.h"
#include "ui/ui_utility.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"

namespace Wenzgram::LocalWallpapers {
namespace {

[[nodiscard]] QPixmap GeneratePreview(const Data::WallPaper &paper) {
	const auto thumb = paper.localThumbnail();
	if (!thumb) {
		return QPixmap();
	}
	const auto size = QSize(st::boxWideWidth, st::boxWideWidth * 3 / 4)
		* style::DevicePixelRatio();
	auto image = thumb->original().scaled(
		size,
		Qt::KeepAspectRatioByExpanding,
		Qt::SmoothTransformation);
	auto result = Ui::PixmapFromImage(image);
	result.setDevicePixelRatio(style::DevicePixelRatio());
	return result;
}

} // namespace

void ShowLocalWallpaperBox(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer,
		Data::WallPaper paper) {
	const auto preview = GeneratePreview(paper);
	const auto hasLocal = has(&peer->session(), peer->id);
	const auto hasNewImage = !paper.isNull() && !preview.isNull();

	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(u"Локальные обои чата"_q);
		box->setWidth(st::boxWideWidth);

		box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			peer->name(),
			st::boxLabel));
		box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			u"Обои видны только на этом устройстве и не синхронизируются с Telegram."_q,
			st::boxLabel));

		const auto widget = box->addRow(object_ptr<Ui::RpWidget>(box));
		widget->resize(st::boxWideWidth, st::boxWideWidth * 3 / 4);
		widget->paintRequest(
		) | rpl::on_next([=] {
			Painter p(widget);
			if (!preview.isNull()) {
				p.drawPixmap(0, 0, preview);
			} else if (hasLocal) {
				const auto current = image(&peer->session(), peer->id);
				if (!current.isNull()) {
					const auto scaled = Ui::PixmapFromImage(current.scaled(
						widget->size() * style::DevicePixelRatio(),
						Qt::KeepAspectRatioByExpanding,
						Qt::SmoothTransformation));
					p.drawPixmap(0, 0, scaled);
					return;
				}
			}
			p.fillRect(widget->rect(), st::boxBg);
		}, widget->lifetime());

		if (hasNewImage) {
			box->addButton(rpl::single(u"Применить локально"_q), [=] {
				const auto thumb = paper.localThumbnail();
				if (!thumb) {
					box->closeBox();
					return;
				}
				auto loaded = thumb->original();
				if (loaded.isNull()) {
					box->closeBox();
					return;
				}
				set(&peer->session(), peer->id, paper, std::move(loaded));
				controller->showToast(u"Локальные обои применены"_q);
				box->closeBox();
			});
		}
		if (hasLocal) {
			box->addButton(rpl::single(u"Сбросить локальные обои"_q), [=] {
				remove(&peer->session(), peer->id);
				controller->showToast(u"Локальные обои сброшены"_q);
				box->closeBox();
			});
		}
		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	}));
}

} // namespace Wenzgram::LocalWallpapers
