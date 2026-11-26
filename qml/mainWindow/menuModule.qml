pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Layouts

import UniComm

Item {
    id: root
    required property MenuModule menuModule
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    RowLayout {
        anchors.fill: parent

        Button {
            id: fileMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "File"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = fileMenuButton.mapToGlobal(0, fileMenuButton.height)
                const localPos = mainOverlay.mapFromGlobal(globalPos)
                fileMenu.popup(localPos.x, localPos.y)
            }

            Shortcut {
                sequence: "Ctrl+O"
                onActivated: openWorkspaceItem.triggered()
            }

            Shortcut {
                sequence: "Ctrl+S"
                onActivated: saveWorkspaceItem.triggered()
            }

            Shortcut {
                sequence: "Ctrl+Shift+S"
                onActivated: saveWorkspaceAsItem.triggered()
            }
        }

        Button {
            id: editMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Edit"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = editMenuButton.mapToGlobal(0, editMenuButton.height)
                const localPos = mainOverlay.mapFromGlobal(globalPos)
                editMenu.popup(localPos.x, localPos.y)
            }
        }
        
        Button {
            id: viewMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "View"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = viewMenuButton.mapToGlobal(0, viewMenuButton.height)
                const localPos = mainOverlay.mapFromGlobal(globalPos)
                viewMenu.popup(localPos.x, localPos.y)
            }
        }

        Button {
            id: navMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Nav"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = navMenuButton.mapToGlobal(0, navMenuButton.height)
                const localPos = mainOverlay.mapFromGlobal(globalPos)
                navMenu.popup(localPos.x, localPos.y)
            }

            Shortcut {
                sequence: "Alt+Left"
                onActivated: navPrevItem.triggered()
            }

            Shortcut {
                sequence: "Alt+Right"
                onActivated: navNextItem.triggered()
            }
        }

        Button {
            id: codeMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Code"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = codeMenuButton.mapToGlobal(0, codeMenuButton.height)
                const localPos = mainOverlay.mapFromGlobal(globalPos)
                codeMenu.popup(localPos.x, localPos.y)
            }

            Shortcut {
                sequence: "Ctrl+Space"
                onActivated: {
                    codeMenu.menuSession = global.documentModule.menuLoad("code")
                    codeCompletionItem.triggered()
                }
            }

            Shortcut {
                sequence: "Ctrl+Alt+L"
                onActivated: {
                    codeMenu.menuSession = global.documentModule.menuLoad("code")
                    codeReformatItem.triggered()
                }
            }
        }

        Button {
            id: execMenuButton
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Exec"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = execMenuButton.mapToGlobal(0, execMenuButton.height)
                const localPos = mainOverlay.mapFromGlobal(globalPos)
                execMenu.popup(localPos.x, localPos.y)
            }

            Shortcut {
                sequence: "Shift+F10"
                onActivated: {
                    execMenu.menuSession = global.documentModule.menuLoad("exec")
                    execRunItem.triggered()
                }
            }

            Shortcut {
                sequence: "Shift+F9"
                onActivated: {
                    execMenu.menuSession = global.documentModule.menuLoad("exec")
                    execDebugItem.triggered()
                }
            }
        }

        Button {
            id: gitMenuButton
            enabled: global.serviceModule.git.hasRepo
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            text: "Git"
            Layout.preferredWidth: 48; Layout.preferredHeight: 24

            onClicked: {
                const globalPos = gitMenuButton.mapToGlobal(0, gitMenuButton.height)
                const localPos = mainOverlay.mapFromGlobal(globalPos)
                gitMenu.popup(localPos.x, localPos.y)
            }
        }

        Item {
            Layout.fillWidth: true; Layout.preferredHeight: 24
        }

        Button {
            id: themeButton
            Layout.preferredWidth: 24; Layout.preferredHeight: 24
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            checkable: true
            icon.source: checked ? "qrc:/icon/checkmark.svg" :
                global.theme.mode === 0 ? "qrc:/icon/themeDark.svg" : "qrc:/icon/themeLight.svg"
            icon.width: 16; icon.height: 16

            onToggled: {
                if (!checked) {
                    global.mainWindow.themeSet(global.theme.mode === 0 ? 1 : 0)
                }
            }

            Timer {
                interval: 1000
                running: parent.checked
                onTriggered: parent.checked = false
            }
        }
    }

    Menu {
        id: fileMenu
        parent: mainOverlay
        popupType: Popup.Item
        implicitWidth: 300

        onOpened: {
            global.mainWindow.overlayFlagSet(false, false)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            id: openFileItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/document.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.fore
                }

                Label {
                    text: qsTr("Open File")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: ""
                }
            }

            onTriggered: openFileItemDialog.open()

            FileDialog {
                id: openFileItemDialog
                fileMode: FileDialog.OpenFile
                nameFilters: ["All files (*)"]
                onAccepted: global.documentModule.documentOpen(selectedFile)
            }
        }

        MenuItem {
            id: openWorkspaceItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/home.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.fore
                }

                Label {
                    text: qsTr("Open Workspace")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+O"
                }
            }

            onTriggered: Qt.callLater(() => global.mainWindow.workspaceOpen())
        }

        MenuItem {
            id: saveWorkspaceItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/save.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.fore
                }

                Label {
                    text: qsTr("Save Workspace")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+S"
                }
            }

            onTriggered: global.mainWindow.workspaceSave("")
        }

        MenuItem {
            id: saveWorkspaceAsItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/saveAs.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.fore
                }

                Label {
                    text: qsTr("Save Workspace")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+Shift+S"
                }
            }

            onTriggered: saveDialog.open()

            FileDialog {
                id: saveDialog
                currentFolder: StandardPaths.standardLocations(StandardPaths.DesktopLocation)[0]
                fileMode: FileDialog.SaveFile
                nameFilters: ["Json (*.json)"]
                currentFile: currentFolder + "/config.json"
                onAccepted: global.mainWindow.workspaceSave(selectedFile)
            }
        }
    }

    Menu {
        id: editMenu
        parent: mainOverlay
        popupType: Popup.Item
        implicitWidth: 300
        property var menuSession
        readonly property int historyMenuMaximumHeight: 400

        onOpened: {
            global.mainWindow.overlayFlagSet(false, false)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: editMenu.menuSession = global.documentModule.menuLoad("edit")

        Menu {
            id: editUndoMenu
            title: qsTr("Undo")
            enabled: global.undoModule.canUndo
            implicitWidth: 300
            implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                                     editMenu.historyMenuMaximumHeight)
            icon.source: "qrc:/icon/undo.svg"
            icon.width: 16; icon.height: 16

            onAboutToShow: editUndoInstantiator.model = global.undoModule.undoHistory()

            Instantiator {
                id: editUndoInstantiator
                model: []
                delegate: MenuItem {
                    required property var modelData
                    text: qsTr("%1. %2").arg(modelData.steps).arg(modelData.text)
                    onTriggered: global.undoModule.undoTo(modelData.targetIndex)
                }

                onObjectAdded: (index, object) => editUndoMenu.addItem(object)
                onObjectRemoved: (index, object) => editUndoMenu.removeItem(object)
            }
        }

        Menu {
            id: editRedoMenu
            title: qsTr("Redo")
            enabled: global.undoModule.canRedo
            implicitWidth: 300
            implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                                     editMenu.historyMenuMaximumHeight)
            icon.source: "qrc:/icon/redo.svg"
            icon.width: 16; icon.height: 16

            onAboutToShow: editRedoInstantiator.model = global.undoModule.redoHistory()

            Instantiator {
                id: editRedoInstantiator
                model: []
                delegate: MenuItem {
                    required property var modelData
                    text: qsTr("%1. %2").arg(modelData.steps).arg(modelData.text)
                    onTriggered: global.undoModule.redoTo(modelData.targetIndex)
                }

                onObjectAdded: (index, object) => editRedoMenu.addItem(object)
                onObjectRemoved: (index, object) => editRedoMenu.removeItem(object)
            }
        }

        MenuSeparator {
        }

        MenuItem {
            id: editCutItem
            enabled: editMenu.menuSession ? editMenu.menuSession.copiable : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/cut.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: editCutItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
                }

                Label {
                    text: qsTr("Cut")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+X"
                }
            }

            onTriggered: global.documentModule.menuCall("cut")
        }

        MenuItem {
            id: editCopyItem
            enabled: editMenu.menuSession ? editMenu.menuSession.copiable : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/copy.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: editCopyItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
                }

                Label {
                    text: qsTr("Copy")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+C"
                }
            }

            onTriggered: global.documentModule.menuCall("copy")
        }

        MenuItem {
            id: editPasteItem
            enabled: editMenu.menuSession ? editMenu.menuSession.pastable : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/paste.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: editPasteItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
                }

                Label {
                    text: qsTr("Paste")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+V"
                }
            }

            onTriggered: global.documentModule.menuCall("paste")
        }

        MenuSeparator {
        }

        MenuItem {
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/search.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.fore
                }

                Label {
                    text: qsTr("Search")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+F"
                }
            }

            onTriggered: global.documentModule.menuCall("search")
        }

        MenuItem {
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/replace.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.fore
                }

                Label {
                    text: qsTr("Replace")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+R"
                }
            }

            onTriggered: global.documentModule.menuCall("replace")
        }
    }

    Menu {
        id: viewMenu
        parent: mainOverlay
        popupType: Popup.Item

        onOpened: {
            global.mainWindow.overlayFlagSet(false, false)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.explorerModule)
            text: qsTr("Explorer")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.portModule)
            text: qsTr("Port")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.threadpoolModule)
            text: qsTr("Threadpool")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuSeparator {
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.diagnosticsModule)
            text: qsTr("Diagnostics")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.structureModule)
            text: qsTr("Structure")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuSeparator {
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.breakpointModule)
            text: qsTr("Breakpoint")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.debugModule)
            text: qsTr("Debug")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.watchModule)
            text: qsTr("Watch")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuSeparator {
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.logModule)
            text: qsTr("Log")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.gitModule)
            text: qsTr("Git")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.serviceModule)
            text: qsTr("Services")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }

        Menu {
            id: terminalMenu
            title: qsTr("Terminal")

            onOpened: {
                global.mainWindow.overlayFlagSet(false, true)
                mainOverlay.widgetCount += 1
            }
            onClosed: mainOverlay.widgetCount -= 1

            MenuItem {
                text: qsTr("Manage")

                onTriggered: global.terminalModule.terminalManage()
            }

            MenuSeparator {
            }

            Instantiator {
                id: terminalInstantiator
                model: global.terminalModule.terminals
                delegate: MenuItem {
                    required property var modelData
                    text: modelData.name
                    onTriggered: global.terminalModule.terminalOpen(modelData.id)
                }

                onObjectAdded: (index, object) => terminalMenu.insertItem(index + 2, object)
                onObjectRemoved: (index, object) => terminalMenu.removeItem(object)
            }
        }

        MenuSeparator {
        }

        MenuItem {
            readonly property var dockAction: root.menuModule.toggleActionGet(global.agentModule)
            text: qsTr("Agent")
            checkable: true
            checked: dockAction.checked

            onTriggered: dockAction.toggle()
        }
    }

    Menu {
        id: navMenu
        parent: mainOverlay
        popupType: Popup.Item
        implicitWidth: 300
        property var menuSession

        onOpened: {
            global.mainWindow.overlayFlagSet(false, false)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: navMenu.menuSession = global.documentModule.menuLoad("nav")

        MenuItem {
            id: navPrevItem
            enabled: navMenu.menuSession ? navMenu.menuSession.prev : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/arrowLeft.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: navPrevItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
                }

                Label {
                    text: qsTr("Prev")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Alt+Left"
                }
            }

            onTriggered: global.documentModule.navigationPrev()
        }

        MenuItem {
            id: navNextItem
            enabled: navMenu.menuSession ? navMenu.menuSession.next : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/arrowRight.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: navNextItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
                }

                Label {
                    text: qsTr("Next")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Alt+Right"
                }
            }

            onTriggered: global.documentModule.navigationNext()
        }

        MenuSeparator {
        }

        MenuItem {
            enabled: navMenu.menuSession ? navMenu.menuSession.navigation : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                Item {
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16;
                }

                Label {
                    text: qsTr("Definition(s)")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: global.documentModule.definitionRequest(navMenu.menuSession.documentUrl, navMenu.menuSession.line, navMenu.menuSession.character)
        }

        MenuItem {
            enabled: navMenu.menuSession ? navMenu.menuSession.navigation : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                Item {
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16;
                }

                Label {
                    text: qsTr("Reference(s)")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: global.documentModule.referencesRequest(navMenu.menuSession.documentUrl, navMenu.menuSession.line, navMenu.menuSession.character)
        }

        MenuItem {
            enabled: navMenu.menuSession ? navMenu.menuSession.navigation : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                Item {
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16;
                }

                Label {
                    text: qsTr("Implementation(s)")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: global.documentModule.implementationRequest(navMenu.menuSession.documentUrl, navMenu.menuSession.line, navMenu.menuSession.character)
        }

        MenuItem {
            enabled: navMenu.menuSession ? navMenu.menuSession.navigation : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                Item {
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16;
                }

                Label {
                    text: qsTr("Type Definition(s)")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: global.documentModule.typeDefinitionRequest(navMenu.menuSession.documentUrl, navMenu.menuSession.line, navMenu.menuSession.character)
        }
    }

    Menu {
        id: codeMenu
        parent: mainOverlay
        popupType: Popup.Item
        implicitWidth: 300
        property var menuSession

        onOpened: {
            global.mainWindow.overlayFlagSet(false, false)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: codeMenu.menuSession = global.documentModule.menuLoad("code")

        MenuItem {
            id: codeCompletionItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/completion.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.theme.general.fore
                }

                Label {
                    text: qsTr("Completion")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+Space"
                }
            }

            onTriggered: global.documentModule.completionRequest(codeMenu.menuSession.documentUrl, codeMenu.menuSession.line, codeMenu.menuSession.character)
        }

        MenuItem {
            id: codeReformatItem

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
                    text: codeMenu.menuSession ? codeMenu.menuSession.text ? qsTr("Reformat Selected") : qsTr("Reformat") : false
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+Alt+L"
                }
            }

            onTriggered: {
                if (codeMenu.menuSession.text) {
                    global.documentModule.rangeFormattingRequest(codeMenu.menuSession.documentUrl, codeMenu.menuSession.startLine, codeMenu.menuSession.startCharacter, codeMenu.menuSession.endLine, codeMenu.menuSession.endCharacter)
                } else {
                    global.documentModule.formattingRequest(codeMenu.menuSession.documentUrl)
                }
            }
        }

        Menu {
            title: qsTr("Folding")
            icon.source: "qrc:/icon/fold.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Contract Top")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.documentModule.foldContractTop(codeMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Contract Recursively")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.documentModule.foldContractRecursively(codeMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Expand Recursively")
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.documentModule.foldExpandRecursively(codeMenu.menuSession.documentUrl)
            }
        }
    }

    Menu {
        id: execMenu
        parent: mainOverlay
        popupType: Popup.Item
        implicitWidth: 300
        property var menuSession

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAboutToShow: execMenu.menuSession = global.documentModule.menuLoad("exec")

        MenuItem {
            id: execRunItem

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
                    text: qsTr("Run ") + (execMenu.menuSession ? execMenu.menuSession.documentName : "")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Shift+F10"
                }
            }

            onTriggered: global.threadpoolModule.threadStart(execMenu.menuSession.documentUrl, RunMode.Run)
        }

        MenuItem {
            id: execDebugItem

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
                    text: qsTr("Debug ") + (execMenu.menuSession ? execMenu.menuSession.documentName : "")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Shift+F9"
                }
            }

            onTriggered: global.threadpoolModule.threadStart(execMenu.menuSession.documentUrl, RunMode.Debug)
        }
    }

    Menu {
        id: gitMenu
        parent: mainOverlay
        popupType: Popup.Item

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            id: fetchItem
            enabled: global.gitModule.gitStatus === GitOperationState.Idle
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/gitFetch.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: fetchItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
                }

                Label {
                    text: qsTr("Fetch")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: global.gitModule.fetch()
        }

        MenuItem {
            id: commitItem
            enabled: global.gitModule.gitStatus === GitOperationState.Idle
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/gitCommit.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: commitItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
                }

                Label {
                    text: qsTr("Commit")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: global.gitModule.gitCommitPre()
        }

        MenuItem {
            id: pushItem
            enabled: global.gitModule.gitStatus === GitOperationState.Idle
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/gitPush.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: pushItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
                }

                Label {
                    text: qsTr("Push")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: global.gitModule.gitPushPre()
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Config")
            icon.source: "qrc:/icon/settings.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Proxy")
                enabled: false
            }
        }
    }
}
