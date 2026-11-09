/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WINDOWBACKEND_WAYLAND_H
#define WINDOWBACKEND_WAYLAND_H

#include "windowbackend.h"

#include <QPointer>

namespace KWayland {
namespace Client {
class PlasmaWindow;
class PlasmaWindowManagement;
class Registry;
}
}

// KWin hands org_kde_plasma_window_management only to programs whose .desktop
// file asks for it (X-KDE-Wayland-Interfaces, data/lingmo-statusbar.desktop)
class WaylandWindowBackend : public WindowBackend
{
    Q_OBJECT

public:
    explicit WaylandWindowBackend(QObject *parent = nullptr);

    ActiveInfo activeWindow() const override;

    void close() override;
    void minimize() override;
    void restore() override;
    void maximize() override;
    void toggleMaximize() override;
    void move() override;

    QString appMenuServiceName() const override;
    QString appMenuObjectPath() const override;

private:
    void setManagement(KWayland::Client::PlasmaWindowManagement *wm);
    void onActiveWindowChanged();

    KWayland::Client::Registry *m_registry = nullptr;
    KWayland::Client::PlasmaWindowManagement *m_wm = nullptr;
    QPointer<KWayland::Client::PlasmaWindow> m_active;
};

#endif // WINDOWBACKEND_WAYLAND_H
