import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.FluentWinUI3.impl
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQml

Item {
    id: mainItem
    anchors.fill: parent

    // overlay control
    property int widgetCount: 0
    property rect mainGeometry: Qt.rect(0, 0, width, height)

    Item {
        id: mainScreenItem
        x: mainGeometry.x
        y: mainGeometry.y
        width: mainGeometry.width
        height: mainGeometry.height

        Item {
            id: debugConsole
            anchors.fill: parent
            // visible: true
            visible: false

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: global.dangerFore3
                border.width: 3
            }

            ColumnLayout {
                anchors.centerIn: parent

                Label {
                    text: Application.styleHints.colorScheme === Qt.Light ? "Light" : "Dark"
                    color: global.dangerFore3
                    font.pointSize: 16
                }

                Label {
                    id: debugWidgetCount
                    color: global.dangerFore3
                    font.pointSize: 16
                }
            }
        }
    }

    onWidgetCountChanged: {
        if (widgetCount < 0) {
            console.log("severe error occurred!!! widget count: " + widgetCount)
            widgetCount = 0
        }
        if (widgetCount === 0) {
            mainWindow.overlayFlagSet(true, false)
        }

        if (debugConsole.visible) {
            debugWidgetCount.text = "widget count: " + widgetCount
        }
    }

    // main window
    Dialog {
        id: mainWindowCloseDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Save and Exit?")
        standardButtons: Dialog.Yes | Dialog.No

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAccepted: mainWindow.quit()
    }

    Dialog {
        id: mainWindowMessageDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        standardButtons: Dialog.Ok
        property string text
        topPadding: 30; bottomPadding: 20

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        Label {
            text: mainWindowMessageDialog.text
        }
    }

    Dialog {
        id: mainWindowQuitDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Exiting application...")
        standardButtons: Dialog.Abort
        topPadding: 30; bottomPadding: 20
        property real primaryProgress
        property string primaryLog
        property real secondaryProgress
        property string secondaryLog

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onRejected: mainWindow.terminate()

        ColumnLayout {
            width: parent.width

            ProgressBar {
                value: mainWindowQuitDialog.primaryProgress
                Layout.fillWidth: true
            }

            Label {
                text: mainWindowQuitDialog.primaryLog
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }

            Item {
                Layout.fillWidth: true; Layout.preferredHeight: 10
            }

            ProgressBar {
                value: mainWindowQuitDialog.secondaryProgress
                Layout.fillWidth: true
            }

            Label {
                text: mainWindowQuitDialog.secondaryLog
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }
        }
    }

    Component {
        id: mainWindowMessageComponent

        Dialog {
            id: mainWindowMessageDialog
            parent: Overlay.overlay
            x: mainScreenItem.x + (mainScreenItem.width - width) / 2
            y: mainScreenItem.y + (mainScreenItem.height - height) / 2
            width: 600
            modal: true
            standardButtons: Dialog.Ok
            property string text
            topPadding: 30; bottomPadding: 20

            onOpened: {
                mainWindow.overlayFlagSet(false, true)
                widgetCount += 1
            }
            onClosed: {
                widgetCount -= 1
                destroy()
            }

            Label {
                text: mainWindowMessageDialog.text
            }
        }
    }

    function messageDialogNew(eventloop, title, text) {
        const messageDialog = mainWindowMessageComponent.createObject(mainItem, {
            "title": title,
            "text": text
        });
        messageDialog.open()
        messageDialog.closed.connect(() => {
            eventloop.quit()
        });
    }

    function operationErrorShow(error) {
        if (!error)
            return
        mainWindowMessageDialog.title = qsTr("Error")
        mainWindowMessageDialog.text = error
        mainWindowMessageDialog.open()
    }

    Menu {
        id: mainWindowLinkMenu
        property url url

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Copy")
            icon.source: "qrc:/icon/copy.svg"
            icon.width: 16; icon.height: 16

            onTriggered: fileModule.copyToClipboard(String(mainWindowLinkMenu.url))
        }

        MenuItem {
            text: qsTr("Open")
            icon.source: "qrc:/icon/link.svg"
            icon.width: 16; icon.height: 16

            onTriggered: documentModule.documentOpen(mainWindowLinkMenu.url)
        }

        MenuItem {
            text: qsTr("Open Externally")
            icon.source: "qrc:/icon/open.svg"
            icon.width: 16; icon.height: 16

            onTriggered: Qt.openUrlExternally(mainWindowLinkMenu.url)
        }
    }

    ToolTip {
        id: mainWindowTextView
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        property point position
        property string data

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: {
            widgetCount -= 1
            mainWindowTextView.data = ""
        }
        onAboutToShow: {
            position = Overlay.overlay.mapFromGlobal(position)
            x = position.x + 10
            y = position.y + 10
        }

        contentItem: ColumnLayout {
            RowLayout {
                id: mainWindowTextViewLayout

                Item {
                    Layout.fillWidth: true; Layout.preferredHeight: 12

                    MouseArea {
                        anchors.fill: parent
                        preventStealing: true
                        property real lastX
                        property real lastY

                        onPressed: (mouse) => {
                            lastX = mouse.x
                            lastY = mouse.y
                        }

                        onPositionChanged: (mouse) => {
                            mainWindowTextView.x += mouse.x - lastX
                            mainWindowTextView.y += mouse.y - lastY
                        }
                    }
                }
            }

            ScrollView {
                id: mainWindowTextViewScrollView
                rightPadding: effectiveScrollBarWidth
                bottomPadding: effectiveScrollBarHeight
                Layout.minimumWidth: 600; Layout.maximumWidth: 1200
                Layout.minimumHeight: 200; Layout.maximumHeight: 400

                ScrollBar.vertical: ScrollBar {
                    parent: mainWindowTextViewScrollView
                    x: parent.mirrored ? 0 : parent.width - width
                    y: parent.topPadding
                    height: parent.availableHeight
                    active: parent.ScrollBar.horizontal.active
                    policy: ScrollBar.AsNeeded
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    parent: mainWindowTextViewScrollView
                    x: parent.leftPadding
                    y: parent.height - height
                    width: parent.availableWidth
                    active: parent.ScrollBar.vertical.active
                    policy: ScrollBar.AsNeeded
                    minimumSize: Math.min(1, 20 / Math.max(1, width))
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                TextArea {
                    id: mainWindowTextViewTextArea
                    readOnly: true
                    text: mainWindowTextView.data
                    textFormat: TextEdit.AutoText
                    verticalAlignment: TextEdit.AlignTop
                    wrapMode: TextEdit.Wrap
                }
            }
        }
    }

    ToolTip {
        id: mainWindowToolTip
        parent: Overlay.overlay
        closePolicy: Popup.NoAutoClose
        visible: text
        property point position

        onPositionChanged: {
            let p = Overlay.overlay.mapFromGlobal(position)
            x = p.x + 10
            y = p.y + 10
        }

        Behavior on width {
            enabled: mainWindowToolTip.visible
            NumberAnimation {
                duration: 80
                easing.type: Easing.OutCubic
            }
        }

        Behavior on height {
            enabled: mainWindowToolTip.visible
            NumberAnimation {
                duration: 80
                easing.type: Easing.OutCubic
            }
        }

        Behavior on x {
            enabled: mainWindowToolTip.visible
            SmoothedAnimation {
                duration: 80
                velocity: -1
            }
        }

        Behavior on y {
            enabled: mainWindowToolTip.visible
            SmoothedAnimation {
                duration: 80
                velocity: -1
            }
        }
    }

    // agent module
    Dialog {
        id: agentModuleRenameDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Rename Conversation")
        standardButtons: Dialog.Ok
        property string oldTitle

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            agentModuleRenameTextField.text = agentModuleRenameDialog.oldTitle
            agentModuleRenameTextField.forceActiveFocus()
            agentModuleRenameTextField.selectAll()
        }
        onAccepted: agentModule.conversationRename(agentModuleRenameTextField.text)

        TextField {
            id: agentModuleRenameTextField
            width: parent.width
            placeholderText: qsTr("Enter new name:")

            onAccepted: agentModuleRenameDialog.accept()
            Keys.onEscapePressed: agentModuleRenameDialog.reject()
        }
    }

    Menu {
        id: agentModuleModeMenu
        implicitWidth: 400
        property int selectedIndex: -1
        readonly property var modes: [
            {
                "title": qsTr("Chat"),
                "description": qsTr("Answer questions without tool access."),
                "icon": "qrc:/icon/chat.svg"
            },
            {
                "title": qsTr("Read"),
                "description": qsTr("Inspect the workspace without making changes."),
                "icon": "qrc:/icon/eye.svg"
            },
            {
                "title": qsTr("Write"),
                "description": qsTr("Inspect and modify workspace data."),
                "icon": "qrc:/icon/edit.svg"
            },
            {
                "title": qsTr("Full access"),
                "description": qsTr("Run programs and use every available tool."),
                "icon": "qrc:/icon/lockOpen.svg"
            }
        ]

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        Instantiator {
            model: agentModuleModeMenu.modes

            delegate: MenuItem {
                id: modeItem
                required property int index
                required property var modelData
                leftPadding: 10
                topPadding: 0; bottomPadding: 0
                implicitWidth: agentModuleModeMenu.availableWidth
                implicitHeight: 58

                contentItem: RowLayout {
                    spacing: 10

                    IconImage {
                        color: modeItem.index === 3 ? global.warningFore3 : global.fore
                        source: modeItem.modelData.icon
                        sourceSize.width: 18; sourceSize.height: 18
                        Layout.preferredWidth: 20; Layout.preferredHeight: 20
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true

                        Label {
                            text: modeItem.modelData.title
                            color: modeItem.index === 3 ? global.warningFore3 : global.fore
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            text: modeItem.modelData.description
                            color: modeItem.index === 3 ? global.warningFore3 : global.stroke
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    IconImage {
                        visible: agentModuleModeMenu.selectedIndex === modeItem.index
                        color: modeItem.index === 3 ? global.warningFore3 : global.fore
                        source: "qrc:/icon/checkmark.svg"
                        sourceSize.width: 16; sourceSize.height: 16
                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    }
                }

                onTriggered: agentModule.conversationModeSet(index)
            }

            onObjectAdded: (index, object) => agentModuleModeMenu.addItem(object)
            onObjectRemoved: (index, object) => agentModuleModeMenu.removeItem(object)
        }
    }

    Menu {
        id: agentModuleModelMenu
        property var providerModel

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        Instantiator {
            model: agentModuleModelMenu.providerModel
            delegate: Menu {
                id: providerMenu
                property string providerId: model.id
                property string providerName: model.display
                property var providerModels: model.models
                title: providerName

                Instantiator {
                    model: providerMenu.providerModels
                    delegate: MenuItem {
                        text: model.display
                        onTriggered: agentModule.conversationModelSet(providerMenu.providerId, model.id)
                    }

                    onObjectAdded: (index, object) => providerMenu.addItem(object)
                    onObjectRemoved: (index, object) => providerMenu.removeItem(object)
                }
            }

            onObjectAdded: (index, object) => agentModuleModelMenu.insertMenu(index, object)
            onObjectRemoved: (index, object) => agentModuleModelMenu.removeMenu(object)
        }
    }

    // breakpoint module
    Dialog {
        id: breakpointModuleEditDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Breakpoint Setting")
        standardButtons: Dialog.Ok
        property url documentUrl
        property int line

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            breakpointModuleEnabledCheckBox.checkState = breakpointModule.enabledGet(breakpointModuleEditDialog.documentUrl, breakpointModuleEditDialog.line) ? Qt.Checked : Qt.Unchecked
            breakpointModuleConditionTextField.text = breakpointModule.conditionGet(breakpointModuleEditDialog.documentUrl, breakpointModuleEditDialog.line)
            breakpointModuleConditionTextField.forceActiveFocus()
            breakpointModuleConditionTextField.selectAll()
        }
        onAccepted: {
            breakpointModule.enabledSet(breakpointModuleEditDialog.documentUrl, breakpointModuleEditDialog.line, breakpointModuleEnabledCheckBox.checked)
            breakpointModule.conditionSet(breakpointModuleEditDialog.documentUrl, breakpointModuleEditDialog.line, breakpointModuleConditionTextField.text)
            breakpointModule.breakpointReload(breakpointModuleEditDialog.documentUrl, breakpointModuleEditDialog.line)
        }

        ColumnLayout {
            width: parent.width

            Label {
                text: breakpointModuleEditDialog.documentUrl + ":" + breakpointModuleEditDialog.line
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            CheckBox {
                id: breakpointModuleEnabledCheckBox
                text: qsTr("Enabled")
            }

            TextField {
                id: breakpointModuleConditionTextField
                placeholderText: qsTr("true")
                Layout.fillWidth: true

                onAccepted: breakpointModuleEditDialog.accept()
                Keys.onEscapePressed: breakpointModuleEditDialog.reject()
            }
        }
    }

    Menu {
        id: breakpointModuleLineMenu
        property url documentUrl
        property int line
        property var treeView

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("View")
            icon.source: "qrc:/icon/eye.svg"
            icon.width: 16; icon.height: 16

            onTriggered: breakpointModule.markerAdd(breakpointModuleLineMenu.documentUrl, breakpointModuleLineMenu.line)
        }

        MenuItem {
            text: qsTr("Setting")
            icon.source: "qrc:/icon/settings.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                breakpointModuleEditDialog.documentUrl = breakpointModuleLineMenu.documentUrl
                breakpointModuleEditDialog.line = breakpointModuleLineMenu.line
                breakpointModuleEditDialog.open()
            }
        }

        MenuItem {
            text: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            onTriggered: breakpointModule.breakpointDelete(breakpointModuleLineMenu.documentUrl, breakpointModuleLineMenu.line)
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Folding")
            icon.source: "qrc:/icon/fold.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Collapse All")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: breakpointModuleLineMenu.treeView.collapseRecursively()
            }

            MenuItem {
                text: qsTr("Expand All")
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16

                onTriggered: breakpointModuleLineMenu.treeView.expandRecursively()
            }
        }
    }

    Menu {
        id: breakpointModuleFileMenu
        property url documentUrl
        property var treeView

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        Menu {
            title: qsTr("Delete Breakpoints")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    breakpointModule.breakpointsDelete(breakpointModuleFileMenu.documentUrl)
                    progress = 0
                    breakpointModuleFileMenu.close()
                }
            }
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Folding")
            icon.source: "qrc:/icon/fold.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Collapse All")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: breakpointModuleFileMenu.treeView.collapseRecursively()
            }

            MenuItem {
                text: qsTr("Expand All")
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16

                onTriggered: breakpointModuleFileMenu.treeView.expandRecursively()
            }
        }
    }

    Menu {
        id: breakpointModuleRootMenu
        property var treeView

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        Menu {
            title: qsTr("Delete All")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    breakpointModule.allDelete()
                    progress = 0
                    breakpointModuleRootMenu.close()
                }
            }
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Folding")
            icon.source: "qrc:/icon/fold.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Collapse All")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: breakpointModuleRootMenu.treeView.collapseRecursively()
            }

            MenuItem {
                text: qsTr("Expand All")
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16

                onTriggered: breakpointModuleRootMenu.treeView.expandRecursively()
            }
        }
    }

    // database module
    Menu {
        id: databaseModuleTableMenu
        property string databaseKey

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Insert")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            onTriggered: databaseModule.databaseInsert("", databaseModuleTableMenu.databaseKey)
        }

        MenuItem {
            text: qsTr("Clear")
            icon.source: "qrc:/icon/eraser.svg"
            icon.width: 16; icon.height: 16

            onTriggered: databaseModule.databaseClear(databaseModuleTableMenu.databaseKey)
        }

        MenuItem {
            text: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            onTriggered: databaseModule.databaseRemove(databaseModuleTableMenu.databaseKey)
        }
    }

    Menu {
        id: databaseModuleRootMenu

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            onTriggered: databaseModule.databaseInsert()
        }

        Menu {
            title: qsTr("Clear")
            icon.source: "qrc:/icon/eraser.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    databaseModule.databaseClear()
                    progress = 0
                    databaseModuleRootMenu.close()
                }
            }
        }
    }

    // datatable module
    Menu {
        id: datatableModuleTableMenu
        property string datatableKey

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Insert")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            onTriggered: datatableModule.datatableInsert("", datatableModuleTableMenu.datatableKey)
        }

        MenuItem {
            text: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            onTriggered: datatableModule.datatableRemove(datatableModuleTableMenu.datatableKey)
        }
    }

    Menu {
        id: datatableModuleRootMenu

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            onTriggered: datatableModule.datatableInsert()
        }

        MenuItem {
            text: qsTr("Export")
            icon.source: "qrc:/icon/share.svg"
            icon.width: 16; icon.height: 16

            onTriggered: datatableModule.datatableExport("")
        }

        Menu {
            title: qsTr("Clear")
            icon.source: "qrc:/icon/eraser.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    datatableModule.datatableClear()
                    progress = 0
                    datatableModuleRootMenu.close()
                }
            }
        }
    }

    // dataplot module
    Menu {
        id: dataplotModuleRootMenu
        property var rootItem

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Resize\tMB3")
            icon.source: "qrc:/icon/resize.svg"
            icon.width: 16; icon.height: 16

            onTriggered: dataplotModuleRootMenu.rootItem.graphResize()
        }

        MenuItem {
            text: qsTr("Auto Resize")
            icon.source: "qrc:/icon/autoResize.svg"
            icon.width: 16; icon.height: 16
            checkable: true
            checked: dataplotModuleRootMenu.rootItem ? dataplotModuleRootMenu.rootItem.resize : true

            onToggled: {
                dataplotModuleRootMenu.rootItem.resize = checked
                dataplotModuleRootMenu.rootItem.graphResize()
            }
        }

        Menu {
            title: qsTr("Clear")
            icon.source: "qrc:/icon/eraser.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    dataplotModuleRootMenu.rootItem.graphClear()
                    progress = 0
                    dataplotModuleRootMenu.close()
                }
            }
        }
    }

    // debug module
    Dialog {
        id: debugModuleErrorDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Select a Thread First")
        standardButtons: Dialog.Ok

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
    }

    // diagnostics module
    Menu {
        id: diagnosticsModuleDiagnosticMenu
        property string diagnostic
        property var position

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Copy")
            icon.source: "qrc:/icon/copy.svg"
            icon.width: 16; icon.height: 16

            onTriggered: fileModule.copyToClipboard(diagnosticsModuleDiagnosticMenu.diagnostic)
        }

        MenuItem {
            text: qsTr("View")
            icon.source: "qrc:/icon/eye.svg"
            icon.width: 16; icon.height: 16

            onTriggered: diagnosticsModule.indicatorFill(diagnosticsModuleDiagnosticMenu.position)
        }
    }

    // document module
    Dialog {
        id: documentModuleGotoDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Goto Line[:Character]")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string documentUrl
        property int line
        property int character

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            documentModuleGotoTextField.text = (documentModuleGotoDialog.line + 1) + ":" + documentModuleGotoDialog.character
            documentModuleGotoTextField.forceActiveFocus()
            documentModuleGotoTextField.selectAll()
        }
        onAccepted: {
            const parts = documentModuleGotoTextField.text.split(":")
            const line = parseInt(parts[0]) - 1
            const character = parts.length > 1 ? parseInt(parts[1]) : 0
            documentModule.indexSet(documentModuleGotoDialog.documentUrl, line, character)
        }

        TextField {
            id: documentModuleGotoTextField
            width: parent.width

            onAccepted: documentModuleGotoDialog.accept()
            Keys.onEscapePressed: documentModuleGotoDialog.reject()
        }
    }

    Dialog {
        id: documentModuleRenameDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Rename Symbol")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string documentUrl
        property int line
        property int character
        property string oldName

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            documentModuleRenameTextField.text = documentModuleRenameDialog.oldName
            documentModuleRenameTextField.forceActiveFocus()
            documentModuleRenameTextField.selectAll()
        }
        onAccepted: {
            const newName = documentModuleRenameTextField.text
            if (newName !== "" && newName !== documentModuleRenameDialog.oldName) {
                documentModule.renameRequest(documentModuleRenameDialog.documentUrl, documentModuleRenameDialog.line, documentModuleRenameDialog.character, newName)
            }
        }

        TextField {
            id: documentModuleRenameTextField
            width: parent.width

            onAccepted: documentModuleRenameDialog.accept()
            Keys.onEscapePressed: documentModuleRenameDialog.reject()
        }
    }

    Dialog {
        id: documentModuleSaveDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Save and Exit")
        standardButtons: Dialog.Yes | Dialog.No | Dialog.Cancel
        property string documentUrl
        property string documentName
        property int closeDecision: Dialog.Cancel

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
            closeDecision = Dialog.Cancel
        }
        onClosed: {
            widgetCount -= 1
            if (closeDecision !== Dialog.Cancel)
                documentModule.documentClose(documentUrl, closeDecision === Dialog.Yes)
        }
        onAccepted: closeDecision = Dialog.Yes

        Label {
            text: qsTr("Do you want to save changes to " + documentModuleSaveDialog.documentName + "?")
        }

        Component.onCompleted: {
            documentModuleSaveDialog.standardButton(Dialog.No).clicked.connect(function () {
                documentModuleSaveDialog.closeDecision = Dialog.No
            })
        }
    }

    Menu {
        id: documentModuleEditorMenu
        focus: false
        implicitWidth: 300
        property var menuSession

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            documentModuleEditorMenu.menuSession = documentModule.menuLoad("editor")
            documentModuleEditorMenuRunHereItem.enabled = threadpoolModule.debugging()
        }

        MenuItem {
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/play.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.successFore3
                }

                Label {
                    text: documentModuleEditorMenu.menuSession ? documentModuleEditorMenu.menuSession.text ? qsTr("Run Selected") : qsTr("Run") : false
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Shift+F10"
                }
            }

            onTriggered: {
                if (documentModuleEditorMenu.menuSession.text) {
                    threadpoolModule.threadStart(documentModuleEditorMenu.menuSession.documentUrl, 0, documentModuleEditorMenu.menuSession.startLine, documentModuleEditorMenu.menuSession.startCharacter, documentModuleEditorMenu.menuSession.endLine, documentModuleEditorMenu.menuSession.endCharacter)
                } else {
                    threadpoolModule.threadStart(documentModuleEditorMenu.menuSession.documentUrl, 0)
                }
            }
        }

        MenuItem {
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/bug.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.successFore3
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

            onTriggered: threadpoolModule.threadStart(documentModuleEditorMenu.menuSession.documentUrl, 1)
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Folding")
            icon.source: "qrc:/icon/fold.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Contract Top")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: documentModule.foldContractTop(documentModuleEditorMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Contract Recursively")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: documentModule.foldContractRecursively(documentModuleEditorMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Expand Recursively")
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16

                onTriggered: documentModule.foldExpandRecursively(documentModuleEditorMenu.menuSession.documentUrl)
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
                    color: global.fore
                }

                Label {
                    text: documentModuleEditorMenu.menuSession ? documentModuleEditorMenu.menuSession.text ? qsTr("Reformat Selected") : qsTr("Reformat") : false
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+Alt+L"
                }
            }

            onTriggered: {
                if (documentModuleEditorMenu.menuSession.text) {
                    documentModule.rangeFormattingRequest(documentModuleEditorMenu.menuSession.documentUrl, documentModuleEditorMenu.menuSession.startLine, documentModuleEditorMenu.menuSession.startCharacter, documentModuleEditorMenu.menuSession.endLine, documentModuleEditorMenu.menuSession.endCharacter)
                } else {
                    documentModule.formattingRequest(documentModuleEditorMenu.menuSession.documentUrl)
                }
            }
        }

        MenuItem {
            text: qsTr("Rename")
            icon.source: "qrc:/icon/rename.svg"
            icon.width: 16; icon.height: 16

            onTriggered: documentModule.prepareRenameRequest(documentModuleEditorMenu.menuSession.documentUrl, documentModuleEditorMenu.menuSession.line, documentModuleEditorMenu.menuSession.character)
        }

        Menu {
            title: qsTr("Navigation")
            icon.source: "qrc:/icon/location.svg"
            icon.width: 16; icon.height: 16
            enabled: documentModuleEditorMenu.menuSession ? documentModuleEditorMenu.menuSession.navigation : false

            MenuItem {
                text: qsTr("Definition(s)")

                onTriggered: documentModule.definitionRequest(documentModuleEditorMenu.menuSession.documentUrl, documentModuleEditorMenu.menuSession.line, documentModuleEditorMenu.menuSession.character)
            }

            MenuItem {
                text: qsTr("References(s)")

                onTriggered: documentModule.referencesRequest(documentModuleEditorMenu.menuSession.documentUrl, documentModuleEditorMenu.menuSession.line, documentModuleEditorMenu.menuSession.character)
            }

            MenuItem {
                text: qsTr("Implementation(s)")

                onTriggered: documentModule.implementationRequest(documentModuleEditorMenu.menuSession.documentUrl, documentModuleEditorMenu.menuSession.line, documentModuleEditorMenu.menuSession.character)
            }

            MenuItem {
                text: qsTr("Type Definition(s)")

                onTriggered: documentModule.typeDefinitionRequest(documentModuleEditorMenu.menuSession.documentUrl, documentModuleEditorMenu.menuSession.line, documentModuleEditorMenu.menuSession.character)
            }
        }

        MenuSeparator {
        }

        MenuItem {
            id: documentModuleEditorMenuAddWatchItem
            text: qsTr("Add Watch")
            icon.source: "qrc:/icon/eye.svg"
            icon.width: 16; icon.height: 16
            enabled: documentModuleEditorMenu.menuSession ? documentModuleEditorMenu.menuSession.text : false

            HoverHandler {
                onHoveredChanged: {
                    if (!hovered) {
                        mainWindowToolTip.text = ""
                    }
                }
                onPointChanged: {
                    if (!documentModuleEditorMenuAddWatchItem.enabled) {
                        mainWindowToolTip.position = parent.mapToGlobal(point.position)
                        mainWindowToolTip.text = qsTr("Nothing selected")
                    }
                }
            }

            onTriggered: {
                watchModuleExpressionDialog.watchIndex = -1
                watchModuleExpressionDialog.watchUrl = documentModuleEditorMenu.menuSession.documentUrl
                watchModuleExpressionDialog.watchExpression = documentModuleEditorMenu.menuSession.text
                watchModuleExpressionDialog.open()
            }
        }

        MenuItem {
            id: documentModuleEditorMenuRunHereItem
            text: qsTr("Run Here")
            icon.source: "qrc:/icon/debugContinue.svg"
            icon.width: 16; icon.height: 16
            enabled: false

            HoverHandler {
                onHoveredChanged: {
                    if (!hovered) {
                        mainWindowToolTip.text = ""
                    }
                }
                onPointChanged: {
                    if (!documentModuleEditorMenuRunHereItem.enabled) {
                        mainWindowToolTip.position = parent.mapToGlobal(point.position)
                        mainWindowToolTip.text = qsTr("No debug sessions")
                    }
                }
            }

            onTriggered: debugModule.stateSet("", 6)
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

                onTriggered: fileModule.fileOpenInExplorer(documentModuleEditorMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Application")
                icon.source: "qrc:/icon/apps.svg"
                icon.width: 16; icon.height: 16

                onTriggered: fileModule.fileOpenInApplication(documentModuleEditorMenu.menuSession.documentUrl)
            }
        }

        MenuItem {
            text: qsTr("Property")
            icon.source: "qrc:/icon/property.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                fileModulePropertyDialog.documentUrl = documentModuleEditorMenu.menuSession.documentUrl
                fileModulePropertyDialog.property()
                fileModulePropertyDialog.open()
            }
        }
    }

    ToolTip {
        id: documentModuleCompletionToolTip
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        property point position
        property var completionWidget
        property int typed

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: {
            widgetCount -= 1
            documentModuleCompletionDetailTimer.stop()
            completionWidget.completionHide()
        }
        onAboutToShow: {
            position = Overlay.overlay.mapFromGlobal(position)
            x = position.x - 30
            y = position.y
            documentModuleCompletionDetailToolTip.open()
            documentModuleCompletionDetailTimer.restart()
        }

        contentItem: TableView {
            id: documentModuleCompletionTableView
            anchors.fill: parent
            anchors.margins: 6
            implicitWidth: Math.max(idealWidth, 200); implicitHeight: Math.min(idealHeight, 150)
            alternatingRows: false
            clip: true
            editTriggers: TableView.NoEditTriggers
            flickableDirection: Flickable.VerticalFlick
            property int idealWidth; property int idealHeight
            property int hoveredRow: -1
            property int selectedRow: -1

            ScrollBar.vertical: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.stroke
                    dark: global.strokePressed
                }
            }

            delegate: Item {
                implicitWidth: documentModuleCompletionTableView.width; implicitHeight: textMetrics.height + 4
                required property int row

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: global.backHover
                    opacity: hoverHandler.hovered ? 1 : 0
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 150
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: documentModuleCompletionTableView.selectedRow === row ? global.backSelected : "transparent"
                }

                TextMetrics {
                    id: textMetrics
                    font: documentModuleCompletionToolTip.font
                    text: model.display
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Item {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                        IconImage {
                            anchors.centerIn: parent
                            width: 16; height: 16
                            source: model.decoration
                            color: global.fore
                        }
                    }

                    Label {
                        font: documentModuleCompletionToolTip.font
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: model.display ? "<span style='color: " + global.brandBack + ";'>" + model.display.substring(0, documentModuleCompletionToolTip.typed) + "</span>" + model.display.substring(documentModuleCompletionToolTip.typed) : ""
                        textFormat: Text.RichText
                        elide: Text.ElideRight
                        Layout.fillWidth: true; Layout.preferredHeight: 24
                    }
                }

                HoverHandler {
                    id: hoverHandler

                    onPointChanged: documentModuleCompletionTableView.hoveredRow = row
                    onHoveredChanged: {
                        if (!hovered) {
                            documentModuleCompletionTableView.hoveredRow = -1
                            documentModuleCompletionDetailTimer.restart()
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton

                    onTapped: documentModuleCompletionTableView.selectedRow = row
                    onDoubleTapped: documentModuleCompletionToolTip.completionWidget.textReplace()
                }

                Component.onCompleted: {
                    documentModuleCompletionTableView.idealWidth = Math.max(24 + textMetrics.width + 4 + 10, documentModuleCompletionTableView.idealWidth)
                    documentModuleCompletionTableView.idealHeight = textMetrics.height + 4 + documentModuleCompletionTableView.idealHeight
                }
            }

            onHoveredRowChanged: documentModuleCompletionDetailTimer.restart()

            onSelectedRowChanged: {
                positionViewAtRow(selectedRow, TableView.Contain, 0, Qt.rect(0, 0, 0, 0))
                documentModuleCompletionDetailTimer.restart()
            }

            Timer {
                id: documentModuleCompletionDetailTimer
                interval: 150

                onTriggered: {
                    var interestRow
                    if (documentModuleCompletionTableView.hoveredRow !== -1) {
                        interestRow = documentModuleCompletionTableView.hoveredRow
                    } else {
                        interestRow = documentModuleCompletionTableView.selectedRow
                    }
                    documentModuleCompletionToolTip.completionWidget.detailReload(interestRow)
                    const index = documentModuleCompletionTableView.index(interestRow, 0);
                    const item = documentModuleCompletionTableView.itemAtIndex(index);
                    if (item) {
                        let idealY = item.mapToItem(documentModuleCompletionTableView, 0, 0).y
                        idealY = Math.max(0, idealY)
                        idealY = Math.min(documentModuleCompletionTableView.height - item.height, idealY)
                        documentModuleCompletionDetailToolTip.y = idealY - 6
                    }
                }
            }

            function completionPrev() {
                if (selectedRow > 0) {
                    selectedRow = selectedRow - 1
                }
                // else {
                //     selectedRow = model.rowCount() - 1
                // }
            }

            function completionNext() {
                if (selectedRow < model.rowCount() - 1) {
                    selectedRow = selectedRow + 1
                }
                // else {
                //     selectedRow = 0
                // }
            }

            Connections {
                target: documentModuleCompletionTableView.model

                function onModelReset() {
                    documentModuleCompletionTableView.idealWidth = 0
                    documentModuleCompletionTableView.idealHeight = 0
                    documentModuleCompletionDetailTimer.restart()
                }
            }
        }

        ToolTip {
            id: documentModuleCompletionDetailToolTip
            x: documentModuleCompletionToolTip.width - 5

            contentItem: TableView {
                id: documentModuleCompletionDetailTableView
                anchors.fill: parent
                anchors.margins: 6
                implicitWidth: idealWidth; implicitHeight: idealHeight
                alternatingRows: false
                clip: true
                editTriggers: TableView.NoEditTriggers
                flickableDirection: Flickable.VerticalFlick
                property int idealWidth; property int idealHeight

                delegate: Item {
                    implicitWidth: documentModuleCompletionDetailTableView.width; implicitHeight: textMetrics.height + 4

                    TextMetrics {
                        id: textMetrics
                        font: documentModuleCompletionToolTip.font
                        text: model.display
                    }

                    Label {
                        id: documentModuleCompletionDetailLabel
                        anchors.fill: parent
                        font: documentModuleCompletionToolTip.font
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: model.display
                        elide: Text.ElideRight
                    }

                    Component.onCompleted: {
                        documentModuleCompletionDetailTableView.idealWidth = Math.max(textMetrics.width + 4, documentModuleCompletionDetailTableView.idealWidth)
                        documentModuleCompletionDetailTableView.idealHeight = textMetrics.height + 4 + documentModuleCompletionDetailTableView.idealHeight
                    }
                }

                Connections {
                    target: documentModuleCompletionDetailTableView.model

                    function onModelReset() {
                        documentModuleCompletionDetailTableView.idealWidth = 0
                        documentModuleCompletionDetailTableView.idealHeight = 0
                    }
                }
            }
        }
    }

    ToolTip {
        id: documentModuleDwellToolTip
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        property point position
        property var dwellWidget
        property var codeActions
        property var suggestions

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: {
            widgetCount -= 1
            dwellWidget.dwellHide()
        }
        onAboutToShow: {
            position = Overlay.overlay.mapFromGlobal(position)
            x = position.x + 10
            y = position.y + 10
        }

        contentItem: ColumnLayout {

            ScrollView {
                id: documentModuleDwellDiagnosticScrollView
                visible: documentModuleDwellDiagnosticTextArea.length > 0
                rightPadding: effectiveScrollBarWidth
                bottomPadding: effectiveScrollBarHeight
                Layout.minimumWidth: 400; Layout.maximumWidth: 800

                ScrollBar.vertical: ScrollBar {
                    parent: documentModuleDwellDiagnosticScrollView
                    x: parent.mirrored ? 0 : parent.width - width
                    y: parent.topPadding
                    height: parent.availableHeight
                    active: parent.ScrollBar.horizontal.active
                    policy: ScrollBar.AsNeeded
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    parent: documentModuleDwellDiagnosticScrollView
                    x: parent.leftPadding
                    y: parent.height - height
                    width: parent.availableWidth
                    active: parent.ScrollBar.vertical.active
                    policy: ScrollBar.AsNeeded
                    minimumSize: Math.min(1, 20 / Math.max(1, width))
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                TextArea {
                    id: documentModuleDwellDiagnosticTextArea
                    background: null
                    readOnly: true
                    textFormat: TextEdit.RichText
                    verticalAlignment: TextEdit.AlignTop
                    wrapMode: Text.Wrap

                    HoverHandler {
                        id: diagnosticHoverHandler
                        cursorShape: documentModuleDwellDiagnosticTextArea.hoveredLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        onTapped: {
                            if (documentModuleDwellDiagnosticTextArea.hoveredLink) {
                                documentModuleDwellToolTip.dwellWidget.linkClick(documentModuleDwellDiagnosticTextArea.hoveredLink)
                            }
                        }
                    }
                }
            }

            ScrollView {
                id: documentModuleDwellHoverScrollView
                visible: documentModuleDwellHoverTextArea.length > 0
                rightPadding: effectiveScrollBarWidth
                bottomPadding: effectiveScrollBarHeight
                Layout.minimumWidth: 400; Layout.maximumWidth: 800

                ScrollBar.vertical: ScrollBar {
                    parent: documentModuleDwellHoverScrollView
                    x: parent.mirrored ? 0 : parent.width - width
                    y: parent.topPadding
                    height: parent.availableHeight
                    active: parent.ScrollBar.horizontal.active
                    policy: ScrollBar.AsNeeded
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    parent: documentModuleDwellHoverScrollView
                    x: parent.leftPadding
                    y: parent.height - height
                    width: parent.availableWidth
                    active: parent.ScrollBar.vertical.active
                    policy: ScrollBar.AsNeeded
                    minimumSize: Math.min(1, 20 / Math.max(1, width))
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                TextArea {
                    id: documentModuleDwellHoverTextArea
                    background: null
                    readOnly: true
                    textFormat: TextEdit.MarkdownText
                    verticalAlignment: TextEdit.AlignTop
                    wrapMode: Text.Wrap

                    HoverHandler {
                        id: dwellHoverHandler
                        cursorShape: documentModuleDwellHoverTextArea.hoveredLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                    }

                    ToolTip {
                        visible: documentModuleDwellHoverTextArea.hoveredLink
                        text: "Click to open link"
                        x: dwellHoverHandler.point.position.x + 10
                        y: dwellHoverHandler.point.position.y + 10
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        onTapped: {
                            if (documentModuleDwellHoverTextArea.hoveredLink) {
                                documentModule.documentOpen(documentModuleDwellHoverTextArea.hoveredLink)
                            }
                        }
                    }
                }
            }
        }

        Menu {
            id: documentModuleDwellCodeActionMenu
            x: documentModuleDwellToolTip.width - 5; y: -8

            onAboutToShow: {
                for (var i = documentModuleDwellCodeActionMenu.count - 1; i >= 0; --i) {
                    var item = documentModuleDwellCodeActionMenu.itemAt(i)
                    documentModuleDwellCodeActionMenu.removeItem(item)
                    item.destroy()
                }
                if (documentModuleDwellToolTip.codeActions.length) {
                    for (let i = 0; i < documentModuleDwellToolTip.codeActions.length; ++i) {
                        const menuItem = documentModuleDwellCodeActionMenuComponent.createObject(
                            null,
                            {
                                text: documentModuleDwellToolTip.codeActions[i]["title"],
                                codeAction: documentModuleDwellToolTip.codeActions[i]["edit"]
                            }
                        )
                        documentModuleDwellCodeActionMenu.addItem(menuItem)
                    }
                } else {
                    const menuItem = documentModuleDwellCodeActionMenuComponent.createObject(
                        null,
                        {
                            text: qsTr("No fixes available")
                        }
                    )
                    documentModuleDwellCodeActionMenu.addItem(menuItem)
                }
            }

            Component {
                id: documentModuleDwellCodeActionMenuComponent

                MenuItem {
                    property var codeAction

                    onTriggered: {
                        documentModuleDwellToolTip.dwellWidget.codeActionAccept(codeAction)
                        documentModuleDwellToolTip.close()
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: text
                }
            }
        }

        Menu {
            id: documentModuleDwellSuggestionMenu
            x: documentModuleDwellToolTip.width - 5; y: -8

            MenuItem {
                text: documentModuleDwellToolTip.suggestions ? documentModuleDwellToolTip.suggestions[0] : ""
                visible: text

                onTriggered: {
                    documentModuleDwellToolTip.dwellWidget.suggestionAccept(text)
                    documentModuleDwellToolTip.close()
                }
            }

            MenuItem {
                text: documentModuleDwellToolTip.suggestions ? documentModuleDwellToolTip.suggestions[1] : ""
                visible: text

                onTriggered: {
                    documentModuleDwellToolTip.dwellWidget.suggestionAccept(text)
                    documentModuleDwellToolTip.close()
                }
            }

            MenuItem {
                text: documentModuleDwellToolTip.suggestions ? documentModuleDwellToolTip.suggestions[2] : ""
                visible: text

                onTriggered: {
                    documentModuleDwellToolTip.dwellWidget.suggestionAccept(text)
                    documentModuleDwellToolTip.close()
                }
            }

            MenuItem {
                text: documentModuleDwellToolTip.suggestions ? documentModuleDwellToolTip.suggestions[3] : ""
                visible: text

                onTriggered: {
                    documentModuleDwellToolTip.dwellWidget.suggestionAccept(text)
                    documentModuleDwellToolTip.close()
                }
            }

            MenuItem {
                text: documentModuleDwellToolTip.suggestions ? documentModuleDwellToolTip.suggestions[4] : ""
                visible: text

                onTriggered: {
                    documentModuleDwellToolTip.dwellWidget.suggestionAccept(text)
                    documentModuleDwellToolTip.close()
                }
            }
        }
    }

    ToolTip {
        id: documentModuleNavigationToolTip
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        property point position
        property var navigationWidget

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: {
            widgetCount -= 1
            documentModuleNavigationDetailTimer.stop()
            navigationWidget.navigationHide()
        }
        onAboutToShow: {
            position = Overlay.overlay.mapFromGlobal(position)
            x = position.x - 6
            y = position.y
            documentModuleNavigationDetailToolTip.open()
            documentModuleNavigationDetailTimer.restart()
        }

        contentItem: TableView {
            id: documentModuleNavigationTableView
            anchors.fill: parent
            anchors.margins: 6
            implicitWidth: Math.max(idealWidth, 200); implicitHeight: Math.min(idealHeight, 150)
            alternatingRows: false
            clip: true
            editTriggers: TableView.NoEditTriggers
            flickableDirection: Flickable.VerticalFlick
            property int idealWidth; property int idealHeight
            property int hoveredRow: -1
            property int selectedRow: -1

            ScrollBar.vertical: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.stroke
                    dark: global.strokePressed
                }
            }

            delegate: Item {
                implicitWidth: documentModuleNavigationTableView.width; implicitHeight: textMetrics.height + 4
                required property int row

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: global.backHover
                    opacity: hoverHandler.hovered ? 1 : 0
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 150
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: documentModuleNavigationTableView.selectedRow === row ? global.backSelected : "transparent"
                }

                TextMetrics {
                    id: textMetrics
                    font: documentModuleNavigationToolTip.font
                    text: model.display
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Label {
                        font: documentModuleNavigationToolTip.font
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: model.display
                        elide: Text.ElideRight
                        Layout.fillWidth: true; Layout.preferredHeight: 24
                    }
                }

                HoverHandler {
                    id: hoverHandler

                    onPointChanged: documentModuleNavigationTableView.hoveredRow = row
                    onHoveredChanged: {
                        if (!hovered) {
                            documentModuleNavigationTableView.hoveredRow = -1
                            documentModuleNavigationDetailTimer.restart()
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton

                    onTapped: documentModuleNavigationTableView.selectedRow = row
                    onDoubleTapped: documentModuleNavigationToolTip.navigationWidget.indicatorInsert()
                }

                Component.onCompleted: {
                    documentModuleNavigationTableView.idealWidth = Math.max(24 + textMetrics.width + 4 + 10, documentModuleNavigationTableView.idealWidth)
                    documentModuleNavigationTableView.idealHeight = textMetrics.height + 4 + documentModuleNavigationTableView.idealHeight
                }
            }

            onHoveredRowChanged: documentModuleNavigationDetailTimer.restart()

            onSelectedRowChanged: {
                positionViewAtRow(selectedRow, TableView.Contain, 0, Qt.rect(0, 0, 0, 0))
                documentModuleNavigationDetailTimer.restart()
            }

            Timer {
                id: documentModuleNavigationDetailTimer
                interval: 150

                onTriggered: {
                    var interestRow
                    if (documentModuleNavigationTableView.hoveredRow !== -1) {
                        interestRow = documentModuleNavigationTableView.hoveredRow
                    } else {
                        interestRow = documentModuleNavigationTableView.selectedRow
                    }
                    documentModuleNavigationToolTip.navigationWidget.detailReload(interestRow)
                    const index = documentModuleNavigationTableView.index(interestRow, 0);
                    const item = documentModuleNavigationTableView.itemAtIndex(index);
                    if (item) {
                        let idealY = item.mapToItem(documentModuleNavigationTableView, 0, 0).y
                        idealY = Math.max(0, idealY)
                        idealY = Math.min(documentModuleNavigationTableView.height - item.height, idealY)
                        documentModuleNavigationDetailToolTip.y = idealY - 6
                    }
                }
            }

            function navigationPrev() {
                if (selectedRow > 0) {
                    selectedRow = selectedRow - 1
                }
                // else {
                //     selectedRow = model.rowCount() - 1
                // }
            }

            function navigationNext() {
                if (selectedRow < model.rowCount() - 1) {
                    selectedRow = selectedRow + 1
                }
                // else {
                //     selectedRow = 0
                // }
            }

            Connections {
                target: documentModuleNavigationTableView.model

                function onModelReset() {
                    documentModuleNavigationTableView.idealWidth = 0
                    documentModuleNavigationTableView.idealHeight = 0
                    documentModuleNavigationDetailTimer.restart()
                }
            }
        }

        ToolTip {
            id: documentModuleNavigationDetailToolTip
            x: documentModuleNavigationToolTip.width - 5

            contentItem: Label {
                id: documentModuleNavigationDetailLabel
                font: documentModuleNavigationToolTip.font
                textFormat: Text.RichText
            }
        }
    }

    ToolTip {
        id: documentModulePositionTooltip
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        property point position

        onPositionChanged: {
            let p = Overlay.overlay.mapFromGlobal(position)
            x = p.x + 10
            y = p.y + 10
        }

        Behavior on x {
            enabled: documentModulePositionTooltip.visible
            NumberAnimation {
                duration: 50
                easing.type: Easing.Linear
            }
        }
        Behavior on y {
            enabled: documentModulePositionTooltip.visible
            NumberAnimation {
                duration: 50
                easing.type: Easing.Linear
            }
        }
    }

    ToolTip {
        id: documentModuleSignatureToolTip
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        property point position
        property var signatureWidget

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: {
            widgetCount -= 1
            signatureWidget.signatureHide()
        }
        onAboutToShow: {
            position = Overlay.overlay.mapFromGlobal(position)
            x = position.x - 6
            y = position.y - documentModuleSignatureToolTip.implicitHeight
        }

        contentItem: Label {
            id: documentModuleSignatureLabel
            textFormat: Text.RichText
        }
    }

    // explorer module
    Dialog {
        id: explorerModuleFileNewDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("New File")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string documentUrl

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            explorerModuleFileNameTextField.clear()
            explorerModuleFileNameTextField.forceActiveFocus()
        }
        onAccepted: {
            if (!explorerModuleFileNameTextField.acceptableInput) {
                return
            }
            operationErrorShow(documentModule.documentCreate(explorerModuleFileNewDialog.documentUrl + "/" + explorerModuleFileNameTextField.text))
        }

        Binding {
            target: explorerModuleFileNewDialog.visible
                ? explorerModuleFileNewDialog.standardButton(Dialog.Ok) : null
            property: "enabled"
            value: explorerModuleFileNameTextField.acceptableInput
        }

        TextField {
            id: explorerModuleFileNameTextField
            width: parent.width
            placeholderText: qsTr("Enter file name:")
            validator: RegularExpressionValidator {
                regularExpression: /.*\..*/
            }

            onAccepted: explorerModuleFileNewDialog.accept()
            Keys.onEscapePressed: explorerModuleFileNewDialog.reject()
        }
    }

    Dialog {
        id: explorerModuleFolderNewDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("New Folder")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string documentUrl

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            explorerModuleFolderNameTextField.clear()
            explorerModuleFolderNameTextField.forceActiveFocus()
        }
        onAccepted: operationErrorShow(
            documentModule.directoryCreate(explorerModuleFolderNewDialog.documentUrl + "/" + explorerModuleFolderNameTextField.text))

        TextField {
            id: explorerModuleFolderNameTextField
            width: parent.width
            placeholderText: qsTr("Enter folder name:")

            onAccepted: explorerModuleFolderNewDialog.accept()
            Keys.onEscapePressed: explorerModuleFolderNewDialog.reject()
        }
    }

    Menu {
        id: explorerModuleFileMenu
        property bool gitUntracked
        property bool gitIgnored
        property string documentUrl

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Run")
            icon.source: "qrc:/icon/play.svg"
            icon.width: 16; icon.height: 16

            onTriggered: explorerModule.scriptRun(explorerModuleFileMenu.documentUrl)
        }

        MenuItem {
            text: qsTr("Debug")
            icon.source: "qrc:/icon/bug.svg"
            icon.width: 16; icon.height: 16

            onTriggered: explorerModule.scriptDebug(explorerModuleFileMenu.documentUrl)
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

                onTriggered: fileModule.fileOpenInExplorer(explorerModuleFileMenu.documentUrl)
            }

            MenuItem {
                text: qsTr("Application")
                icon.source: "qrc:/icon/apps.svg"
                icon.width: 16; icon.height: 16

                onTriggered: fileModule.fileOpenInApplication(explorerModuleFileMenu.documentUrl)
            }
        }

        MenuItem {
            text: qsTr("Rename")
            icon.source: "qrc:/icon/rename.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                fileModulePropertyDialog.documentUrl = explorerModuleFileMenu.documentUrl
                fileModulePropertyDialog.rename()
                fileModulePropertyDialog.open()
            }
        }

        Menu {
            title: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    operationErrorShow(documentModule.documentDelete(explorerModuleFileMenu.documentUrl))
                    progress = 0
                    explorerModuleFileMenu.close()
                }
            }
        }

        MenuItem {
            text: qsTr("Property")
            icon.source: "qrc:/icon/property.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                fileModulePropertyDialog.documentUrl = explorerModuleFileMenu.documentUrl
                fileModulePropertyDialog.property()
                fileModulePropertyDialog.open()
            }
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Git")
            enabled: global.gitEnabled
            icon.source: "qrc:/icon/fileTypeGit.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Add")
                enabled: explorerModuleFileMenu.gitUntracked
                icon.source: "qrc:/icon/add.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    gitModule.gitAdd(explorerModuleFileMenu.documentUrl)
                }
            }

            Menu {
                title: qsTr("Restore")
                enabled: !explorerModuleFileMenu.gitUntracked && !explorerModuleFileMenu.gitIgnored
                icon.source: "qrc:/icon/reset.svg"
                icon.width: 16; icon.height: 16

                MenuItem {
                    text: qsTr("Worktree")

                    onTriggered: gitModule.gitRestore(explorerModuleFileMenu.documentUrl, 0)
                }

                MenuItem {
                    text: qsTr("Staged")

                    onTriggered: gitModule.gitRestore(explorerModuleFileMenu.documentUrl, 1)
                }

                MenuItem {
                    text: qsTr("Both")

                    onTriggered: gitModule.gitRestore(explorerModuleFileMenu.documentUrl, 2)
                }
            }

            MenuItem {
                text: explorerModuleFileMenu.gitIgnored ? qsTr("Unignore") : qsTr("Ignore")
                icon.source: "qrc:/icon/prohibited.svg"
                icon.width: 16; icon.height: 16

                onTriggered: gitModule.gitIgnore(explorerModuleFileMenu.documentUrl, !explorerModuleFileMenu.gitIgnored)
            }
        }
    }

    Menu {
        id: explorerModuleFolderMenu
        property string documentUrl

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Open In Explorer")
            icon.source: "qrc:/icon/open.svg"
            icon.width: 16; icon.height: 16

            onTriggered: fileModule.fileOpenInExplorer(explorerModuleFolderMenu.documentUrl)
        }

        Menu {
            title: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("File")
                icon.source: "qrc:/icon/document.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    explorerModuleFileNewDialog.documentUrl = explorerModuleFolderMenu.documentUrl
                    explorerModuleFileNewDialog.open()
                }
            }

            MenuItem {
                text: qsTr("Folder")
                icon.source: "qrc:/icon/folder.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    explorerModuleFolderNewDialog.documentUrl = explorerModuleFolderMenu.documentUrl
                    explorerModuleFolderNewDialog.open()
                }
            }
        }

        MenuItem {
            text: qsTr("Rename")
            icon.source: "qrc:/icon/rename.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                fileModulePropertyDialog.documentUrl = explorerModuleFolderMenu.documentUrl
                fileModulePropertyDialog.open()
                fileModulePropertyNameTextField.forceActiveFocus()
                fileModulePropertyNameTextField.selectAll()
            }
        }

        Menu {
            title: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    operationErrorShow(documentModule.directoryDelete(explorerModuleFolderMenu.documentUrl))
                    progress = 0
                    explorerModuleFolderMenu.close()
                }
            }
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Git")
            enabled: global.gitEnabled
            icon.source: "qrc:/icon/fileTypeGit.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Add Folder")
                icon.source: "qrc:/icon/add.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    gitModule.gitAdd(explorerModuleFolderMenu.documentUrl)
                }
            }

            Menu {
                title: qsTr("Restore Folder")
                icon.source: "qrc:/icon/reset.svg"
                icon.width: 16; icon.height: 16

                MenuItem {
                    text: qsTr("Worktree")

                    onTriggered: gitModule.gitRestore(explorerModuleFolderMenu.documentUrl, 0)
                }

                MenuItem {
                    text: qsTr("Staged")

                    onTriggered: gitModule.gitRestore(explorerModuleFolderMenu.documentUrl, 1)
                }

                MenuItem {
                    text: qsTr("Both")

                    onTriggered: gitModule.gitRestore(explorerModuleFolderMenu.documentUrl, 2)
                }
            }

            MenuItem {
                text: qsTr("Ignore")
                icon.source: "qrc:/icon/prohibited.svg"
                icon.width: 16; icon.height: 16

                onTriggered: gitModule.gitIgnore(explorerModuleFolderMenu.documentUrl, true)
            }
        }
    }

    Menu {
        id: explorerModuleRootMenu
        property string documentUrl

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Open In Explorer")
            icon.source: "qrc:/icon/open.svg"
            icon.width: 16; icon.height: 16

            onTriggered: fileModule.fileOpenInExplorer(explorerModuleRootMenu.documentUrl)
        }

        Menu {
            title: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("File")
                icon.source: "qrc:/icon/document.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    explorerModuleFileNewDialog.documentUrl = explorerModuleRootMenu.documentUrl
                    explorerModuleFileNewDialog.open()
                }
            }

            MenuItem {
                text: qsTr("Folder")
                icon.source: "qrc:/icon/folder.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    explorerModuleFolderNewDialog.documentUrl = explorerModuleRootMenu.documentUrl
                    explorerModuleFolderNewDialog.open()
                }
            }
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Git")
            enabled: global.gitEnabled
            icon.source: "qrc:/icon/fileTypeGit.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Add All")
                icon.source: "qrc:/icon/add.svg"
                icon.width: 16; icon.height: 16

                onTriggered: {
                    gitModule.gitAdd()
                }
            }

            Menu {
                title: qsTr("Restore All")
                icon.source: "qrc:/icon/reset.svg"
                icon.width: 16; icon.height: 16

                MenuItem {
                    text: qsTr("Worktree")

                    onTriggered: gitModule.gitRestore("", 0)
                }

                MenuItem {
                    text: qsTr("Staged")

                    onTriggered: gitModule.gitRestore("", 1)
                }

                MenuItem {
                    text: qsTr("Both")

                    onTriggered: gitModule.gitRestore("", 2)
                }
            }
        }
    }

    // file module
    Dialog {
        id: fileModulePropertyDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("File Property")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string documentUrl
        property var infoSession
        property bool directory

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            fileModulePropertyDialog.infoSession = fileModule.fileInfo(fileModulePropertyDialog.documentUrl)
            fileModulePropertyDialog.directory = fileModulePropertyDialog.infoSession.directory
            fileModulePropertyImage.source = fileModulePropertyDialog.infoSession.source
            fileModulePropertyNameTextField.text = fileModulePropertyDialog.infoSession.baseName
            fileModulePropertySizeLabel.text = fileModulePropertyDialog.infoSession.size
            fileModulePropertyAbsolutePathLabel.text = fileModulePropertyDialog.infoSession.absolutePath
            fileModulePropertyBirthTimeLabel.text = fileModulePropertyDialog.infoSession.birthTime
            fileModulePropertyLastModifiedLabel.text = fileModulePropertyDialog.infoSession.lastModified
            fileModulePropertyLastReadLabel.text = fileModulePropertyDialog.infoSession.lastRead
            fileModulePropertyReadableCheckBox.checked = fileModulePropertyDialog.infoSession.readable
            fileModulePropertyWritableCheckBox.checked = fileModulePropertyDialog.infoSession.writable
            fileModulePropertyHiddenCheckBox.checked = fileModulePropertyDialog.infoSession.hidden
        }

        onAccepted: {
            if (fileModulePropertyNameTextField.text !== fileModulePropertyDialog.infoSession.baseName) {
                const suffix = fileModulePropertyDialog.directory || !fileModulePropertyDialog.infoSession.suffix
                    ? "" : "." + fileModulePropertyDialog.infoSession.suffix
                const targetUrl = fileModulePropertyDialog.infoSession.parentUrl + "/"
                    + encodeURIComponent(fileModulePropertyNameTextField.text + suffix)
                const error = fileModulePropertyDialog.directory
                    ? documentModule.directoryRename(fileModulePropertyDialog.documentUrl, targetUrl)
                    : documentModule.documentRename(fileModulePropertyDialog.documentUrl, targetUrl)
                operationErrorShow(error)
            }
            if (fileModulePropertyWritableCheckBox.checked !== fileModulePropertyDialog.infoSession.writable) {
                fileModule.fileWritable(fileModulePropertyDialog.documentUrl, fileModulePropertyWritableCheckBox.checked)
            }
        }

        function rename() {
            fileModulePropertyNameTextField.forceActiveFocus()
            fileModulePropertyNameTextField.selectAll()
        }

        function permission() {
            fileModulePropertyWritableCheckBox.forceActiveFocus()
        }

        function property() {
            fileModulePropertyNameTextField.forceActiveFocus()
            fileModulePropertyNameTextField.cursorPosition = fileModulePropertyNameTextField.length
        }

        ColumnLayout {
            width: parent.width

            RowLayout {
                Layout.preferredHeight: 64

                Image {
                    id: fileModulePropertyImage
                    sourceSize.width: 48
                    sourceSize.height: 48
                    horizontalAlignment: Image.AlignLeft
                    fillMode: Image.PreserveAspectFit
                    Layout.preferredWidth: 150
                }

                TextField {
                    id: fileModulePropertyNameTextField
                    placeholderText: qsTr("Enter new name:")
                    Layout.fillWidth: true

                    onAccepted: fileModulePropertyDialog.accept()
                    Keys.onEscapePressed: fileModulePropertyDialog.reject()
                }
            }

            Rectangle {
                color: global.stroke
                Layout.fillWidth: true; Layout.preferredHeight: 1
            }

            RowLayout {
                Layout.preferredHeight: 32

                Label {
                    text: qsTr("Absolute Path:")
                    Layout.preferredWidth: 150
                }

                Label {
                    id: fileModulePropertyAbsolutePathLabel
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
                    id: fileModulePropertySizeLabel
                }
            }

            Rectangle {
                color: global.stroke
                Layout.fillWidth: true; Layout.preferredHeight: 1
            }

            RowLayout {
                Layout.preferredHeight: 32

                Label {
                    text: qsTr("Birth Time:")
                    Layout.preferredWidth: 150
                }

                Label {
                    id: fileModulePropertyBirthTimeLabel
                }
            }

            RowLayout {
                Layout.preferredHeight: 32

                Label {
                    text: qsTr("Last Modified:")
                    Layout.preferredWidth: 150
                }

                Label {
                    id: fileModulePropertyLastModifiedLabel
                }
            }

            RowLayout {
                Layout.preferredHeight: 32

                Label {
                    text: qsTr("Last Read:")
                    Layout.preferredWidth: 150
                }

                Label {
                    id: fileModulePropertyLastReadLabel
                }
            }

            Rectangle {
                color: global.stroke
                Layout.fillWidth: true; Layout.preferredHeight: 1
            }

            RowLayout {
                Layout.preferredHeight: 32

                Label {
                    text: qsTr("Attributes:")
                    Layout.preferredWidth: 150
                }

                CheckBox {
                    id: fileModulePropertyReadableCheckBox
                    text: qsTr("Readable")
                }

                CheckBox {
                    id: fileModulePropertyWritableCheckBox
                    text: qsTr("Writable")

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: -3
                        z: -1
                        visible: parent.activeFocus
                        color: "transparent"
                        border.width: 2
                        border.color: global.warningBack3
                        radius: 7
                    }
                }

                CheckBox {
                    id: fileModulePropertyHiddenCheckBox
                    text: qsTr("Hidden")
                }
            }
        }
    }

    // git module
    Dialog {
        id: gitModuleContinueDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: global.gitStatus === 2 ? qsTr("Finish Merge") : qsTr("Finish Rebase")
        standardButtons: Dialog.Ok

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            gitModuleContinueCheckBox.checked = global.gitStatus !== 2
            gitModuleContinueCheckBox.enabled = global.gitStatus === 2
            gitModuleContinueTextField.text = ""
            gitModuleContinueTextField.forceActiveFocus()
        }

        onAccepted: gitModule.gitContinue(gitModuleContinueTextField.text)

        ColumnLayout {
            width: parent.width

            CheckBox {
                id: gitModuleContinueCheckBox
                text: qsTr("Used auto-generated message")
            }

            TextField {
                id: gitModuleContinueTextField
                visible: !gitModuleContinueCheckBox.checked
                placeholderText: qsTr("Enter commit message:")
                Layout.fillWidth: true

                onAccepted: gitModuleContinueDialog.accept()
                Keys.onEscapePressed: gitModuleContinueDialog.reject()
            }
        }
    }

    Dialog {
        id: gitModuleErrorDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        standardButtons: Dialog.Ok
        property string text

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        Label {
            width: parent.width
            text: gitModuleErrorDialog.text
            wrapMode: Text.WrapAnywhere
        }
    }

    Dialog {
        id: gitModuleBranchCreateDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Create branch from ") + gitModuleBranchCreateDialog.name
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string name

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            gitModuleBranchCreateTextField.text = gitModuleBranchCreateDialog.name
            gitModuleBranchCreateTextField.forceActiveFocus()
            gitModuleBranchCreateTextField.selectAll()
            gitModuleBranchSwitchCheckBox.checked = true
        }

        onAccepted: gitModule.gitCreate(gitModuleBranchCreateDialog.name, gitModuleBranchCreateTextField.text, gitModuleBranchSwitchCheckBox.checked)

        ColumnLayout {
            width: parent.width

            TextField {
                id: gitModuleBranchCreateTextField
                placeholderText: qsTr("Enter branch name:")
                Layout.fillWidth: true

                onAccepted: gitModuleBranchCreateDialog.accept()
                Keys.onEscapePressed: gitModuleBranchCreateDialog.reject()
            }

            RowLayout {
                Layout.preferredHeight: 32

                CheckBox {
                    id: gitModuleBranchSwitchCheckBox
                    text: qsTr("Switch")
                }
            }
        }
    }

    Dialog {
        id: gitModuleProxyDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Proxy Configuration")
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string localHttpProxy
        property string localHttpsProxy
        property string globalHttpProxy
        property string globalHttpsProxy

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            gitModuleLocalHttpProxyTextField.text = gitModuleProxyDialog.localHttpProxy
            gitModuleLocalHttpsProxyTextField.text = gitModuleProxyDialog.localHttpsProxy
            gitModuleGlobalHttpProxyTextField.text = gitModuleProxyDialog.globalHttpProxy
            gitModuleGlobalHttpsProxyTextField.text = gitModuleProxyDialog.globalHttpsProxy
        }

        onAccepted: gitModule.gitProxySet(
            gitModuleLocalHttpProxyTextField.text,
            gitModuleLocalHttpsProxyTextField.text,
            gitModuleGlobalHttpProxyTextField.text,
            gitModuleGlobalHttpsProxyTextField.text)

        ColumnLayout {
            width: parent.width

            Label {
                text: qsTr("Local Http Proxy")
                Layout.fillWidth: true
            }

            TextField {
                id: gitModuleLocalHttpProxyTextField
                Layout.fillWidth: true

                onAccepted: gitModuleProxyDialog.accept()
                Keys.onEscapePressed: gitModuleProxyDialog.reject()
            }

            Label {
                text: qsTr("Local Https Proxy")
                Layout.fillWidth: true
            }

            TextField {
                id: gitModuleLocalHttpsProxyTextField
                Layout.fillWidth: true

                onAccepted: gitModuleProxyDialog.accept()
                Keys.onEscapePressed: gitModuleProxyDialog.reject()
            }

            Label {
                text: qsTr("Global Http Proxy")
                Layout.fillWidth: true
            }

            TextField {
                id: gitModuleGlobalHttpProxyTextField
                Layout.fillWidth: true

                onAccepted: gitModuleProxyDialog.accept()
                Keys.onEscapePressed: gitModuleProxyDialog.reject()
            }

            Label {
                text: qsTr("Global Https Proxy")
                Layout.fillWidth: true
            }

            TextField {
                id: gitModuleGlobalHttpsProxyTextField
                Layout.fillWidth: true

                onAccepted: gitModuleProxyDialog.accept()
                Keys.onEscapePressed: gitModuleProxyDialog.reject()
            }
        }
    }

    Dialog {
        id: gitModuleBranchRenameDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Rename branch ") + gitModuleBranchRenameDialog.name
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string name

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            gitModuleBranchRenameTextField.text = gitModuleBranchRenameDialog.name
            gitModuleBranchRenameTextField.forceActiveFocus()
            gitModuleBranchRenameTextField.selectAll()
        }

        onAccepted: gitModule.gitRename(gitModuleBranchRenameDialog.name, gitModuleBranchRenameTextField.text)

        TextField {
            id: gitModuleBranchRenameTextField
            placeholderText: qsTr("Enter branch name:")
            width: parent.width

            onAccepted: gitModuleBranchRenameDialog.accept()
            Keys.onEscapePressed: gitModuleBranchRenameDialog.reject()
        }
    }

    Dialog {
        id: gitModuleRemoteAddDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Add Remote Repository")
        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            gitModuleRemoteAddTextField.text = ""
            gitModuleRemoteAddTextField.forceActiveFocus()
        }

        onAccepted: gitModule.gitRemoteAdd(gitModuleRemoteAddTextField.text)

        TextField {
            id: gitModuleRemoteAddTextField
            placeholderText: qsTr("Remote URL (e.g., https://github.com/user/repo.git)")
            width: parent.width

            onAccepted: gitModuleRemoteAddDialog.accept()
            Keys.onEscapePressed: gitModuleRemoteAddDialog.reject()
        }
    }

    Menu {
        id: gitModuleBranchMenu
        property url name
        property string type

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Switch")
            enabled: !["current", "upstream", "remote"].includes(gitModuleBranchMenu.type)
            icon.source: "qrc:/icon/arrowRight.svg"
            icon.width: 16; icon.height: 16

            onTriggered: gitModule.gitSwitch(gitModuleBranchMenu.name)
        }

        MenuItem {
            text: qsTr("Create")
            enabled: !["upstream", "remote"].includes(gitModuleBranchMenu.type)
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                gitModuleBranchCreateDialog.name = gitModuleBranchMenu.name
                gitModuleBranchCreateDialog.open()
            }
        }

        MenuItem {
            text: qsTr("Rename")
            enabled: !["upstream", "remote"].includes(gitModuleBranchMenu.type)
            icon.source: "qrc:/icon/rename.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                gitModuleBranchRenameDialog.name = gitModuleBranchMenu.name
                gitModuleBranchRenameDialog.open()
            }
        }

        Menu {
            title: qsTr("Delete")
            enabled: !["upstream", "remote"].includes(gitModuleBranchMenu.type)
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    gitModule.gitDelete(gitModuleBranchMenu.name)
                    progress = 0
                    gitModuleBranchMenu.close()
                }
            }
        }

        MenuSeparator {
        }

        MenuItem {
            text: gitModuleBranchMenu.type === "upstream" ? qsTr("Unset Upstream") : qsTr("Set Upstream")
            enabled: global.gitStatus === 0 && ["upstream", "remote"].includes(gitModuleBranchMenu.type)
            icon.source: "qrc:/icon/link.svg"
            icon.width: 16; icon.height: 16

            onTriggered: gitModuleBranchMenu.type === "upstream" ? gitModule.gitUpstreamUnset() : gitModule.gitUpstreamSet(gitModuleBranchMenu.name)
        }

        MenuItem {
            text: qsTr("Merge")
            enabled: global.gitStatus === 0
            icon.source: "qrc:/icon/gitMerge.svg"
            icon.width: 16; icon.height: 16

            onTriggered: gitModule.gitMerge(gitModuleBranchMenu.name)
        }

        MenuItem {
            text: qsTr("Rebase")
            enabled: global.gitStatus === 0
            icon.source: "qrc:/icon/gitMerge.svg"
            icon.width: 16; icon.height: 16

            onTriggered: gitModule.gitRebase(gitModuleBranchMenu.name)
        }
    }

    Menu {
        id: gitModuleLogMenu
        property string hash

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Cherry-pick")
            enabled: global.gitStatus === 0
            icon.source: "qrc:/icon/gitMerge.svg"
            icon.width: 16; icon.height: 16

            onTriggered: gitModule.gitCherryPick(gitModuleLogMenu.hash)
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Reset")
            icon.source: "qrc:/icon/reset.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Mixed")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainWindowToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainWindowToolTip.position = parent.mapToGlobal(point.position)
                        mainWindowToolTip.text = qsTr("Leave your working directory unchanged.")
                    }
                }

                onTriggered: gitModule.gitReset(gitModuleLogMenu.hash, 0)
            }

            MenuItem {
                text: qsTr("Soft")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainWindowToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainWindowToolTip.position = parent.mapToGlobal(point.position)
                        mainWindowToolTip.text = qsTr("Leave your working tree files and the index unchanged.")
                    }
                }

                onTriggered: gitModule.gitReset(gitModuleLogMenu.hash, 1)
            }

            MenuItem {
                text: qsTr("Hard")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainWindowToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainWindowToolTip.position = parent.mapToGlobal(point.position)
                        mainWindowToolTip.text = qsTr("Overwrite all files and directories with the version from commit, and may overwrite untracked files.")
                    }
                }

                onTriggered: gitModule.gitReset(gitModuleLogMenu.hash, 2)
            }

            MenuItem {
                text: qsTr("Merge")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainWindowToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainWindowToolTip.position = parent.mapToGlobal(point.position)
                        mainWindowToolTip.text = qsTr("Reset the index and update the files in the working tree that are different between commit and HEAD, but keep those which are different between the index and working tree.")
                    }
                }

                onTriggered: gitModule.gitReset(gitModuleLogMenu.hash, 3)
            }

            MenuItem {
                text: qsTr("Keep")

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainWindowToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainWindowToolTip.position = parent.mapToGlobal(point.position)
                        mainWindowToolTip.text = qsTr("Resets index entries and updates files in the working tree that are different between commit and HEAD.")
                    }
                }

                onTriggered: gitModule.gitReset(gitModuleLogMenu.hash, 4)
            }
        }
    }

    // log module
    Dialog {
        id: logModuleHeightDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Set Max Line Count")
        standardButtons: Dialog.Ok | Dialog.Cancel

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            logModuleHeightSpinBox.value = logModule.heightGet()
            logModuleHeightSpinBox.forceActiveFocus()
        }
        onAccepted: logModule.heightSet(logModuleHeightSpinBox.value)

        SpinBox {
            id: logModuleHeightSpinBox
            width: parent.width
            editable: true
            from: 1000
            to: 10000
            stepSize: 1000

            Keys.onReturnPressed: logModuleHeightDialog.accept()
            Keys.onEnterPressed: logModuleHeightDialog.accept()
            Keys.onEscapePressed: logModuleHeightDialog.reject()
        }
    }

    // menu module
    Menu {
        id: menuModuleFileMenu
        implicitWidth: 300

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            id: menuModuleOpenFileItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/document.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.fore
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

            onTriggered: menuModuleOpenFileItemDialog.open()

            FileDialog {
                id: menuModuleOpenFileItemDialog
                fileMode: FileDialog.OpenFile
                nameFilters: ["All files (*)"]
                onAccepted: documentModule.documentOpen(selectedFile)
            }
        }

        MenuItem {
            id: menuModuleOpenWorkspaceItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/home.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.fore
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

            Shortcut {
                sequence: "Ctrl+O"
                onActivated: menuModuleOpenWorkspaceItem.triggered()
            }

            onTriggered: Qt.callLater(() => mainWindow.workspaceOpen())
        }

        MenuItem {
            id: menuModuleSaveWorkspaceItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/save.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.fore
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

            Shortcut {
                sequence: "Ctrl+S"
                onActivated: menuModuleSaveWorkspaceItem.triggered()
            }

            onTriggered: mainWindow.workspaceSave("")
        }

        MenuItem {
            id: menuModuleSaveWorkspaceAsItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/saveAs.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.fore
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

            Shortcut {
                sequence: "Ctrl+Shift+S"
                onActivated: menuModuleSaveWorkspaceAsItem.triggered()
            }

            onTriggered: menuModuleSaveDialog.open()

            FileDialog {
                id: menuModuleSaveDialog
                currentFolder: StandardPaths.standardLocations(StandardPaths.DesktopLocation)[0]
                fileMode: FileDialog.SaveFile
                nameFilters: ["Json (*.json)"]
                currentFile: currentFolder + "/config.json"
                onAccepted: mainWindow.workspaceSave(selectedFile)
            }
        }
    }

    Menu {
        id: menuModuleEditMenu
        implicitWidth: 300
        property var menuSession
        readonly property int historyMenuMaximumHeight: 400

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: menuModuleEditMenu.menuSession = documentModule.menuLoad("edit")

        Menu {
            id: menuModuleEditUndoMenu
            title: qsTr("Undo")
            enabled: undoModule.canUndo
            implicitWidth: 300
            implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                                     menuModuleEditMenu.historyMenuMaximumHeight)
            icon.source: "qrc:/icon/undo.svg"
            icon.width: 16; icon.height: 16

            onAboutToShow: menuModuleEditUndoInstantiator.model = undoModule.undoHistory()

            Instantiator {
                id: menuModuleEditUndoInstantiator
                model: []
                delegate: MenuItem {
                    required property var modelData
                    text: qsTr("%1. %2").arg(modelData.steps).arg(modelData.text)
                    onTriggered: undoModule.undoTo(modelData.targetIndex)
                }

                onObjectAdded: (index, object) => menuModuleEditUndoMenu.addItem(object)
                onObjectRemoved: (index, object) => menuModuleEditUndoMenu.removeItem(object)
            }
        }

        Menu {
            id: menuModuleEditRedoMenu
            title: qsTr("Redo")
            enabled: undoModule.canRedo
            implicitWidth: 300
            implicitHeight: Math.min(contentItem.implicitHeight + topPadding + bottomPadding,
                                     menuModuleEditMenu.historyMenuMaximumHeight)
            icon.source: "qrc:/icon/redo.svg"
            icon.width: 16; icon.height: 16

            onAboutToShow: menuModuleEditRedoInstantiator.model = undoModule.redoHistory()

            Instantiator {
                id: menuModuleEditRedoInstantiator
                model: []
                delegate: MenuItem {
                    required property var modelData
                    text: qsTr("%1. %2").arg(modelData.steps).arg(modelData.text)
                    onTriggered: undoModule.redoTo(modelData.targetIndex)
                }

                onObjectAdded: (index, object) => menuModuleEditRedoMenu.addItem(object)
                onObjectRemoved: (index, object) => menuModuleEditRedoMenu.removeItem(object)
            }
        }

        MenuSeparator {
        }

        MenuItem {
            id: menuModuleEditCutItem
            enabled: menuModuleEditMenu.menuSession ? menuModuleEditMenu.menuSession.copiable : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/cut.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleEditCutItem.enabled ? global.fore : global.foreDisabled
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

            onTriggered: documentModule.menuCall("cut")
        }

        MenuItem {
            id: menuModuleEditCopyItem
            enabled: menuModuleEditMenu.menuSession ? menuModuleEditMenu.menuSession.copiable : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/copy.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleEditCopyItem.enabled ? global.fore : global.foreDisabled
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

            onTriggered: documentModule.menuCall("copy")
        }

        MenuItem {
            id: menuModuleEditPasteItem
            enabled: menuModuleEditMenu.menuSession ? menuModuleEditMenu.menuSession.pastable : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/paste.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleEditPasteItem.enabled ? global.fore : global.foreDisabled
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

            onTriggered: documentModule.menuCall("paste")
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
                    color: global.fore
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

            onTriggered: documentModule.menuCall("search")
        }

        MenuItem {
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/replace.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.fore
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

            onTriggered: documentModule.menuCall("replace")
        }
    }

    Menu {
        id: menuModuleViewMenu

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Explorer")
            checkable: true
            checked: explorerModuleAction ? explorerModuleAction.checked : false

            onTriggered: explorerModuleAction.toggle()
        }

        MenuItem {
            text: qsTr("Port")
            checkable: true
            checked: portModuleAction ? portModuleAction.checked : false

            onTriggered: portModuleAction.toggle()
        }

        MenuItem {
            text: qsTr("Threadpool")
            checkable: true
            checked: threadpoolModuleAction ? threadpoolModuleAction.checked : false

            onTriggered: threadpoolModuleAction.toggle()
        }

        MenuSeparator {
        }

        MenuItem {
            text: qsTr("Diagnostics")
            checkable: true
            checked: diagnosticsModuleAction ? diagnosticsModuleAction.checked : false

            onTriggered: diagnosticsModuleAction.toggle()
        }

        MenuItem {
            text: qsTr("Structure")
            checkable: true
            checked: structureModuleAction ? structureModuleAction.checked : false

            onTriggered: structureModuleAction.toggle()
        }

        MenuSeparator {
        }

        MenuItem {
            text: qsTr("Database")
            checkable: true
            checked: databaseModuleAction ? databaseModuleAction.checked : false

            onTriggered: databaseModuleAction.toggle()
        }

        MenuItem {
            text: qsTr("Datatable")
            checkable: true
            checked: datatableModuleAction ? datatableModuleAction.checked : false

            onTriggered: datatableModuleAction.toggle()
        }

        MenuItem {
            text: qsTr("Dataplot")
            checkable: true
            checked: dataplotModuleAction ? dataplotModuleAction.checked : false

            onTriggered: dataplotModuleAction.toggle()
        }

        MenuSeparator {
        }

        MenuItem {
            text: qsTr("Breakpoint")
            checkable: true
            checked: breakpointModuleAction ? breakpointModuleAction.checked : false

            onTriggered: breakpointModuleAction.toggle()
        }

        MenuItem {
            text: qsTr("Debug")
            checkable: true
            checked: debugModuleAction ? debugModuleAction.checked : false

            onTriggered: debugModuleAction.toggle()
        }

        MenuItem {
            text: qsTr("Watch")
            checkable: true
            checked: watchModuleAction ? watchModuleAction.checked : false

            onTriggered: watchModuleAction.toggle()
        }

        MenuSeparator {
        }

        MenuItem {
            text: qsTr("Log")
            checkable: true
            checked: logModuleAction ? logModuleAction.checked : false

            onTriggered: logModuleAction.toggle()
        }

        MenuItem {
            text: qsTr("Git")
            checkable: true
            checked: gitModuleAction ? gitModuleAction.checked : false

            onTriggered: gitModuleAction.toggle()
        }

        Menu {
            id: terminalModuleTerminalMenu
            title: qsTr("Terminal")
            property var terminalModel

            onOpened: {
                mainWindow.overlayFlagSet(false, true)
                widgetCount += 1
            }
            onClosed: widgetCount -= 1

            MenuItem {
                text: qsTr("Manage")

                onTriggered: terminalModule.terminalManage()
            }

            MenuSeparator {
            }

            Instantiator {
                id: terminalModuleTerminalInstantiator
                model: terminalModuleTerminalMenu.terminalModel
                delegate: MenuItem {
                    text: model.session.name
                    onTriggered: terminalModule.terminalOpen(model.id)
                }

                onObjectAdded: (index, object) => terminalModuleTerminalMenu.insertItem(index + 2, object)
                onObjectRemoved: (index, object) => terminalModuleTerminalMenu.removeItem(object)
            }
        }

        MenuSeparator {
        }

        MenuItem {
            text: qsTr("Agent")
            checkable: true
            checked: agentModuleAction ? agentModuleAction.checked : false

            onTriggered: agentModuleAction.toggle()
        }
    }

    Menu {
        id: menuModuleNavMenu
        implicitWidth: 300
        property var menuSession

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: menuModuleNavMenu.menuSession = documentModule.menuLoad("nav")

        MenuItem {
            id: menuModuleNavPrevItem
            enabled: menuModuleNavMenu.menuSession ? menuModuleNavMenu.menuSession.prev : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/arrowLeft.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleNavPrevItem.enabled ? global.fore : global.foreDisabled
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

            Shortcut {
                sequence: "Alt+Left"
                onActivated: menuModuleNavPrevItem.triggered()
            }

            onTriggered: documentModule.navigationPrev()
        }

        MenuItem {
            id: menuModuleNavNextItem
            enabled: menuModuleNavMenu.menuSession ? menuModuleNavMenu.menuSession.next : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/arrowRight.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleNavNextItem.enabled ? global.fore : global.foreDisabled
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

            Shortcut {
                sequence: "Alt+Right"
                onActivated: menuModuleNavNextItem.triggered()
            }

            onTriggered: documentModule.navigationNext()
        }

        MenuSeparator {
        }

        MenuItem {
            enabled: menuModuleNavMenu.menuSession ? menuModuleNavMenu.menuSession.navigation : false

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

            onTriggered: documentModule.definitionRequest(menuModuleNavMenu.menuSession.documentUrl, menuModuleNavMenu.menuSession.line, menuModuleNavMenu.menuSession.character)
        }

        MenuItem {
            enabled: menuModuleNavMenu.menuSession ? menuModuleNavMenu.menuSession.navigation : false

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

            onTriggered: documentModule.referencesRequest(menuModuleNavMenu.menuSession.documentUrl, menuModuleNavMenu.menuSession.line, menuModuleNavMenu.menuSession.character)
        }

        MenuItem {
            enabled: menuModuleNavMenu.menuSession ? menuModuleNavMenu.menuSession.navigation : false

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

            onTriggered: documentModule.implementationRequest(menuModuleNavMenu.menuSession.documentUrl, menuModuleNavMenu.menuSession.line, menuModuleNavMenu.menuSession.character)
        }

        MenuItem {
            enabled: menuModuleNavMenu.menuSession ? menuModuleNavMenu.menuSession.navigation : false

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

            onTriggered: documentModule.typeDefinitionRequest(menuModuleNavMenu.menuSession.documentUrl, menuModuleNavMenu.menuSession.line, menuModuleNavMenu.menuSession.character)
        }
    }

    Menu {
        id: menuModuleCodeMenu
        implicitWidth: 300
        property var menuSession

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: menuModuleCodeMenu.menuSession = documentModule.menuLoad("code")

        MenuItem {
            id: menuModuleCodeCompletionItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/completion.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.fore
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

            Shortcut {
                sequence: "Ctrl+Space"
                onActivated: {
                    menuModuleCodeMenu.menuSession = documentModule.menuLoad("code")
                    menuModuleCodeCompletionItem.triggered()
                }
            }

            onTriggered: documentModule.completionRequest(menuModuleCodeMenu.menuSession.documentUrl, menuModuleCodeMenu.menuSession.line, menuModuleCodeMenu.menuSession.character)
        }

        MenuItem {
            id: menuModuleCodeReformatItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/brush.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.fore
                }

                Label {
                    text: menuModuleCodeMenu.menuSession ? menuModuleCodeMenu.menuSession.text ? qsTr("Reformat Selected") : qsTr("Reformat") : false
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Ctrl+Alt+L"
                }
            }

            Shortcut {
                sequence: "Ctrl+Alt+L"
                onActivated: {
                    menuModuleCodeMenu.menuSession = documentModule.menuLoad("code")
                    menuModuleCodeReformatItem.triggered()
                }
            }

            onTriggered: {
                if (menuModuleCodeMenu.menuSession.text) {
                    documentModule.rangeFormattingRequest(menuModuleCodeMenu.menuSession.documentUrl, menuModuleCodeMenu.menuSession.startLine, menuModuleCodeMenu.menuSession.startCharacter, menuModuleCodeMenu.menuSession.endLine, menuModuleCodeMenu.menuSession.endCharacter)
                } else {
                    documentModule.formattingRequest(menuModuleCodeMenu.menuSession.documentUrl)
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

                onTriggered: documentModule.foldContractTop(menuModuleCodeMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Contract Recursively")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: documentModule.foldContractRecursively(menuModuleCodeMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Expand Recursively")
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16

                onTriggered: documentModule.foldExpandRecursively(menuModuleCodeMenu.menuSession.documentUrl)
            }
        }
    }

    Menu {
        id: menuModuleExecMenu
        implicitWidth: 300
        property var menuSession

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: menuModuleExecMenu.menuSession = documentModule.menuLoad("exec")

        MenuItem {
            id: menuModuleExecRunItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/play.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.successFore3
                }

                Label {
                    text: qsTr("Run ") + (menuModuleExecMenu.menuSession ? menuModuleExecMenu.menuSession.text ? "Selected" : menuModuleExecMenu.menuSession.documentName : "")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Shift+F10"
                }
            }

            Shortcut {
                sequence: "Shift+F10"
                onActivated: {
                    menuModuleExecMenu.menuSession = documentModule.menuLoad("exec")
                    menuModuleExecRunItem.triggered()
                }
            }

            onTriggered: {
                if (menuModuleExecMenu.menuSession.text) {
                    threadpoolModule.threadStart(menuModuleExecMenu.menuSession.documentUrl, 0, menuModuleExecMenu.menuSession.startLine, menuModuleExecMenu.menuSession.startCharacter, menuModuleExecMenu.menuSession.endLine, menuModuleExecMenu.menuSession.endCharacter)
                } else {
                    threadpoolModule.threadStart(menuModuleExecMenu.menuSession.documentUrl, 0)
                }
            }
        }

        MenuItem {
            id: menuModuleExecDebugItem

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/bug.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: global.successFore3
                }

                Label {
                    text: qsTr("Debug ") + (menuModuleExecMenu.menuSession ? menuModuleExecMenu.menuSession.documentName : "")
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: "Shift+F9"
                }
            }

            Shortcut {
                sequence: "Shift+F9"
                onActivated: {
                    menuModuleExecMenu.menuSession = documentModule.menuLoad("exec")
                    menuModuleExecDebugItem.triggered()
                }
            }

            onTriggered: threadpoolModule.threadStart(menuModuleExecMenu.menuSession.documentUrl, 1)
        }
    }

    Menu {
        id: menuModuleGitMenu

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            id: menuModuleFetchItem
            enabled: global.gitStatus === 0
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/gitFetch.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleFetchItem.enabled ? global.fore : global.foreDisabled
                }

                Label {
                    text: qsTr("Fetch")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: gitModule.gitFetch()
        }

        MenuItem {
            id: menuModuleCommitItem
            enabled: global.gitStatus === 0
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/gitCommit.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleCommitItem.enabled ? global.fore : global.foreDisabled
                }

                Label {
                    text: qsTr("Commit")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: gitModule.gitCommitPre()
        }

        MenuItem {
            id: menuModulePushItem
            enabled: global.gitStatus === 0
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/gitPush.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModulePushItem.enabled ? global.fore : global.foreDisabled
                }

                Label {
                    text: qsTr("Push")
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            onTriggered: gitModule.gitPushPre()
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Config")
            icon.source: "qrc:/icon/settings.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Proxy")

                onTriggered: gitModule.gitProxyGet()
            }
        }
    }

    // port module
    Menu {
        id: portModuleTableMenu
        property int portIndex
        property string portName

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            onTriggered: portModule.portSetting()
        }

        MenuItem {
            text: qsTr("Edit")
            icon.source: "qrc:/icon/edit.svg"
            icon.width: 16; icon.height: 16

            onTriggered: portModule.portSetting(portModuleTableMenu.portIndex)
        }

        Menu {
            title: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    portModule.portRemove(portModuleTableMenu.portName)
                    progress = 0
                    portModuleTableMenu.close()
                }
            }
        }
    }

    Menu {
        id: portModuleRootMenu

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16

            onTriggered: portModule.portSetting()
        }
    }

    // status module
    Menu {
        id: statusModuleEolModeMenu
        property var eolModeButton
        property url documentUrl
        property int eolMode

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            statusModuleEolModeMenu.eolMode = documentModule.eolModeGet(documentUrl)
            statusModuleEolModeViewItem.checked = documentModule.eolViewGet(documentUrl)
        }

        MenuItem {
            text: "CRLF"
            enabled: statusModuleEolModeMenu.eolMode !== 0
            onTriggered: {
                documentModule.eolModeSet(statusModuleEolModeMenu.documentUrl, 0)
                statusModuleEolModeMenu.eolModeButton.text = "CRLF"
            }
        }

        MenuItem {
            text: "CR"
            enabled: statusModuleEolModeMenu.eolMode !== 1
            onTriggered: {
                documentModule.eolModeSet(statusModuleEolModeMenu.documentUrl, 1)
                statusModuleEolModeMenu.eolModeButton.text = "CR"
            }
        }

        MenuItem {
            text: "LF"
            enabled: statusModuleEolModeMenu.eolMode !== 2
            onTriggered: {
                documentModule.eolModeSet(statusModuleEolModeMenu.documentUrl, 2)
                statusModuleEolModeMenu.eolModeButton.text = "LF"
            }
        }

        MenuSeparator {
        }

        MenuItem {
            id: statusModuleEolModeViewItem
            checkable: true
            text: qsTr("View EOL")

            onTriggered: documentModule.eolViewSet(statusModuleEolModeMenu.documentUrl, statusModuleEolModeViewItem.checked)
        }
    }

    ToolTip {
        id: statusModuleBackgroundToolTip
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        property point position
        property var backgroundModel

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            x = position.x
            y = position.y
        }

        contentItem: TableView {
            id: statusModuleBackgroundTableView
            anchors.fill: parent
            anchors.margins: 6
            implicitWidth: 400; implicitHeight: 300
            alternatingRows: false
            clip: true
            editTriggers: TableView.NoEditTriggers
            rowSpacing: 1
            model: statusModuleBackgroundToolTip.backgroundModel
            contentWidth: width

            ScrollBar.vertical: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.stroke
                    dark: global.strokePressed
                }
            }

            Rectangle {
                anchors.fill: parent
                color: global.stroke
            }

            delegate: Item {
                implicitWidth: statusModuleBackgroundTableView.width
                implicitHeight: 48

                Rectangle {
                    anchors.fill: parent
                    color: global.back
                }

                ColumnLayout {
                    anchors.fill: parent

                    RowLayout {
                        Layout.fillWidth: true; Layout.fillHeight: true

                        Label {
                            leftPadding: 6
                            horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                            text: model.display || ""
                            elide: Text.ElideRight
                            Layout.fillWidth: true; Layout.preferredHeight: 24
                        }

                        Button {
                            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                            flat: true
                            icon.source: "qrc:/icon/dismiss.svg"
                            icon.width: 16; icon.height: 16
                            Layout.preferredWidth: 24; Layout.preferredHeight: 24

                            onClicked: {
                                const close = statusModuleBackgroundToolTip.backgroundModel.taskCount <= 2
                                statusModule.backgroundAbort(model.taskId)
                                if (close) {
                                    statusModuleBackgroundToolTip.close()
                                }
                            }
                        }
                    }

                    ProgressBar {
                        indeterminate: true
                        Layout.fillWidth: true; Layout.preferredHeight: 12
                    }
                }
            }
        }
    }

    // structure module
    Menu {
        id: structureModuleRootMenu
        property var treeView

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        Menu {
            title: qsTr("Folding")
            icon.source: "qrc:/icon/fold.svg"
            icon.width: 16; icon.height: 16

            MenuItem {
                text: qsTr("Collapse All")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: structureModuleRootMenu.treeView.collapseRecursively()
            }

            MenuItem {
                text: qsTr("Expand All")
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16

                onTriggered: structureModuleRootMenu.treeView.expandRecursively()
            }
        }
    }

    // threadpool module
    Dialog {
        id: threadpoolModuleErrorDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Terminate Request Has Been Sent")
        standardButtons: Dialog.Ok

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
    }

    Menu {
        id: threadpoolModuleThreadMenu
        property string threadId

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Terminate")
            icon.source: "qrc:/icon/stop.svg"
            icon.width: 16; icon.height: 16

            onTriggered: threadpoolModule.threadStop(threadpoolModuleThreadMenu.threadId)
        }
    }

    // watch module
    Dialog {
        id: watchModuleExpressionDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Enter Watch")
        standardButtons: Dialog.Ok
        property int watchIndex
        property url watchUrl
        property string watchExpression

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            watchModuleUrlTextField.text = watchModuleExpressionDialog.watchUrl
            watchModuleExpressionTextField.text = watchModuleExpressionDialog.watchExpression
            watchModuleExpressionTextField.forceActiveFocus()
            watchModuleExpressionTextField.selectAll()
        }
        onAccepted: {
            if (watchModuleExpressionDialog.watchIndex === -1 || !watchModuleExpressionDialog.watchExpression) {
                watchModule.watchInsert(watchModuleExpressionDialog.watchIndex, watchModuleUrlTextField.text, watchModuleExpressionTextField.text)
            } else {
                watchModule.watchRename(watchModuleExpressionDialog.watchIndex, watchModuleUrlTextField.text, watchModuleExpressionTextField.text)
            }
        }

        ColumnLayout {
            width: parent.width

            Label {
                text: qsTr("Url")
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }

            TextField {
                id: watchModuleUrlTextField
                width: parent.width
                placeholderText: qsTr("Enter url:")
                Layout.fillWidth: true

                onAccepted: watchModuleExpressionDialog.accept()
                Keys.onEscapePressed: watchModuleExpressionDialog.reject()
            }

            Label {
                text: qsTr("Expression")
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }

            TextField {
                id: watchModuleExpressionTextField
                width: parent.width
                placeholderText: qsTr("Enter expression:")
                Layout.fillWidth: true

                onAccepted: watchModuleExpressionDialog.accept()
                Keys.onEscapePressed: watchModuleExpressionDialog.reject()
            }
        }
    }

    Dialog {
        id: watchModuleValueDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: qsTr("Edit Value")
        standardButtons: Dialog.Ok
        property string currentThread
        property url watchUrl
        property string watchExpression
        property string currentValue
        property string currentType

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            watchModuleValueTextField.text = watchModuleValueDialog.currentValue
            watchModuleValueTextField.forceActiveFocus()
            watchModuleValueTextField.selectAll()
            watchModuleValueComboBox.currentValue = watchModuleValueDialog.currentType
        }
        onAccepted: threadpoolModule.valueSet(watchModuleValueDialog.currentThread, watchModuleValueDialog.watchUrl, watchModuleValueDialog.watchExpression, watchModuleValueTextField.text, watchModuleValueComboBox.currentValue)

        ColumnLayout {
            width: parent.width

            Label {
                text: watchModuleValueDialog.currentThread
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }

            Label {
                text: watchModuleValueDialog.watchUrl
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }

            Label {
                text: watchModuleValueDialog.watchExpression
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
            }

            RowLayout {

                TextField {
                    id: watchModuleValueTextField
                    placeholderText: qsTr("Enter new value:")
                    Layout.fillWidth: true

                    onAccepted: watchModuleValueDialog.accept()
                    Keys.onEscapePressed: watchModuleValueDialog.reject()
                }

                ComboBox {
                    id: watchModuleValueComboBox
                    model: ListModel {
                        ListElement {
                            text: "boolean"; value: "boolean"
                        }
                        ListElement {
                            text: "number"; value: "number"
                        }
                        ListElement {
                            text: "string"; value: "string"
                        }
                    }
                    textRole: "text"
                    valueRole: "value"
                }
            }
        }
    }

    Menu {
        id: watchModuleExpressionMenu
        property int watchIndex
        property url watchUrl
        property string watchExpression

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Rename")
            icon.source: "qrc:/icon/rename.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                watchModuleExpressionDialog.watchIndex = watchModuleExpressionMenu.watchIndex
                watchModuleExpressionDialog.watchUrl = watchModuleExpressionMenu.watchUrl
                watchModuleExpressionDialog.watchExpression = watchModuleExpressionMenu.watchExpression
                watchModuleExpressionDialog.open()
            }
        }

        MenuItem {
            text: qsTr("Clear")
            icon.source: "qrc:/icon/eraser.svg"
            icon.width: 16; icon.height: 16

            onTriggered: watchModule.watchClear(watchModuleExpressionMenu.watchIndex)
        }

        MenuItem {
            text: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            onTriggered: watchModule.watchRemove(watchModuleExpressionMenu.watchIndex)
        }
    }

    Menu {
        id: watchModuleValueMenu
        property url watchUrl
        property string watchExpression
        property string currentValue
        property string currentType

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Hot Update")
            icon.source: "qrc:/icon/edit.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                if (!debugModule.threadGet()) {
                    debugModuleErrorDialog.open()
                } else {
                    watchModuleValueDialog.currentThread = debugModule.threadGet()
                    watchModuleValueDialog.watchUrl = watchModuleValueMenu.watchUrl
                    watchModuleValueDialog.watchExpression = watchModuleValueMenu.watchExpression
                    watchModuleValueDialog.currentValue = watchModuleValueMenu.currentValue
                    watchModuleValueDialog.currentType = watchModuleValueMenu.currentType
                    watchModuleValueDialog.open()
                }
            }
        }

        MenuItem {
            text: qsTr("Clear")
            icon.source: "qrc:/icon/eraser.svg"
            icon.width: 16; icon.height: 16

            onTriggered: watchModule.watchClear(watchModuleExpressionMenu.watchIndex)
        }

        MenuItem {
            text: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            onTriggered: watchModule.watchRemove(watchModuleExpressionMenu.watchIndex)
        }
    }

    Menu {
        id: watchModuleRootMenu

        onOpened: {
            widgetCount += 1
            mainWindow.overlayFlagSet(false, true)
        }
        onClosed: widgetCount -= 1

        Menu {
            title: qsTr("Clear")
            icon.source: "qrc:/icon/eraser.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    watchModule.watchClear(-1)
                    progress = 0
                    watchModuleRootMenu.close()
                }
            }
        }
    }

    Component.onCompleted: {
        const objects = {
            "mainItem": mainItem,
            "mainWindowCloseDialog": mainWindowCloseDialog,
            "mainWindowMessageDialog": mainWindowMessageDialog,
            "mainWindowQuitDialog": mainWindowQuitDialog,
            "mainWindowLinkMenu": mainWindowLinkMenu,
            "mainWindowTextView": mainWindowTextView,
            "mainWindowToolTip": mainWindowToolTip,

            "agentModuleRenameDialog": agentModuleRenameDialog,
            "agentModuleModeMenu": agentModuleModeMenu,
            "agentModuleModelMenu": agentModuleModelMenu,

            "breakpointModuleEditDialog": breakpointModuleEditDialog,
            "breakpointModuleLineMenu": breakpointModuleLineMenu,
            "breakpointModuleFileMenu": breakpointModuleFileMenu,
            "breakpointModuleRootMenu": breakpointModuleRootMenu,

            "databaseModuleTableMenu": databaseModuleTableMenu,
            "databaseModuleRootMenu": databaseModuleRootMenu,

            "datatableModuleTableMenu": datatableModuleTableMenu,
            "datatableModuleRootMenu": datatableModuleRootMenu,

            "dataplotModuleRootMenu": dataplotModuleRootMenu,

            "debugModuleErrorDialog": debugModuleErrorDialog,

            "diagnosticsModuleDiagnosticMenu": diagnosticsModuleDiagnosticMenu,

            "documentModuleGotoDialog": documentModuleGotoDialog,
            "documentModuleRenameDialog": documentModuleRenameDialog,
            "documentModuleSaveDialog": documentModuleSaveDialog,
            "documentModuleEditorMenu": documentModuleEditorMenu,
            "documentModuleCompletionToolTip": documentModuleCompletionToolTip,
            "documentModuleCompletionTableView": documentModuleCompletionTableView,
            "documentModuleCompletionDetailTableView": documentModuleCompletionDetailTableView,
            "documentModuleDwellToolTip": documentModuleDwellToolTip,
            "documentModuleDwellDiagnosticTextArea": documentModuleDwellDiagnosticTextArea,
            "documentModuleDwellHoverTextArea": documentModuleDwellHoverTextArea,
            "documentModuleDwellCodeActionMenu": documentModuleDwellCodeActionMenu,
            "documentModuleDwellSuggestionMenu": documentModuleDwellSuggestionMenu,
            "documentModuleNavigationToolTip": documentModuleNavigationToolTip,
            "documentModuleNavigationTableView": documentModuleNavigationTableView,
            "documentModuleNavigationDetailLabel": documentModuleNavigationDetailLabel,
            "documentModulePositionTooltip": documentModulePositionTooltip,
            "documentModuleSignatureToolTip": documentModuleSignatureToolTip,
            "documentModuleSignatureLabel": documentModuleSignatureLabel,

            "explorerModuleFileMenu": explorerModuleFileMenu,
            "explorerModuleFolderMenu": explorerModuleFolderMenu,
            "explorerModuleRootMenu": explorerModuleRootMenu,

            "fileModulePropertyDialog": fileModulePropertyDialog,

            "gitModuleContinueDialog": gitModuleContinueDialog,
            "gitModuleErrorDialog": gitModuleErrorDialog,
            "gitModuleProxyDialog": gitModuleProxyDialog,
            "gitModuleRemoteAddDialog": gitModuleRemoteAddDialog,
            "gitModuleBranchMenu": gitModuleBranchMenu,
            "gitModuleLogMenu": gitModuleLogMenu,

            "logModuleHeightDialog": logModuleHeightDialog,

            "menuModuleFileMenu": menuModuleFileMenu,
            "menuModuleEditMenu": menuModuleEditMenu,
            "menuModuleViewMenu": menuModuleViewMenu,
            "menuModuleNavMenu": menuModuleNavMenu,
            "menuModuleCodeMenu": menuModuleCodeMenu,
            "menuModuleExecMenu": menuModuleExecMenu,
            "menuModuleGitMenu": menuModuleGitMenu,

            "portModuleTableMenu": portModuleTableMenu,
            "portModuleRootMenu": portModuleRootMenu,

            "statusModuleEolModeMenu": statusModuleEolModeMenu,
            "statusModuleBackgroundToolTip": statusModuleBackgroundToolTip,

            "structureModuleRootMenu": structureModuleRootMenu,

            "terminalModuleTerminalMenu": terminalModuleTerminalMenu,

            "threadpoolModuleErrorDialog": threadpoolModuleErrorDialog,
            "threadpoolModuleThreadMenu": threadpoolModuleThreadMenu,

            "watchModuleExpressionMenu": watchModuleExpressionMenu,
            "watchModuleValueMenu": watchModuleValueMenu,
            "watchModuleRootMenu": watchModuleRootMenu,
        };
        mainWindow.propertyGet(objects)
    }
}
