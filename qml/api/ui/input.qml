import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: rootItem
    width: 480; height: 220

    required property string text
    property alias value: inputField.text
    property bool accepted: false

    Window.onActiveChanged: {
        if (!rootItem.Window.active) return
        inputField.forceActiveFocus()
        inputField.selectAll()
    }

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        ScrollView {
            id: inputScrollView
            Layout.fillWidth: true; Layout.fillHeight: true
            contentWidth: availableWidth

            Label {
                width: inputScrollView.availableWidth
                text: rootItem.text
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
        }

        TextField {
            id: inputField
            Layout.fillWidth: true
            focus: true
            selectByMouse: true

            onAccepted: buttonBox.accepted()
        }

        DialogButtonBox {
            id: buttonBox
            Layout.alignment: Qt.AlignRight
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
            padding: 0
            background: null

            onAccepted: {
                rootItem.accepted = true
                rootItem.Window.window.close()
            }
            onRejected: rootItem.Window.window.close()
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: buttonBox.rejected()
    }
}
