pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import UniComm

ToolTip {
    id: root
    parent: mainOverlay
    popupType: Popup.Item
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
    required property QtObject positionWidget

    Connections {
        target: root.positionWidget

        function onSetPosition(position, text) {
            const localPosition = mainOverlay.mapFromGlobal(position)
            root.x = localPosition.x + 10
            root.y = localPosition.y + 10
            root.text = text
        }

        function onShowPosition() { root.open() }
        function onHidePosition() { root.close() }
    }

    onVisibleChanged: positionWidget.visible = visible

    Behavior on x {
        enabled: root.visible
        SmoothedAnimation { duration: AnimationDuration.Follow; velocity: -1 }
    }
    Behavior on y {
        enabled: root.visible
        SmoothedAnimation { duration: AnimationDuration.Follow; velocity: -1 }
    }
}
