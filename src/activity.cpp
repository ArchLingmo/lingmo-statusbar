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

#include "activity.h"
#include "windowbackend.h"

static const QStringList blockList = {"lingmo-launcher",
                                      "lingmo-statusbar"};

Activity::Activity(QObject *parent)
    : QObject(parent)
    , m_cApps(CApplications::self())
    , m_backend(WindowBackend::self())
{
    onActiveWindowChanged();

    connect(m_backend, &WindowBackend::activeWindowChanged, this, &Activity::onActiveWindowChanged);
}

bool Activity::launchPad() const
{
    return m_launchPad;
}

QString Activity::title() const
{
    return m_title;
}

QString Activity::icon() const
{
    return m_icon;
}

void Activity::close()
{
    m_backend->close();
}

void Activity::minimize()
{
    m_backend->minimize();
}

void Activity::restore()
{
    m_backend->restore();
}

void Activity::maximize()
{
    m_backend->maximize();
}

void Activity::toggleMaximize()
{
    m_backend->toggleMaximize();
}

void Activity::move()
{
    m_backend->move();
}

void Activity::onActiveWindowChanged()
{
    const WindowBackend::ActiveInfo info = m_backend->activeWindow();

    // On Wayland the launcher is a layer surface, never the active window
    m_launchPad = (info.windowClass == "lingmo-launcher");
    emit launchPadChanged();

    if (info.desktop) {
        m_title = QString();
        m_icon = "sysytemlogo";

        emit titleChanged();
        emit iconChanged();

        return;
    }

    if (!info.acceptable
            || blockList.contains(info.windowClass)) {
        clearTitle();
        clearIcon();
        return;
    }

    m_pid = info.pid;
    m_windowClass = info.windowClass.toLower();

    CAppItem *item = m_cApps->matchItem(m_pid, m_windowClass);

    if (item) {
        m_title = item->localName;
        emit titleChanged();

        if (m_icon != item->icon) {
            m_icon = item->icon;
            emit iconChanged();
        }

    } else {
        QString title = info.title;
        if (title != m_title) {
            m_title = title;
            emit titleChanged();
            clearIcon();
        }
    }
}

void Activity::clearTitle()
{
    m_title.clear();
    emit titleChanged();
}

void Activity::clearIcon()
{
    m_icon.clear();
    emit iconChanged();
}
