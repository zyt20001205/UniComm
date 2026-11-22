pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

ToolTip {
    id: root
    parent: mainOverlay
    popupType: Popup.Item
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
    font: signatureWidget.font

    required property QtObject signatureWidget
    property point position

    Connections {
        target: root.signatureWidget

        function onShowSignature(position) {
            root.position = position
            root.open()
        }

        function onHideSignature() { root.close() }
    }

    onVisibleChanged: signatureWidget.visible = visible
    onAboutToShow: {
        const localPosition = mainOverlay.mapFromGlobal(position)
        x = localPosition.x - 6
        y = localPosition.y - implicitHeight
    }
    onOpened: {
        global.mainWindow.overlayFlagSet(false, false)
        mainOverlay.widgetCount += 1
    }
    onClosed: mainOverlay.widgetCount -= 1

    contentItem: Label {
        text: root.signatureWidget.text
        font: root.font
        textFormat: Text.RichText
    }
}
