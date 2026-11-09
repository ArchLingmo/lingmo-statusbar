/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "windowbackend.h"
#include "windowbackend_wayland.h"
#include "windowbackend_x11.h"

#include <KWindowSystem>

WindowBackend::WindowBackend(QObject *parent)
    : QObject(parent)
{
}

WindowBackend *WindowBackend::self()
{
    static WindowBackend *backend = nullptr;
    if (!backend) {
        if (KWindowSystem::isPlatformWayland())
            backend = new WaylandWindowBackend;
        else
            backend = new X11WindowBackend;
    }
    return backend;
}
