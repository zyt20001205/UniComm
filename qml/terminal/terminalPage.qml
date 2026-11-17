import QtQuick
import QtQuick.Controls

Item {
    id: rootItem
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    Item {
        id: terminalItem
        anchors.fill: parent

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
            size: vtermWidget.scrollSize
            position: vtermWidget.scrollPosition

            onPositionChanged: {
                if (pressed) vtermWidget.scrollPosition = position
            }
        }
    }

    Component.onCompleted: {
        const objects = {
            "terminalItem": terminalItem,
            "damageOverlay": damageOverlay
        };
        terminalPage.propertyGet(objects)
    }
}
