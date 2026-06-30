/*
This file is part of Wenzgram,
a Telegram Desktop fork with local profile NFT display.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_profile_nft_box.h"

#include "wenzgram/wenzgram_profile_nft.h"
#include "wenzgram/wenzgram_settings.h"

#include "base/random.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "ui/chat/attach/attach_extensions.h"
#include "ui/image/image.h"
#include "ui/layers/generic_box.h"
#include "ui/painter.h"
#include "ui/ui_utility.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"

#include <QtCore/QFileInfo>

namespace Wenzgram::ProfileNft {
namespace {

[[nodiscard]] QPixmap GeneratePreview(const QImage &image) {
	if (image.isNull()) {
		return QPixmap();
	}
	const auto size = QSize(st::boxWideWidth / 2, st::boxWideWidth / 2)
		* style::DevicePixelRatio();
	auto scaled = image.scaled(
		size,
		Qt::KeepAspectRatioByExpanding,
		Qt::SmoothTransformation);
	auto result = Ui::PixmapFromImage(std::move(scaled));
	result.setDevicePixelRatio(style::DevicePixelRatio());
	return result;
}

void ApplyImage(
		not_null<Window::SessionController*> controller,
		QImage image,
		const QString &title) {
	if (image.isNull()) {
		return;
	}
	auto entry = Entry{
		.id = QString::number(base::RandomValue<quint64>()),
		.title = title.isEmpty() ? u"Локальный NFT"_q : title,
		.image = std::move(image),
	};
	setOwn(&controller->session(), std::move(entry));
	controller->showToast(u"NFT профиля установлен"_q);
}

} // namespace

void ShowProfileNftBox(not_null<Window::SessionController*> controller) {
	if (!profileNftEnabled()) {
		controller->showToast(
			u"Включите локальные NFT в настройках Wenzgram"_q);
		return;
	}
	const auto session = &controller->session();
	const auto current = own(session);
	const auto preview = current
		? GeneratePreview(current->image)
		: QPixmap();

	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(u"Локальный NFT профиля"_q);
		box->setWidth(st::boxWideWidth);

		box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			u"NFT виден в профиле у пользователей Wenzgram. Не синхронизируется с Telegram."_q,
			st::boxLabel));

		const auto widget = box->addRow(object_ptr<Ui::RpWidget>(box));
		const auto side = st::boxWideWidth / 2;
		widget->resize(side, side);
		widget->paintRequest(
		) | rpl::on_next([=] {
			Painter p(widget);
			if (!preview.isNull()) {
				p.drawPixmap(0, 0, preview);
			} else {
				p.fillRect(widget->rect(), st::boxBg);
			}
			if (current) {
				p.setPen(st::boxLabel.textFg);
				p.drawText(
					widget->rect().adjusted(8, 8, -8, -8),
					Qt::AlignBottom | Qt::AlignHCenter,
					current->title);
			}
		}, widget->lifetime());

		box->addButton(rpl::single(u"Выбрать изображение"_q), [=] {
			auto filters = QStringList(
				u"Images (*"_q
				+ Ui::ImageExtensions().join(u" *"_q)
				+ u")"_q);
			filters.push_back(FileDialog::AllFilesFilter());
			FileDialog::GetOpenPath(
				Core::App().getFileDialogParent(),
				u"Выберите NFT для профиля"_q,
				filters.join(u";;"_q),
				crl::guard(controller, [=](const FileDialog::OpenResult &result) {
					if (result.paths.isEmpty()
						&& result.remoteContent.isEmpty()) {
						return;
					}
					auto loaded = Images::Read({
						.path = result.paths.isEmpty()
							? QString()
							: result.paths.front(),
						.content = result.remoteContent,
						.forceOpaque = true,
					}).image;
					if (loaded.isNull()) {
						controller->showToast(u"Не удалось открыть изображение"_q);
						return;
					}
					const auto name = result.paths.isEmpty()
						? u"Локальный NFT"_q
						: QFileInfo(result.paths.front()).baseName();
					ApplyImage(controller, std::move(loaded), name);
					box->closeBox();
				}));
		});

		if (current) {
			box->addButton(rpl::single(u"Сбросить NFT"_q), [=] {
				removeOwn(session);
				controller->showToast(u"NFT профиля сброшен"_q);
				box->closeBox();
			});
		}

		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	}));
}

} // namespace Wenzgram::ProfileNft
