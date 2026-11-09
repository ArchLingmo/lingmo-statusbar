/*
 * SPDX-FileCopyrightText: 2024-2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// X11 session: the code that used to live in activity.cpp, unchanged

#include "windowbackend_x11.h"

#include <QCursor>
#include <xcb/xcb.h>

#include <NETWM>
#include <KWindowSystem>
#include <KX11Extras>
#include <KWindowInfo>

static xcb_connection_t *x11Connection()
{
    static xcb_connection_t *conn = nullptr;
    if (!conn) {
        conn = xcb_connect(nullptr, nullptr);
    }
    if (!conn || xcb_connection_has_error(conn)) {
        return nullptr;
    }
    return conn;
}

static xcb_window_t x11RootWindow()
{
    xcb_connection_t *conn = x11Connection();
    if (!conn) {
        return XCB_WINDOW_NONE;
    }
    const xcb_setup_t *setup = xcb_get_setup(conn);
    if (!setup) {
        return XCB_WINDOW_NONE;
    }
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    return it.rem ? it.data->root : XCB_WINDOW_NONE;
}

X11WindowBackend::X11WindowBackend(QObject *parent)
    : WindowBackend(parent)
{
    connect(KX11Extras::self(), &KX11Extras::activeWindowChanged, this, &WindowBackend::activeWindowChanged);
    connect(KX11Extras::self(), static_cast<void (KX11Extras::*)(WId id, NET::Properties properties, NET::Properties2 properties2)>(&KX11Extras::windowChanged),
            this, &WindowBackend::activeWindowChanged);
}

WindowBackend::ActiveInfo X11WindowBackend::activeWindow() const
{
    const WId wid = KX11Extras::activeWindow();
    KWindowInfo info(wid,
                     NET::WMState | NET::WMVisibleName | NET::WMWindowType,
                     NET::WM2WindowClass);

    ActiveInfo a;
    a.valid = info.valid();
    a.windowClass = QString::fromUtf8(info.windowClassClass());
    a.desktop = NET::typeMatchesMask(info.windowType(NET::AllTypesMask), NET::DesktopMask);
    a.acceptable = isAcceptableWindow(wid);
    a.title = info.visibleName();
    a.pid = info.pid();
    return a;
}

void X11WindowBackend::close()
{
    if (auto *conn = x11Connection()) {
        NETRootInfo(conn, NET::CloseWindow).closeWindowRequest(KX11Extras::activeWindow());
    }
}

void X11WindowBackend::minimize()
{
    KX11Extras::minimizeWindow(KX11Extras::activeWindow());
}

void X11WindowBackend::restore()
{
    KX11Extras::clearState(KX11Extras::activeWindow(), NET::Max);
}

void X11WindowBackend::maximize()
{
    KX11Extras::setState(KX11Extras::activeWindow(), NET::Max);
}

void X11WindowBackend::toggleMaximize()
{
    KWindowInfo info(KX11Extras::activeWindow(), NET::WMState);
    bool isWindow = !info.hasState(NET::SkipTaskbar) ||
                     info.windowType(NET::UtilityMask) != NET::Utility ||
                     info.windowType(NET::DesktopMask) != NET::Desktop;

    if (!isWindow)
        return;

    bool isMaximized = info.hasState(NET::Max);
    isMaximized ? restore() : maximize();
}

void X11WindowBackend::move()
{
    WId winId = KX11Extras::activeWindow();
    KWindowInfo info(winId, NET::WMState | NET::WMGeometry | NET::WMDesktop);
    bool window = !info.hasState(NET::SkipTaskbar) ||
                     info.windowType(NET::UtilityMask) != NET::Utility ||
                     info.windowType(NET::DesktopMask) != NET::Desktop;

    if (!window)
        return;

    bool onCurrent = info.isOnCurrentDesktop();
    if (!onCurrent) {
        KX11Extras::setCurrentDesktop(info.desktop());
        KX11Extras::forceActiveWindow(winId);
    }

    if (auto *conn = x11Connection()) {
        NETRootInfo ri(conn, NET::WMMoveResize);
        ri.moveResizeRequest(winId,
                             QCursor::pos().x(),
                             QCursor::pos().y(), NET::Move);
    }
}

bool X11WindowBackend::isAcceptableWindow(quint64 wid) const
{
    QFlags<NET::WindowTypeMask> ignoreList;
    ignoreList |= NET::DesktopMask;
    ignoreList |= NET::DockMask;
    ignoreList |= NET::SplashMask;
    ignoreList |= NET::ToolbarMask;
    ignoreList |= NET::MenuMask;
    ignoreList |= NET::PopupMenuMask;
    ignoreList |= NET::NotificationMask;

    KWindowInfo info(wid, NET::WMWindowType | NET::WMState, NET::WM2TransientFor | NET::WM2WindowClass);

    if (!info.valid())
        return false;

    if (NET::typeMatchesMask(info.windowType(NET::AllTypesMask), ignoreList))
        return false;

    if (info.hasState(NET::SkipTaskbar) || info.hasState(NET::SkipPager))
        return false;

    // WM_TRANSIENT_FOR hint not set - normal window
    WId transFor = info.transientFor();
    if (transFor == 0 || transFor == wid || transFor == (WId) x11RootWindow())
        return true;

    info = KWindowInfo(transFor, NET::WMWindowType);

    QFlags<NET::WindowTypeMask> normalFlag;
    normalFlag |= NET::NormalMask;
    normalFlag |= NET::DialogMask;
    normalFlag |= NET::UtilityMask;

    return !NET::typeMatchesMask(info.windowType(NET::AllTypesMask), normalFlag);
}
