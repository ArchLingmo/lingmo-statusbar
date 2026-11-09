/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WINDOWBACKEND_X11_H
#define WINDOWBACKEND_X11_H

#include "windowbackend.h"

class X11WindowBackend : public WindowBackend
{
    Q_OBJECT

public:
    explicit X11WindowBackend(QObject *parent = nullptr);

    ActiveInfo activeWindow() const override;

    void close() override;
    void minimize() override;
    void restore() override;
    void maximize() override;
    void toggleMaximize() override;
    void move() override;

private:
    bool isAcceptableWindow(quint64 wid) const;
};

#endif // WINDOWBACKEND_X11_H
