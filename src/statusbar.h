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

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QQuickView>
#include <QTimer>
#include "activity.h"

namespace LayerShellQt {
class Window;
}

class StatusBar : public QQuickView
{
    Q_OBJECT
    Q_PROPERTY(QRect screenRect READ screenRect NOTIFY screenRectChanged)
    Q_PROPERTY(bool twentyFourTime READ twentyFourTime NOTIFY twentyFourTimeChanged)
    Q_PROPERTY(int barHeight READ barHeight NOTIFY barHeightChanged)

public:
    explicit StatusBar(QQuickView *parent = nullptr);

    QRect screenRect();
    bool twentyFourTime();

    void setBatteryPercentage(bool enabled);
    void setTwentyFourTime(bool t);

    // Height of the bar, ~/.config/lingmoos/statusbar.conf [General] Height
    int barHeight() const { return m_barHeight; }
    void setHeight(int height);

    void updateGeometry();
    void updateViewStruts();

signals:
    void screenRectChanged();
    void launchPadChanged();
    void twentyFourTimeChanged();
    void barHeightChanged();

private slots:
    void initState();
    void onPrimaryScreenChanged(QScreen *screen);
    // Back on top of the primary screen, wherever it is now
    void followPrimaryScreen();

private:
    void watchScreen(QScreen *screen);

    QTimer *m_relayout;
    QRect m_screenRect;
    Activity *m_acticity;
    bool m_twentyFourTime;
    int m_barHeight;
    // Wayland: the bar's layer surface (null on X11)
    LayerShellQt::Window *m_layerShell = nullptr;
};

#endif // STATUSBAR_H
