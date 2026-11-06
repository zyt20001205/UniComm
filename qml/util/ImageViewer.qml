import QtQuick
import QtQuick.Controls

Item {
    id: rootItem
    focus: true

    property url source

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    Item {
        id: viewport
        anchors.fill: parent
        clip: true

        function center() {
            imageContainer.x = (width - imageContainer.width) / 2
            imageContainer.y = (height - imageContainer.height) / 2
        }

        Item {
            id: imageContainer
            x: 0; y: 0
            width: image.implicitWidth; height: image.implicitHeight
            scale: 1.0
            transformOrigin: Item.TopLeft

            Image {
                id: image
                anchors.fill: parent
                source: rootItem.source
                fillMode: Image.PreserveAspectFit
            }
        }

        DragHandler {
            id: dragHandler
            target: null
            acceptedButtons: Qt.LeftButton
            property real lastX: 0
            property real lastY: 0

            onActiveChanged: {
                lastX = 0
                lastY = 0
            }

            onTranslationChanged: {
                imageContainer.x += translation.x - lastX
                imageContainer.y += translation.y - lastY
                lastX = translation.x
                lastY = translation.y
            }
        }

        HoverHandler {
            cursorShape: dragHandler.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton

            onTapped: rootItem.forceActiveFocus()
        }

        TapHandler {
            acceptedButtons: Qt.MiddleButton

            onTapped: {
                imageContainer.scale = 1.0
                viewport.center()
            }
        }

        WheelHandler {
            id: wheelHandler

            onWheel: function (event) {
                if (image.implicitWidth <= 0 || image.implicitHeight <= 0) return
                const point = wheelHandler.point.position
                const imageX = (point.x - imageContainer.x) / imageContainer.scale
                const imageY = (point.y - imageContainer.y) / imageContainer.scale
                imageContainer.scale *= event.angleDelta.y > 0 ? 1.2 : 0.8
                imageContainer.x = point.x - imageX * imageContainer.scale
                imageContainer.y = point.y - imageY * imageContainer.scale
                event.accepted = true
            }
        }
    }

    DoubleSpinBox {
        width: 120; height: 36
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
        from: 0.01
        to: 100
        value: imageContainer.scale
        stepSize: 0.1
        editable: true

        textFromValue: function(value, locale) {
            return "x" + value.toFixed(2)
        }

        valueFromText: function(text, locale) {
            return Number(text.replace("x", ""))
        }

        onValueModified: imageContainer.scale = value
    }
}
