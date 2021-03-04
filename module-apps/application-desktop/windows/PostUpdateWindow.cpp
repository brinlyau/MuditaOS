// Copyright (c) 2017-2021, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "PostUpdateWindow.hpp"

#include "service-appmgr/Controller.hpp"
#include "gui/widgets/BottomBar.hpp"
#include "gui/widgets/TopBar.hpp"
#include "RichTextParser.hpp"
#include "FontManager.hpp"

#include "application-desktop/data/AppDesktopStyle.hpp"
#include "Names.hpp"

#include <application-phonebook/ApplicationPhonebook.hpp>
#include <i18n/i18n.hpp>

using namespace gui;

PostUpdateWindow::PostUpdateWindow(app::Application *app)
    : AppWindow(app, app::window::name::desktop_post_update_window)
{
    buildInterface();
}

void PostUpdateWindow::onBeforeShow(ShowMode mode, SwitchData *data)
{
    setVisibleState();
}

void PostUpdateWindow::setVisibleState()
{
    successImage->setVisible(true);
    bottomBar->setActive(BottomBar::Side::CENTER, true);
}

bool PostUpdateWindow::onInput(const InputEvent &inputEvent)
{
    if (inputEvent.isShortPress()) {
        if (inputEvent.is(KeyCode::KEY_ENTER) && bottomBar->isActive(BottomBar::Side::CENTER)) {
            application->switchWindow(gui::name::window::main_window);
            return true;
        }
    }
    return AppWindow::onInput(inputEvent);
}

void PostUpdateWindow::rebuild()
{
    destroyInterface();
    buildInterface();
}

top_bar::Configuration PostUpdateWindow::configureTopBar(top_bar::Configuration appConfiguration)
{
    appConfiguration.enable(top_bar::Indicator::Time);
    appConfiguration.disable(top_bar::Indicator::Lock);
    appConfiguration.disable(top_bar::Indicator::Battery);
    appConfiguration.disable(top_bar::Indicator::NetworkAccessTechnology);
    appConfiguration.disable(top_bar::Indicator::Signal);
    appConfiguration.disable(top_bar::Indicator::SimCard);
    return appConfiguration;
}

void PostUpdateWindow::buildInterface()
{
    namespace post_update_style = style::window::pin_lock;
    AppWindow::buildInterface();

    setTitle(utils::localize.get("app_desktop_update_muditaos"));

    bottomBar->setText(BottomBar::Side::CENTER, utils::localize.get("common_ok"));

    successImage =
        new gui::Image(this, post_update_style::image::x, post_update_style::image::y, 0, 0, "circle_success");
    infoText = new Text(this,
                        post_update_style::primary_text::x,
                        post_update_style::primary_text::y,
                        post_update_style::primary_text::w,
                        post_update_style::primary_text::h);

    infoText->setText(utils::localize.get("app_desktop_update_success"));
    infoText->setAlignment(Alignment::Horizontal::Center);
}

void PostUpdateWindow::destroyInterface()
{
    erase();
    invalidate();
}

void PostUpdateWindow::invalidate() noexcept
{
    successImage = nullptr;
    infoText     = nullptr;
}