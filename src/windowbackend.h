/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WINDOWBACKEND_H
#define WINDOWBACKEND_H

#include <QObject>
#include <QString>

// The active window and what the bar can do with it. One implementation per
// session: X11 (KX11Extras/NETWM, windowbackend_x11.cpp) and Wayland
// (org_kde_plasma_window_management through KWayland, windowbackend_wayland.cpp).
class WindowBackend : public QObject
{
    Q_OBJECT

public:
    struct ActiveInfo {
        bool valid = false;
        // The desktop (wallpaper and icons) is what has the focus
        bool desktop = false;
        // A window the bar names: not a dock, panel, menu, transient of another window...
        bool acceptable = false;
        // X11: WM_CLASS class; Wayland: app id
        QString windowClass;
        QString title;
        quint32 pid = 0;
    };

    // The session's backend (created on first use)
    static WindowBackend *self();

    virtual ActiveInfo activeWindow() const = 0;

    virtual void close() = 0;
    virtual void minimize() = 0;
    virtual void restore() = 0;
    virtual void maximize() = 0;
    virtual void toggleMaximize() = 0;
    virtual void move() = 0;

    // Global menu of the active window (Wayland: org_kde_plasma_window); the
    // X11 menu keeps reading the window properties in AppMenuModel
    virtual QString appMenuServiceName() const { return QString(); }
    virtual QString appMenuObjectPath() const { return QString(); }

signals:
    // The active window changed, or something the bar shows about it
    void activeWindowChanged();
    void appMenuChanged();

protected:
    explicit WindowBackend(QObject *parent = nullptr);
};

#endif // WINDOWBACKEND_H
