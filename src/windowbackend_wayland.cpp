/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "windowbackend_wayland.h"

#include <QDebug>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/registry.h>

using namespace KWayland::Client;

WaylandWindowBackend::WaylandWindowBackend(QObject *parent)
    : WindowBackend(parent)
{
    ConnectionThread *connection = ConnectionThread::fromApplication(this);
    if (!connection) {
        qWarning() << "statusbar: no Wayland connection, the active window stays unknown";
        return;
    }

    m_registry = new Registry(this);
    m_registry->create(connection);
    connect(m_registry, &Registry::plasmaWindowManagementAnnounced, this, [this](quint32 name, quint32 version) {
        setManagement(m_registry->createPlasmaWindowManagement(name, version, this));
    });
    connect(m_registry, &Registry::interfacesAnnounced, this, [this] {
        if (!m_wm)
            qWarning() << "statusbar: KWin did not offer org_kde_plasma_window_management"
                       << "(X-KDE-Wayland-Interfaces in lingmo-statusbar.desktop?)";
    });
    m_registry->setup();
    connection->roundtrip();
}

void WaylandWindowBackend::setManagement(PlasmaWindowManagement *wm)
{
    m_wm = wm;
    connect(m_wm, &PlasmaWindowManagement::activeWindowChanged, this, &WaylandWindowBackend::onActiveWindowChanged);
    connect(m_wm, &PlasmaWindowManagement::removed, this, [this] {
        m_wm = nullptr;
        onActiveWindowChanged();
    });
    onActiveWindowChanged();
}

void WaylandWindowBackend::onActiveWindowChanged()
{
    PlasmaWindow *active = m_wm ? m_wm->activeWindow() : nullptr;
    if (m_active != active) {
        if (m_active)
            disconnect(m_active, nullptr, this, nullptr);
        m_active = active;
        if (m_active) {
            // What the bar shows about it
            connect(m_active, &PlasmaWindow::titleChanged, this, &WindowBackend::activeWindowChanged);
            connect(m_active, &PlasmaWindow::appIdChanged, this, &WindowBackend::activeWindowChanged);
            connect(m_active, &PlasmaWindow::skipTaskbarChanged, this, &WindowBackend::activeWindowChanged);
            connect(m_active, &PlasmaWindow::applicationMenuChanged, this, &WindowBackend::appMenuChanged);
            connect(m_active, &PlasmaWindow::unmapped, this, &WaylandWindowBackend::onActiveWindowChanged);
        }
        emit appMenuChanged();
    }
    emit activeWindowChanged();
}

WindowBackend::ActiveInfo WaylandWindowBackend::activeWindow() const
{
    ActiveInfo a;
    if (!m_active || !m_active->isValid()) {
        // Nothing with the focus among the windows: the desktop (a layer surface,
        // not in the window list) has it, or nothing at all
        a.valid = m_wm != nullptr;
        a.desktop = a.valid;
        return a;
    }

    a.valid = true;
    a.windowClass = m_active->appId();
    a.title = m_active->title();
    a.pid = m_active->pid();
    // Like on X11: the bar names normal windows, not transients (dialogs of a
    // window) nor windows that stay off the dock
    a.acceptable = !m_active->skipTaskbar() && m_active->parentWindow().isNull();
    return a;
}

void WaylandWindowBackend::close()
{
    if (m_active)
        m_active->requestClose();
}

void WaylandWindowBackend::minimize()
{
    if (m_active && !m_active->isMinimized())
        m_active->requestToggleMinimized();
}

void WaylandWindowBackend::restore()
{
    if (m_active && m_active->isMaximized())
        m_active->requestToggleMaximized();
}

void WaylandWindowBackend::maximize()
{
    if (m_active && !m_active->isMaximized())
        m_active->requestToggleMaximized();
}

void WaylandWindowBackend::toggleMaximize()
{
    if (m_active && m_active->isMaximizeable())
        m_active->requestToggleMaximized();
}

void WaylandWindowBackend::move()
{
    if (m_active && m_active->isMovable())
        m_active->requestMove();
}

QString WaylandWindowBackend::appMenuServiceName() const
{
    return m_active ? m_active->applicationMenuServiceName() : QString();
}

QString WaylandWindowBackend::appMenuObjectPath() const
{
    return m_active ? m_active->applicationMenuObjectPath() : QString();
}
