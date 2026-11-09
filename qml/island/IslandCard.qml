/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Window

import Lingmo.StatusBar 1.0
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// The open Island: a window over the middle of the bar where one glass
// surface springs from the capsule's shape into the card and back. The
// capsule hides while this window is up, so the two read as one object.
Window {
    id: win

    property var island
    property var manager
    property Item capsuleItem

    // Open (or opening); false while it springs back and once closed
    property bool expanded: false
    // Opened by a click: stays open while the pointer is away
    property bool pinned: false
    // The card covers the capsule (the capsule hides)
    readonly property bool surfaceShown: visible

    readonly property int shadowMargin: 28
    readonly property int cardWidth: 380
    readonly property int padding: LingmoUI.Units.largeSpacing + 2

    readonly property bool springs: LingmoUI.Units.springEnabled
    // Card geometry: while opening/open, and with "reduce motion" until the
    // fade-out is over (it fades in place instead of shrinking)
    readonly property bool cardShape: expanded || (!springs && visible)

    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool | Qt.WindowDoesNotAcceptFocus
    transientParent: null
    visible: false

    width: cardWidth + shadowMargin * 2
    height: Math.max(capRect.y + capRect.height, expRect.y + expRect.height) + shadowMargin
    x: StatusBar.screenRect.x + Math.round((StatusBar.screenRect.width - width) / 2)
    y: StatusBar.screenRect.y

    // Wayland: over everything, centred at the top of the bar's screen (the x/y
    // above, which KWin can't take from a client there); on top of the bar, so
    // it ignores the bar's reserved room. Nothing changes on X11.
    LingmoUI.LayerShell.layer: LingmoUI.LayerShell.LayerOverlay
    LingmoUI.LayerShell.anchors: LingmoUI.LayerShell.AnchorTop
    LingmoUI.LayerShell.exclusiveZone: -1
    LingmoUI.LayerShell.keyboardInteractivity: LingmoUI.LayerShell.KeyboardNone
    LingmoUI.LayerShell.activateOnShow: false
    LingmoUI.LayerShell.scope: "lingmo-island"
    LingmoUI.LayerShell.screen: StatusBar.screen

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    // The capsule in this window's coordinates
    property rect capRect: Qt.rect(0, 3, 120, 22)

    function updateCapRect() {
        if (!capsuleItem)
            return
        const g = capsuleItem.mapToGlobal(0, 0)
        capRect = Qt.rect(g.x - win.x, g.y - win.y, capsuleItem.width, capsuleItem.height)
    }

    // The card, under the capsule's top edge
    readonly property rect expRect: Qt.rect(shadowMargin, capRect.y, cardWidth,
                                            _expanded.implicitHeight + padding * 2)

    function expand(pin) {
        _closeTimer.stop()
        if (pin !== undefined)
            pinned = pin || pinned
        if (expanded)
            return
        updateCapRect()
        // Start exactly on the capsule, without animating there
        _surface.animate = false
        _surface.snapToCapsule()
        visible = true
        manager.prepareWindow(win)
        _surface.animate = true
        expanded = true
    }

    function collapse() {
        _closeTimer.stop()
        pinned = false
        if (!expanded)
            return
        updateCapRect()
        expanded = false
        if (!springs)
            _hideTimer.restart()
    }

    // Pointer moved between the capsule and the card
    function pointerMoved() {
        if (!expanded)
            return
        if (_hover.hovered || island.capsuleHovered) {
            _closeTimer.stop()
        } else {
            _closeTimer.interval = pinned ? 8000 : 450
            _closeTimer.restart()
        }
    }

    Timer {
        id: _closeTimer
        onTriggered: if (!_hover.hovered && !win.island.capsuleHovered) win.collapse()
    }

    // Without springs: hide once the short animation is over
    Timer {
        id: _hideTimer
        interval: LingmoUI.Units.shortDuration + 30
        onTriggered: if (!win.expanded) win.visible = false
    }

    // The capsule moved or changed size (another activity, new text)
    Connections {
        target: win.capsuleItem
        function onWidthChanged() { if (win.visible) win.updateCapRect() }
        function onXChanged() { if (win.visible) win.updateCapRect() }
    }

    Item {
        id: _surface

        property bool animate: true

        // 0 = capsule, 1 = card (drives the cross-fade of the contents)
        readonly property real morph: Math.max(0, Math.min(1, (height - win.capRect.height)
                                                           / Math.max(1, win.expRect.height - win.capRect.height)))

        x: win.cardShape ? win.expRect.x : win.capRect.x
        y: win.cardShape ? win.expRect.y : win.capRect.y
        width: win.cardShape ? win.expRect.width : win.capRect.width
        height: win.cardShape ? win.expRect.height : win.capRect.height
        property real radius: win.cardShape ? LingmoUI.Theme.hugeRadius + 8 : win.capRect.height / 2

        function snapToCapsule() {
            x = Qt.binding(() => win.cardShape ? win.expRect.x : win.capRect.x)
            y = Qt.binding(() => win.cardShape ? win.expRect.y : win.capRect.y)
            width = Qt.binding(() => win.cardShape ? win.expRect.width : win.capRect.width)
            height = Qt.binding(() => win.cardShape ? win.expRect.height : win.capRect.height)
        }

        // Spring morph (or a short ease with "reduce motion")
        component Morph: SpringAnimation {
            spring: LingmoUI.Units.spring
            damping: LingmoUI.Units.springDamping
            mass: LingmoUI.Units.springMass
            epsilon: LingmoUI.Units.springEpsilon
        }
        Behavior on x { enabled: _surface.animate && win.springs; Morph {} }
        Behavior on y { enabled: _surface.animate && win.springs; Morph {} }
        Behavior on width { enabled: _surface.animate && win.springs; Morph {} }
        Behavior on height { enabled: _surface.animate && win.springs; Morph {} }
        Behavior on radius { enabled: _surface.animate && win.springs; Morph {} }

        // Reduce motion: the card fades in place instead of morphing
        opacity: win.springs ? 1 : (win.expanded ? 1 : 0)
        Behavior on opacity { NumberAnimation { duration: LingmoUI.Units.shortDuration } }

        // Back on the capsule: hide the window, the capsule takes over
        function settle() {
            if (!win.expanded && win.springs
                    && Math.abs(width - win.capRect.width) < 1 && Math.abs(height - win.capRect.height) < 1)
                win.visible = false
        }
        onWidthChanged: settle()
        onHeightChanged: settle()

        LingmoUI.GlassSurface {
            id: _glass
            anchors.fill: parent
            radius: _surface.radius
            // The window is bigger than the card (room for the shadow)
            blurMode: 1 // GlassSurface.Always
            elevated: _surface.morph > 0.2
        }

        Item {
            anchors.fill: parent
            clip: true

            // Compact look while it is still capsule-sized
            IslandCompact {
                anchors.fill: parent
                anchors.leftMargin: win.capRect.height * 0.25
                anchors.rightMargin: win.capRect.height * 0.25
                height: win.capRect.height
                opacity: 1 - Math.min(1, _surface.morph * 3)
                visible: opacity > 0
                island: win.island
                activity: win.island.primary
                now: win.island.now
            }

            IslandExpanded {
                id: _expanded
                x: win.padding
                y: win.padding
                width: win.cardWidth - win.padding * 2
                opacity: Math.max(0, (_surface.morph - 0.35) / 0.65)
                visible: opacity > 0
                island: win.island
                card: win
            }
        }

        HoverHandler {
            id: _hover
            onHoveredChanged: win.pointerMoved()
        }
    }
}
