/*
This file is part of Wenzgram,
a Telegram Desktop fork with extended client settings.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_wenzgram.h"

#include "settings/settings_common_session.h"

#include "settings/settings_builder.h"
#include "settings/sections/settings_main.h"
#include "lang/lang_keys.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "wenzgram/wenzgram_profile_nft.h"
#include "wenzgram/wenzgram_settings.h"
#include "wenzgram/wenzgram_updater.h"
#include "wenzgram/wenzgram_version.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

namespace Settings {
namespace {

using namespace Builder;

void AddBoolToggle(
		SectionBuilder &builder,
		const QString &id,
		const QString &title,
		std::string_view key,
		bool fallback,
		QStringList keywords = {}) {
	const auto toggle = builder.addButton({
		.id = id,
		.title = rpl::single(title),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(Wenzgram::readBool(key, fallback)),
		.keywords = std::move(keywords),
	});
	if (toggle) {
		toggle->toggledValue(
		) | rpl::filter([=](bool value) {
			return (value != Wenzgram::readBool(key, fallback));
		}) | rpl::on_next([=](bool value) {
			Wenzgram::writeBool(key, value);
		}, toggle->lifetime());
	}
}

void BuildDeletedMessagesSection(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"wenzgram/messages"_q,
		.title = rpl::single(u"Сообщения"_q),
		.keywords = { u"deleted"_q, u"messages"_q },
	});

	AddBoolToggle(
		builder,
		u"wenzgram/show_deleted_messages"_q,
		u"Показывать удалённые сообщения"_q,
		Wenzgram::kShowDeletedMessagesKey,
		true,
		{ u"deleted"_q, u"removed"_q, u"удалено"_q });

	builder.addButton({
		.id = u"wenzgram/deleted_messages_hint"_q,
		.title = rpl::single(
			u"Удалённые сообщения остаются в чате с меткой «УДАЛЕНО»"_q),
		.st = &st::settingsButtonNoIcon,
		.keywords = { u"deleted"_q, u"hint"_q },
	});
}

void BuildProfileNftSection(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"wenzgram/profile_nft"_q,
		.title = rpl::single(u"NFT профиля"_q),
		.keywords = { u"nft"_q, u"profile"_q, u"аватар"_q },
	});

	AddBoolToggle(
		builder,
		u"wenzgram/profile_nft_enabled"_q,
		u"Локальные NFT в профиле"_q,
		Wenzgram::kProfileNftKey,
		true,
		{ u"nft"_q, u"profile"_q });

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"wenzgram/profile_nft_manage"_q,
		.title = rpl::single(u"Настроить NFT профиля"_q),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			Wenzgram::ProfileNft::showManager(controller);
		},
		.keywords = { u"nft"_q, u"choose"_q },
	});

	builder.addButton({
		.id = u"wenzgram/profile_nft_hint"_q,
		.title = rpl::single(
			u"NFT видят пользователи Wenzgram. Настройка: Профиль → Изменить"_q),
		.st = &st::settingsButtonNoIcon,
		.keywords = { u"hint"_q, u"how"_q },
	});
}

void BuildAppearanceSection(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"wenzgram/appearance"_q,
		.title = rpl::single(u"Внешний вид"_q),
		.keywords = { u"appearance"_q, u"ui"_q },
	});

	AddBoolToggle(
		builder,
		u"wenzgram/show_branding"_q,
		u"Показывать название Wenzgram"_q,
		Wenzgram::kShowBrandingKey,
		true);
	AddBoolToggle(
		builder,
		u"wenzgram/compact_dialogs"_q,
		u"Компактный список чатов"_q,
		Wenzgram::kCompactDialogsKey,
		false);
	AddBoolToggle(
		builder,
		u"wenzgram/large_emoji"_q,
		u"Крупные эмодзи в сообщениях"_q,
		Wenzgram::kLargeEmojiKey,
		false);
	AddBoolToggle(
		builder,
		u"wenzgram/hide_edited"_q,
		u"Скрывать метку «изменено»"_q,
		Wenzgram::kHideEditedBadgeKey,
		false);
	AddBoolToggle(
		builder,
		u"wenzgram/show_seconds"_q,
		u"Показывать секунды во времени"_q,
		Wenzgram::kShowSecondsInTimeKey,
		false);
}

void BuildUpdatesSection(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"wenzgram/updates"_q,
		.title = rpl::single(u"Обновления"_q),
		.keywords = { u"update"_q, u"updater"_q, u"release"_q },
	});

	AddBoolToggle(
		builder,
		u"wenzgram/auto_update"_q,
		u"Проверять обновления автоматически"_q,
		Wenzgram::kAutoUpdateKey,
		true,
		{ u"update"_q, u"github"_q });

	const auto controller = builder.controller();

	const auto install = builder.addButton({
		.id = u"wenzgram/install_update"_q,
		.title = rpl::single(u"Установить обновление"_q),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] { Wenzgram::Updater::Service::Instance().installUpdate(); },
		.keywords = { u"install"_q },
	});
	if (install) {
		install->setVisible(Wenzgram::Updater::Service::Instance().isReady());
	}

	builder.addButton({
		.id = u"wenzgram/check_update"_q,
		.title = rpl::single(u"Проверить обновления"_q),
		.st = &st::settingsButtonNoIcon,
		.onClick = [=] {
			Wenzgram::Updater::Service::Instance().checkNow();
			controller->showToast(u"Проверяем обновления на GitHub..."_q);
		},
		.keywords = { u"check"_q },
	});

	const auto updater = &Wenzgram::Updater::Service::Instance();
	if (install) {
		updater->ready() | rpl::on_next([=] {
			install->setVisible(true);
			controller->showToast(
				u"Доступна версия "_q + updater->latestVersion());
		}, install->lifetime());

		updater->isLatest() | rpl::on_next([=] {
			controller->showToast(u"Установлена последняя версия"_q);
		}, install->lifetime());

		updater->failed() | rpl::on_next([=] {
			controller->showToast(u"Не удалось проверить обновления"_q);
		}, install->lifetime());
	}
}

class WenzgramSettings : public Section<WenzgramSettings> {
public:
	WenzgramSettings(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

const auto kMeta = BuildHelper({
	.id = WenzgramSettings::Id(),
	.parentId = MainId(),
	.title = &tr::lng_settings_experimental,
	.icon = &st::menuIconManage,
}, [](SectionBuilder &builder) {
	BuildDeletedMessagesSection(builder);
	BuildProfileNftSection(builder);
	BuildAppearanceSection(builder);
	BuildUpdatesSection(builder);
});

const SectionBuildMethod kWenzgramSection = kMeta.build;

WenzgramSettings::WenzgramSettings(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> WenzgramSettings::title() {
	return rpl::single(u"Wenzgram"_q);
}

void WenzgramSettings::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kWenzgramSection);

	Ui::AddSkip(content);
	const auto versionWrap = content->add(object_ptr<Ui::FixedHeightWidget>(
		content,
		st::boxLabel.style.font->height + st::settingsThumbSkip));
	const auto version = Ui::CreateChild<Ui::FlatLabel>(
		versionWrap,
		rpl::single(
			u"Текущая версия: "_q + QLatin1String(Wenzgram::kVersion)),
		st::boxLabel);
	version->setAttribute(Qt::WA_TransparentForMouseEvents);
	versionWrap->widthValue(
	) | rpl::on_next([=](int width) {
		version->resizeToWidth(width);
		version->moveToLeft((width - version->width()) / 2, 0);
	}, version->lifetime());

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type WenzgramId() {
	return WenzgramSettings::Id();
}

} // namespace Settings
