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

#include "statusbar.h"
#include "battery.h"
#include "processprovider.h"
#include "appmenu/appmenu.h"
#include "statusbaradaptor.h"
#include "lyricshelper.h"
#include "permissionsurveillance.h"
#include <QQmlEngine>
#include <QQmlContext>

#include <QDBusConnection>
#include <QApplication>
#include <QSettings>
#include <QScreen>

#include <NETWM>
#include <KWindowSystem>
#include <KX11Extras>
#include <KWindowEffects>

#include <LayerShellQt/Window>

static const int s_defaultHeight = 28;
static const int s_minHeight = 24;
static const int s_maxHeight = 40;

StatusBar::StatusBar(QQuickView *parent)
    : QQuickView(parent)
    , m_acticity(new Activity)
{
    QSettings settings("lingmoos", "locale");
    m_twentyFourTime = settings.value("twentyFour", false).toBool();

    QSettings barSettings(QSettings::UserScope, "lingmoos", "statusbar");
    m_barHeight = qBound(s_minHeight, barSettings.value("Height", s_defaultHeight).toInt(), s_maxHeight);

    setFlags(Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    setColor(Qt::transparent);

    if (KWindowSystem::isPlatformWayland()) {
        // A panel in the layer above the windows, across the top of its screen,
        // reserving its height (the X11 strut)
        m_layerShell = LayerShellQt::Window::get(this);
        m_layerShell->setScope(QStringLiteral("lingmo-statusbar"));
        m_layerShell->setLayer(LayerShellQt::Window::LayerTop);
        m_layerShell->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop
                                                               | LayerShellQt::Window::AnchorLeft
                                                               | LayerShellQt::Window::AnchorRight));
        m_layerShell->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
        m_layerShell->setActivateOnShow(false);
        m_layerShell->setExclusiveZone(m_barHeight);
        m_layerShell->setScreen(qApp->primaryScreen());
    } else {
        KX11Extras::setOnDesktop(winId(), NET::OnAllDesktops);
        KX11Extras::setType(winId(), NET::Dock);
    }

    new StatusbarAdaptor(this);
    new AppMenu(this);

    engine()->rootContext()->setContextProperty("StatusBar", this);
    engine()->rootContext()->setContextProperty("acticity", m_acticity);
    engine()->rootContext()->setContextProperty("process", new ProcessProvider);
    engine()->rootContext()->setContextProperty("lyricsHelper", new LyricsHelper);
    engine()->rootContext()->setContextProperty("permissionSurveillance", new PermissionSurveillance);
    engine()->rootContext()->setContextProperty("battery", Battery::self());

    setSource(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    setResizeMode(QQuickView::SizeRootObjectToView);
    setScreen(qApp->primaryScreen());
    updateGeometry();
    setVisible(true);
    initState();

    connect(m_acticity, &Activity::launchPadChanged, this, &StatusBar::initState);

    // Always on the primary screen. Monitors moved, rotated, plugged in or made primary
    // change several screens at once, and KWin may move the bar while they settle
    // (which makes Qt think it now lives on another screen), so every such change
    // just schedules one look at where the primary screen is now.
    m_relayout = new QTimer(this);
    m_relayout->setSingleShot(true);
    m_relayout->setInterval(250);
    connect(m_relayout, &QTimer::timeout, this, &StatusBar::followPrimaryScreen);

    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &StatusBar::onPrimaryScreenChanged);
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        watchScreen(screen);
        m_relayout->start();
    });
    connect(qGuiApp, &QGuiApplication::screenRemoved, m_relayout, qOverload<>(&QTimer::start));
    connect(this, &QWindow::screenChanged, m_relayout, qOverload<>(&QTimer::start));
    for (QScreen *screen : qGuiApp->screens())
        watchScreen(screen);

    // The primary screen may have changed while the QML above was loading: the session
    // runs autostart entries (xrandr layouts, ...) as soon as the desktop is up
    followPrimaryScreen();
}

void StatusBar::watchScreen(QScreen *screen)
{
    connect(screen, &QScreen::geometryChanged, m_relayout, qOverload<>(&QTimer::start));
    connect(screen, &QScreen::virtualGeometryChanged, m_relayout, qOverload<>(&QTimer::start));
}

void StatusBar::followPrimaryScreen()
{
    QScreen *primary = qApp->primaryScreen();
    if (!primary)
        return;
    if (m_layerShell && m_layerShell->screen() != primary) {
        // The output of a layer surface is fixed when it is created
        const bool visible = isVisible();
        setVisible(false);
        setScreen(primary);
        m_layerShell->setScreen(primary);
        updateGeometry();
        setVisible(visible);
        return;
    }
    if (screen() != primary)
        setScreen(primary);
    updateGeometry();
}

QRect StatusBar::screenRect()
{
    return m_screenRect;
}

bool StatusBar::twentyFourTime()
{
    return m_twentyFourTime;
}

void StatusBar::setBatteryPercentage(bool enabled)
{
    Battery::self()->setShowPercentage(enabled);
}

void StatusBar::setTwentyFourTime(bool t)
{
    if (m_twentyFourTime != t) {
        m_twentyFourTime = t;
        emit twentyFourTimeChanged();
    }
}

void StatusBar::setHeight(int height)
{
    height = qBound(s_minHeight, height, s_maxHeight);
    if (height == m_barHeight)
        return;

    m_barHeight = height;
    QSettings barSettings(QSettings::UserScope, "lingmoos", "statusbar");
    barSettings.setValue("Height", height);
    emit barHeightChanged();
    updateGeometry();
}

void StatusBar::updateGeometry()
{
    // The primary screen's, not screen(): Qt moves that to whichever screen the
    // window happens to be over
    QScreen *primary = qApp->primaryScreen() ? qApp->primaryScreen() : screen();
    const QRect rect = primary->geometry();

    if (m_screenRect != rect) {
        m_screenRect = rect;
        emit screenRectChanged();
    }

    QRect windowRect = QRect(rect.x(), rect.y(), rect.width(), m_barHeight);
    setGeometry(windowRect);
    updateViewStruts();
    // The blur behind the bar is asked for by its glass (qml/main.qml,
    // LingmoUI.GlassSurface), which drops it with Glass off or no compositor
}

void StatusBar::updateViewStruts()
{
    if (m_layerShell) {
        m_layerShell->setExclusiveZone(m_barHeight);
        return;
    }

    const QRect wholeScreen(QPoint(0, 0), (qApp->primaryScreen() ? qApp->primaryScreen() : screen())->virtualSize());
    const QRect rect = geometry();
    const int topOffset = (qApp->primaryScreen() ? qApp->primaryScreen() : screen())->geometry().top();

    NETExtendedStrut strut;
    strut.top_width = rect.height() + topOffset - 1;
    strut.top_start = rect.x();
    strut.top_end = rect.x() + rect.width() - 1;

    KX11Extras::setExtendedStrut(winId(),
                                 strut.left_width,
                                 strut.left_start,
                                 strut.left_end,
                                 strut.right_width,
                                 strut.right_start,
                                 strut.right_end,
                                 strut.top_width,
                                 strut.top_start,
                                 strut.top_end,
                                 strut.bottom_width,
                                 strut.bottom_start,
                                 strut.bottom_end);
}

void StatusBar::initState()
{
    // Wayland: the bar is in the Top layer and the launcher in Overlay, above it
    if (m_layerShell)
        return;

    // Remain below the face launchpad.
    KX11Extras::setState(winId(), m_acticity->launchPad() ? NET::KeepBelow : NET::KeepAbove);
}

void StatusBar::onPrimaryScreenChanged(QScreen *)
{
    m_relayout->start();
}
