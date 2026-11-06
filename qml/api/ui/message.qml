import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: rootItem
    width: 480; height: 220
    padding: 20

    required property string text
    required property list<string> button
    property bool accepted: false

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        ScrollView {
            id: messageScrollView
            Layout.fillWidth: true; Layout.fillHeight: true
            contentWidth: availableWidth

            Label {
                width: messageScrollView.availableWidth
                text: rootItem.text
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
        }

        DialogButtonBox {
            id: buttonBox
            Layout.alignment: Qt.AlignRight
            standardButtons: (rootItem.button.indexOf("ok") !== -1 ? DialogButtonBox.Ok : DialogButtonBox.NoButton)
                             | (rootItem.button.indexOf("cancel") !== -1 ? DialogButtonBox.Cancel : DialogButtonBox.NoButton)
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
        sequences: ["Return", "Enter"]
        enabled: rootItem.button.indexOf("ok") !== -1
        onActivated: buttonBox.accepted()
    }

    Shortcut {
        sequence: "Escape"
        onActivated: buttonBox.rejected()
    }
}
