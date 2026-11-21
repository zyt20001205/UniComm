pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import UniComm

Item {
    id: root
    required property TerminalPage terminalPage
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    TerminalWidget {
        id: terminalWidget
        anchors.fill: parent

        Component.onCompleted: root.terminalPage.terminalWidgetSet(terminalWidget)
        onDebugDamage: rect => {
            damageOverlay.x = rect.x
            damageOverlay.y = rect.y
            damageOverlay.width = rect.width
            damageOverlay.height = rect.height
            damageOverlay.visible = true
        }

        Rectangle {
            id: damageOverlay
            visible: false
            color: "transparent"
            border.color: "#ff00ff"
            border.width: 1
            z: 100
        }

        ScrollBar {
            minimumSize: Math.min(1, 20 / Math.max(1, height))
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            z: 10

            policy: ScrollBar.AsNeeded
            palette {
                mid: global.theme.general.stroke
                dark: global.theme.general.strokePressed
            }
            size: root.terminalPage.vtermWidget.scrollSize
            position: root.terminalPage.vtermWidget.scrollPosition

            onPositionChanged: {
                if (pressed) root.terminalPage.vtermWidget.scrollPosition = position
            }
        }
    }
}
