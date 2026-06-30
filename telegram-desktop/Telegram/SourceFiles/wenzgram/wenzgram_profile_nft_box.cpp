/*
This file is part of Wenzgram,
a Telegram Desktop fork with local profile NFT display.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "wenzgram/wenzgram_profile_nft_box.h"

#include "wenzgram/wenzgram_profile_nft.h"
#include "wenzgram/wenzgram_settings.h"

#include "data/data_star_gift.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "ui/layers/generic_box.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

namespace Wenzgram::ProfileNft {
namespace {

void AppendGiftRows(
		not_null<Ui::VerticalLayout*> container,
		not_null<Window::SessionController*> controller,
		const std::vector<Data::SavedStarGift> &gifts) {
	const auto session = &controller->session();
	for (const auto &gift : gifts) {
		if (!gift.info.unique) {
			continue;
		}
		const auto title = Data::UniqueGiftName(*gift.info.unique);
		const auto button = AddButtonWithLabel(
			container,
			rpl::single(title),
			rpl::single(QString()),
			st::settingsButtonNoIcon);
		button->setClickedCallback([=, giftData = gift] {
			setOwn(session, giftData);
			controller->showToast(u"NFT добавлен в профиль"_q);
			controller->hideLayer();
		});
	}
}

} // namespace

void ShowProfileNftPicker(not_null<Window::SessionController*> controller) {
	if (!profileNftEnabled()) {
		controller->showToast(
			u"Включите NFT профиля в настройках Wenzgram"_q);
		return;
	}
	const auto session = &controller->session();
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(u"Выберите NFT"_q);
		box->setWidth(st::boxWideWidth);

		box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			u"Выберите один из ваших NFT-подарков Telegram. Он будет виден только вам в профиле Wenzgram."_q,
			st::boxLabel));

		const auto content = box->addRow(
			object_ptr<Ui::VerticalLayout>(box));
		const auto loading = Ui::CreateChild<Ui::FlatLabel>(
			content,
			rpl::single(u"Загрузка NFT..."_q),
			st::boxLabel);
		loading->show();

		const auto state = box->lifetime().make_state<QString>();
		const auto request = [=](QString offset) {
			Data::MyUniqueGiftsSlice(
				session,
				Data::MyUniqueType::OnlyOwned,
				offset
			) | rpl::on_next([=](Data::MyGiftsDescriptor &&descriptor) {
				loading->hide();
				if (descriptor.list.empty() && offset.isEmpty()) {
					content->add(
						object_ptr<Ui::FlatLabel>(
							content,
							u"У вас пока нет NFT-подарков в Telegram."_q,
							st::boxLabel));
					return;
				}
				AppendGiftRows(content, controller, descriptor.list);
				if (!descriptor.offset.isEmpty()
					&& *state != descriptor.offset) {
					*state = descriptor.offset;
					const auto more = AddButtonWithLabel(
						content,
						rpl::single(u"Показать ещё"_q),
						rpl::single(QString()),
						st::settingsButtonNoIcon);
					more->setClickedCallback([=] {
						more->hide();
						request(descriptor.offset);
					});
				}
			}, box->lifetime());
		};
		request(QString());

		if (hasOwn(session)) {
			box->addButton(rpl::single(u"Убрать NFT из профиля"_q), [=] {
				removeOwn(session);
				controller->showToast(u"NFT убран из профиля"_q);
				box->closeBox();
			});
		}

		box->addButton(tr::lng_close(), [=] { box->closeBox(); });
	}));
}

} // namespace Wenzgram::ProfileNft
