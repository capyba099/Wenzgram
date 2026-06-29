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
#include "ui/widgets/buttons.h"
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

void BuildLocalWallpapersSection(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"wenzgram/local_wallpapers"_q,
		.title = rpl::single(u"Локальные обои"_q),
		.keywords = { u"wallpaper"_q, u"background"_q, u"обои"_q },
	});

	AddBoolToggle(
		builder,
		u"wenzgram/local_wallpapers_enabled"_q,
		u"Локальные обои для чатов"_q,
		Wenzgram::kLocalChatWallpapersKey,
		true,
		{ u"local"_q, u"chat"_q, u"обои"_q });

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

	builder.addButton({
		.id = u"wenzgram/local_wallpapers_hint"_q,
		.title = rpl::single(
			u"Выберите чат → меню ⋮ → «Локальные обои»"_q),
		.st = &st::settingsButtonNoIcon,
		.keywords = { u"how"_q, u"help"_q },
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

void BuildBehaviorSection(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"wenzgram/behavior"_q,
		.title = rpl::single(u"Поведение"_q),
		.keywords = { u"behavior"_q, u"actions"_q },
	});

	AddBoolToggle(
		builder,
		u"wenzgram/confirm_send"_q,
		u"Подтверждать отправку сообщений"_q,
		Wenzgram::kConfirmBeforeSendKey,
		false);
	AddBoolToggle(
		builder,
		u"wenzgram/copy_username"_q,
		u"Копировать @username по клику"_q,
		Wenzgram::kCopyUsernameOnClickKey,
		false);
	AddBoolToggle(
		builder,
		u"wenzgram/disable_animations"_q,
		u"Отключить анимации в чатах"_q,
		Wenzgram::kDisableChatAnimationsKey,
		false);
	AddBoolToggle(
		builder,
		u"wenzgram/hide_folders"_q,
		u"Скрывать панель папок чатов"_q,
		Wenzgram::kHideChatFoldersKey,
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
	const auto versionLabel = u"Текущая версия: "_q
		+ Wenzgram::Updater::Service::Instance().currentVersion();

	builder.addButton({
		.id = u"wenzgram/update_status"_q,
		.title = rpl::single(versionLabel),
		.st = &st::settingsButtonNoIcon,
		.keywords = { u"version"_q },
	});

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

void BuildSecuritySection(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"wenzgram/security"_q,
		.title = rpl::single(u"Безопасность Wenzgram"_q),
		.keywords = { u"security"_q, u"session"_q, u"защита"_q },
	});

	builder.addButton({
		.id = u"wenzgram/session_binding"_q,
		.title = rpl::single(u"Привязка сессии к устройству: включена"_q),
		.st = &st::settingsButtonNoIcon,
		.keywords = { u"device"_q, u"tdata"_q, u"защита"_q },
	});
	builder.addButton({
		.id = u"wenzgram/session_binding_hint"_q,
		.title = rpl::single(
			u"Скопированная папка tdata не откроется на другом ПК"_q),
		.st = &st::settingsButtonNoIcon,
		.keywords = { u"hint"_q },
	});
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
	BuildLocalWallpapersSection(builder);
	BuildProfileNftSection(builder);
	BuildAppearanceSection(builder);
	BuildBehaviorSection(builder);
	BuildUpdatesSection(builder);
	BuildSecuritySection(builder);
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
	Ui::ResizeFitChild(this, content);
}

} // namespace

Type WenzgramId() {
	return WenzgramSettings::Id();
}

} // namespace Settings
