pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Item {
    id: root
    readonly property rect dialogArea: mainOverlay.mainGeometry

    Connections {
        target: global.documentModule

        function onShowGotoDialog(documentUrl, line, character) {
            gotoDialog.documentUrl = documentUrl
            gotoDialog.line = line
            gotoDialog.character = character
            gotoDialog.open()
        }

        function onPrepareRenameDialog(documentUrl, line, character) {
            renameDialog.documentUrl = documentUrl
            renameDialog.line = line
            renameDialog.character = character
        }

        function onShowRenameDialog(documentUrl, oldName) {
            renameDialog.oldName = oldName
            renameDialog.open()
        }

        function onShowSaveDialog(documentUrl, documentName) {
            saveDialog.documentUrl = documentUrl
            saveDialog.documentName = documentName
            saveDialog.open()
        }
    }

    Dialog {
        id: gotoDialog
        parent: mainOverlay
        x: root.dialogArea.x + (root.dialogArea.width - width) / 2
        y: root.dialogArea.y + (root.dialogArea.height - height) / 2
        popupType: Popup.Item
        width: 600
        modal: true
        title: qsTr("Goto Line[:Character]")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property url documentUrl
        property int line
        property int character

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: {
            gotoField.text = (gotoDialog.line + 1) + ":" + gotoDialog.character
            gotoField.forceActiveFocus()
            gotoField.selectAll()
        }
        onAccepted: {
            const parts = gotoField.text.split(":")
            const line = parseInt(parts[0]) - 1
            const character = parts.length > 1 ? parseInt(parts[1]) : 0
            global.documentModule.indexSet(gotoDialog.documentUrl, line, character)
        }

        TextField {
            id: gotoField
            width: parent.width

            onAccepted: gotoDialog.accept()
            Keys.onEscapePressed: gotoDialog.reject()
        }
    }

    Dialog {
        id: renameDialog
        parent: mainOverlay
        x: root.dialogArea.x + (root.dialogArea.width - width) / 2
        y: root.dialogArea.y + (root.dialogArea.height - height) / 2
        popupType: Popup.Item
        width: 600
        modal: true
        title: qsTr("Rename Symbol")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property url documentUrl
        property int line
        property int character
        property string oldName

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: {
            renameField.text = renameDialog.oldName
            renameField.forceActiveFocus()
            renameField.selectAll()
        }
        onAccepted: {
            const newName = renameField.text
            if (newName !== "" && newName !== renameDialog.oldName) {
                global.documentModule.renameRequest(renameDialog.documentUrl, renameDialog.line, renameDialog.character, newName)
            }
        }

        TextField {
            id: renameField
            width: parent.width

            onAccepted: renameDialog.accept()
            Keys.onEscapePressed: renameDialog.reject()
        }
    }

    Dialog {
        id: saveDialog
        parent: mainOverlay
        x: root.dialogArea.x + (root.dialogArea.width - width) / 2
        y: root.dialogArea.y + (root.dialogArea.height - height) / 2
        popupType: Popup.Item
        width: 600
        modal: true
        title: qsTr("Save and Exit")
        standardButtons: Dialog.Yes | Dialog.No | Dialog.Cancel
        property url documentUrl
        property string documentName
        property int closeDecision: Dialog.Cancel

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
            closeDecision = Dialog.Cancel
        }
        onClosed: {
            mainOverlay.widgetCount -= 1
            if (closeDecision !== Dialog.Cancel)
                global.documentModule.documentClose(documentUrl, closeDecision === Dialog.Yes)
        }
        onAccepted: closeDecision = Dialog.Yes

        Label {
            text: qsTr("Do you want to save changes to " + saveDialog.documentName + "?")
        }

        Component.onCompleted: {
            saveDialog.standardButton(Dialog.No).clicked.connect(function () {
                saveDialog.closeDecision = Dialog.No
            })
        }
    }
}
