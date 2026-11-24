pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    parent: mainScreen
    anchors.centerIn: parent
    popupType: Popup.Item
    width: 600
    modal: true
    focus: true
    title: qsTr("File Property")
    standardButtons: Dialog.Ok | Dialog.Cancel
    property url documentUrl
    property var info: ({})

    function openFor(url) {
        documentUrl = url
        info = global.fileModule.fileInfo(url)
        nameField.text = info.baseName
        writableBox.checked = info.writable
        open()
    }

    Connections {
        target: global.fileModule

        function onShowPropertyDialog(documentUrl) {
            root.openFor(documentUrl)
            nameField.forceActiveFocus()
            nameField.cursorPosition = nameField.length
        }

        function onShowRenameDialog(documentUrl) {
            root.openFor(documentUrl)
            nameField.forceActiveFocus()
            nameField.selectAll()
        }

        function onShowPermissionDialog(documentUrl) {
            root.openFor(documentUrl)
            writableBox.forceActiveFocus()
        }
    }

    onOpened: {
        global.mainWindow.overlayFlagSet(false, true)
        mainOverlay.widgetCount += 1
    }
    onClosed: mainOverlay.widgetCount -= 1
    onAccepted: {
        let url = root.documentUrl
        if (nameField.text !== root.info.baseName) {
            const suffix = root.info.directory || !root.info.suffix ? "" : "." + root.info.suffix
            const targetUrl = root.info.parentUrl + "/" + encodeURIComponent(nameField.text + suffix)
            if (global.fileModule.fileRename(url, targetUrl)) url = targetUrl
        }
        if (writableBox.checked !== root.info.writable)
            global.fileModule.fileWritable(url, writableBox.checked)
    }

    ColumnLayout {
        width: parent.width

        RowLayout {
            Layout.preferredHeight: 64

            Image {
                source: root.info.source ?? ""
                sourceSize.width: 48
                sourceSize.height: 48
                horizontalAlignment: Image.AlignLeft
                fillMode: Image.PreserveAspectFit
                Layout.preferredWidth: 150
            }

            TextField {
                id: nameField
                placeholderText: qsTr("Enter new name:")
                Layout.fillWidth: true

                onAccepted: root.accept()
                Keys.onEscapePressed: root.reject()
            }
        }

        Rectangle {
            color: global.theme.general.stroke
            Layout.fillWidth: true; Layout.preferredHeight: 1
        }

        RowLayout {
            Layout.preferredHeight: 32

            Label {
                text: qsTr("Absolute Path:")
                Layout.preferredWidth: 150
            }

            Label {
                text: root.info.absolutePath ?? ""
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.preferredHeight: 32

            Label {
                text: qsTr("Size:")
                Layout.preferredWidth: 150
            }

            Label {
                text: root.info.size ?? ""
            }
        }

        Rectangle {
            color: global.theme.general.stroke
            Layout.fillWidth: true; Layout.preferredHeight: 1
        }

        RowLayout {
            Layout.preferredHeight: 32

            Label {
                text: qsTr("Birth Time:")
                Layout.preferredWidth: 150
            }

            Label {
                text: root.info.birthTime ?? ""
            }
        }

        RowLayout {
            Layout.preferredHeight: 32

            Label {
                text: qsTr("Last Modified:")
                Layout.preferredWidth: 150
            }

            Label {
                text: root.info.lastModified ?? ""
            }
        }

        RowLayout {
            Layout.preferredHeight: 32

            Label {
                text: qsTr("Last Read:")
                Layout.preferredWidth: 150
            }

            Label {
                text: root.info.lastRead ?? ""
            }
        }

        Rectangle {
            color: global.theme.general.stroke
            Layout.fillWidth: true; Layout.preferredHeight: 1
        }

        RowLayout {
            Layout.preferredHeight: 32

            Label {
                text: qsTr("Attributes:")
                Layout.preferredWidth: 150
            }

            CheckBox {
                checked: root.info.readable ?? false
                text: qsTr("Readable")
            }

            CheckBox {
                id: writableBox
                text: qsTr("Writable")

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -3
                    z: -1
                    visible: parent.activeFocus
                    color: "transparent"
                    border.width: 2
                    border.color: global.theme.general.warningBack3
                    radius: 7
                }
            }

            CheckBox {
                checked: root.info.hidden ?? false
                text: qsTr("Hidden")
            }
        }
    }
}
