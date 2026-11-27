pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import UniComm

Item {
    Connections {
        target: global.documentModule

        function onShowContextMenu() { contextMenu.popup() }

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
        parent: mainScreen
        anchors.centerIn: parent
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
        parent: mainScreen
        anchors.centerIn: parent
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
        parent: mainScreen
        anchors.centerIn: parent
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

    Menu {
        id: contextMenu
        parent: mainOverlay
        popupType: Popup.Item
        focus: false
        implicitWidth: 300
        property var menuSession

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: menuSession = global.documentModule.menuLoad("editor")

        MenuItem {
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/play.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.successFore3
                }

                Label {
                    text: qsTr("Run")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Shift+F10"
                }
            }

            onTriggered: global.threadpoolModule.threadStart(contextMenu.menuSession.documentUrl, RunMode.Run)
        }

        MenuItem {
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/bug.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.successFore3
                }

                Label {
                    text: qsTr("Debug")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Shift+F9"
                }
            }

            onTriggered: global.threadpoolModule.threadStart(contextMenu.menuSession.documentUrl, RunMode.Debug)
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Folding")
            icon.source: "qrc:/icon/fold.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Contract Top")
                icon.source: "qrc:/icon/chevronDownUp.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.documentModule.foldContractTop(contextMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Contract Recursively")
                icon.source: "qrc:/icon/chevronDownUp.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.documentModule.foldContractRecursively(contextMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Expand Recursively")
                icon.source: "qrc:/icon/chevronUpDown.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.documentModule.foldExpandRecursively(contextMenu.menuSession.documentUrl)
            }
        }

        MenuItem {
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/brush.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.fore
                }

                Label {
                    text: contextMenu.menuSession ? contextMenu.menuSession.text ? qsTr("Reformat Selected") : qsTr("Reformat") : false
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+Alt+L"
                }
            }

            onTriggered: {
                if (contextMenu.menuSession.text) {
                    global.documentModule.rangeFormattingRequest(contextMenu.menuSession.documentUrl, contextMenu.menuSession.startLine, contextMenu.menuSession.startCharacter, contextMenu.menuSession.endLine, contextMenu.menuSession.endCharacter)
                } else {
                    global.documentModule.formattingRequest(contextMenu.menuSession.documentUrl)
                }
            }
        }

        MenuItem {
            text: qsTr("Rename")
            icon.source: "qrc:/icon/rename.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.documentModule.prepareRenameRequest(contextMenu.menuSession.documentUrl, contextMenu.menuSession.line, contextMenu.menuSession.character)
        }

        Menu {
            title: qsTr("Navigation")
            icon.source: "qrc:/icon/location.svg"
            icon.width: 16; icon.height: 16
            enabled: contextMenu.menuSession ? contextMenu.menuSession.navigation : false

            MenuItem {
                text: qsTr("Definition(s)")

                onTriggered: global.documentModule.definitionRequest(contextMenu.menuSession.documentUrl, contextMenu.menuSession.line, contextMenu.menuSession.character)
            }

            MenuItem {
                text: qsTr("References(s)")

                onTriggered: global.documentModule.referencesRequest(contextMenu.menuSession.documentUrl, contextMenu.menuSession.line, contextMenu.menuSession.character)
            }

            MenuItem {
                text: qsTr("Implementation(s)")

                onTriggered: global.documentModule.implementationRequest(contextMenu.menuSession.documentUrl, contextMenu.menuSession.line, contextMenu.menuSession.character)
            }

            MenuItem {
                text: qsTr("Type Definition(s)")

                onTriggered: global.documentModule.typeDefinitionRequest(contextMenu.menuSession.documentUrl, contextMenu.menuSession.line, contextMenu.menuSession.character)
            }
        }

        MenuSeparator {
        }

        MenuItem {
            id: addWatchItem
            text: qsTr("Add Watch")
            icon.source: "qrc:/icon/eye.svg"
            icon.width: 16; icon.height: 16
            enabled: contextMenu.menuSession ? contextMenu.menuSession.text : false

            HoverHandler {
                onHoveredChanged: {
                    if (!hovered) {
                        mainTooltip.text = ""
                    }
                }
                onPointChanged: {
                    if (!addWatchItem.enabled) {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Nothing selected")
                    }
                }
            }

            onTriggered: global.watchModule.addWatch(contextMenu.menuSession.documentUrl, contextMenu.menuSession.text)
        }

        MenuItem {
            id: runHereItem
            text: qsTr("Run Here")
            icon.source: "qrc:/icon/debugContinue.svg"
            icon.width: 16; icon.height: 16
            enabled: global.debugModule.currentThread.length > 0 && global.documentModule.selection.line !== undefined

            HoverHandler {
                onHoveredChanged: {
                    if (!hovered) {
                        mainTooltip.text = ""
                    }
                }
                onPointChanged: {
                    if (global.debugModule.currentThread.length === 0) {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("No debug sessions")
                    }
                }
            }

            onTriggered: global.threadpoolModule.interpreterStateSet(
                global.debugModule.currentThread, InterpreterState.RunToCursor,
                global.documentModule.focusedUrl, global.documentModule.selection.line)
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Open In")
            icon.source: "qrc:/icon/open.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Explorer")
                icon.source: "qrc:/icon/folder.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.fileModule.fileOpenInExplorer(contextMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Application")
                icon.source: "qrc:/icon/apps.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.fileModule.fileOpenInApplication(contextMenu.menuSession.documentUrl)
            }
        }

        MenuItem {
            text: qsTr("Property")
            icon.source: "qrc:/icon/property.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.showPropertyDialog(contextMenu.menuSession.documentUrl)
        }
    }
}
