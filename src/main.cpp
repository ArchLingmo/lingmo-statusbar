/*
 * Copyright (C) 2024 LingmoOS Team.
 *
 * Author:     lingmoos <lingmoos@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_LINGMO_INTENTS
#include "island/islandintents.h"
#endif
#include <QApplication>
#include <QTranslator>
#include <QLocale>

#include "statusbar.h"
#include "controlcenterdialog.h"
#include "systemtray/systemtraymodel.h"
#include "appmenu/appmenumodel.h"
#include "appmenu/appmenuapplet.h"
#include "poweractions.h"
#include "notifications.h"
#include "spotlightshortcut.h"
#include "backgroundhelper.h"
#include "lyricshelper.h"
#include "permissionsurveillance.h"
#include "appearance.h"
#include "brightness.h"
#include "battery.h"
#include "nightlight.h"
#include "keyboardlayout.h"
#include "powerprofiles.h"
#include "island/islandmanager.h"

int main(int argc, char *argv[])
{
    // X11 session: always through xcb, as before. In a Wayland session the bar is
    // a native Wayland client (layer-shell panel), so nothing is forced there.
    if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY"))
        qputenv("QT_QPA_PLATFORM", "xcb");
    QApplication app(argc, argv);

    const char *uri = "Lingmo.StatusBar";
    // @uri Lingmo.StatusBar
    qmlRegisterType<SystemTrayModel>(uri, 1, 0, "SystemTrayModel");
    qmlRegisterType<ControlCenterDialog>(uri, 1, 0, "ControlCenterDialog");
    qmlRegisterType<Appearance>(uri, 1, 0, "Appearance");
    qmlRegisterType<Brightness>(uri, 1, 0, "Brightness");
    qmlRegisterType<Battery>(uri, 1, 0, "Battery");
    qmlRegisterType<AppMenuModel>(uri, 1, 0, "AppMenuModel");
    qmlRegisterType<AppMenuApplet>(uri, 1, 0, "AppMenuApplet");
    qmlRegisterType<PowerActions>(uri, 1, 0, "PowerActions");
    qmlRegisterType<Notifications>(uri, 1, 0, "Notifications");
    qmlRegisterType<SpotlightShortcut>(uri, 1, 0, "SpotlightShortcut");
    qmlRegisterType<LyricsHelper>(uri, 1, 0, "LyricsHelper");
    qmlRegisterType<BackgroundHelper>(uri, 1, 0, "BackgroundHelper");
    qmlRegisterType<PermissionSurveillance>(uri, 1, 0, "PermissionSurveillance");
    qmlRegisterType<NightLight>(uri, 1, 0, "NightLight");
    qmlRegisterType<KeyboardLayout>(uri, 1, 0, "KeyboardLayout");
    qmlRegisterType<PowerProfiles>(uri, 1, 0, "PowerProfiles");
    qmlRegisterType<IslandManager>(uri, 1, 0, "IslandManager");

    QString qmFilePath = QString("%1/%2.qm").arg("/usr/share/lingmo-statusbar/translations/").arg(QLocale::system().name());
    if (QFile::exists(qmFilePath)) {
        QTranslator *translator = new QTranslator(QApplication::instance());
        if (translator->load(qmFilePath)) {
            QGuiApplication::installTranslator(translator);
        } else {
            translator->deleteLater();
        }
    }

    StatusBar bar;

    if (!QDBusConnection::sessionBus().registerService("com.lingmo.Statusbar")) {
        return -1;
    }

    if (!QDBusConnection::sessionBus().registerObject("/Statusbar", &bar)) {
        return -1;
    }

#ifdef HAVE_LINGMO_INTENTS
    // island.start / update / finish on com.lingmo.Island /Intents
    publishIslandIntents(&app);
#endif

    return app.exec();
}
