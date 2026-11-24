import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.FluentWinUI3.impl
import QtQuick.Layouts
import QtQml

import UniComm

Item {
    id: root
    required property MainWindow mainWindow
    anchors.fill: parent

    // overlay control
    property int widgetCount: 0
    property rect mainGeometry: Qt.rect(0, 0, width, height)

    Shortcut {
        sequence: "F11"
        context: Qt.ApplicationShortcut
        autoRepeat: false
        onActivated: root.mainWindow.maximizeToggle()
    }

    Shortcut {
        sequence: "F12"
        context: Qt.ApplicationShortcut
        autoRepeat: false
        onActivated: debugConsole.visible = !debugConsole.visible
    }

    Rectangle {
        anchors.fill: parent
        visible: debugConsole.visible
        color: "transparent"
        border.color: global.theme.general.warningFore3
        border.width: 2
    }

    Item {
        id: mainScreen
        x: mainGeometry.x
        y: mainGeometry.y
        width: mainGeometry.width
        height: mainGeometry.height

        Rectangle {
            id: debugConsole
            anchors.fill: parent
            anchors.margins: 2
            visible: false
            color: "transparent"
            border.color: global.theme.general.brandStroke
            border.width: 2
            readonly property int windowFlags: root.Window.window ? root.Window.window.flags : 0

            Rectangle {
                x: 8; y: 8
                width: stats.implicitWidth + 16
                height: stats.implicitHeight + 12
                color: global.theme.general.back

                RowLayout {
                    id: stats
                    anchors.centerIn: parent
                    spacing: 12

                    ColumnLayout {
                        spacing: 0

                        Label {
                            text: "Popups"
                            color: global.theme.general.fore
                            Layout.alignment: Qt.AlignRight
                        }
                        Label {
                            text: "Input"
                            color: global.theme.general.fore
                            Layout.alignment: Qt.AlignRight
                        }
                        Label {
                            text: "Focus"
                            color: global.theme.general.fore
                            Layout.alignment: Qt.AlignRight
                        }
                    }

                    ColumnLayout {
                        spacing: 0

                        Label {
                            text: root.widgetCount
                            color: global.theme.general.fore
                        }
                        Label {
                            text: (debugConsole.windowFlags & Qt.WindowTransparentForInput) ? "Pass-through" : "Interactive"
                            color: global.theme.general.fore
                        }
                        Label {
                            text: (debugConsole.windowFlags & Qt.WindowDoesNotAcceptFocus) ? "Blocked" : "Allowed"
                            color: global.theme.general.fore
                        }
                    }
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
            root.mainWindow.overlayFlagSet(true, false)
        }
    }

    // main window
    Dialog {
        id: mainWindowCloseDialog
        parent: mainScreen
        anchors.centerIn: parent
        width: 600
        modal: true
        title: qsTr("Save and Exit?")
        standardButtons: Dialog.Yes | Dialog.No

        onOpened: {
            root.mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAccepted: root.mainWindow.quit()
    }

    Dialog {
        id: mainWindowMessageDialog
        parent: mainScreen
        anchors.centerIn: parent
        width: 600
        modal: true
        standardButtons: Dialog.Ok
        property string text
        topPadding: 30; bottomPadding: 20

        onOpened: {
            root.mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        Label {
            text: mainWindowMessageDialog.text
        }
    }

    Dialog {
        id: mainWindowQuitDialog
        parent: mainScreen
        anchors.centerIn: parent
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
            root.mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onRejected: root.mainWindow.terminate()

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
            root.mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1

        MenuItem {
            text: qsTr("Copy")
            icon.source: "qrc:/icon/copy.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.copyToClipboard(String(mainWindowLinkMenu.url))
        }

        MenuItem {
            text: qsTr("Open")
            icon.source: "qrc:/icon/link.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.documentModule.documentOpen(mainWindowLinkMenu.url)
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
            root.mainWindow.overlayFlagSet(false, true)
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
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
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
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
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
                duration: AnimationDuration.Follow
                easing.type: Easing.OutCubic
            }
        }

        Behavior on height {
            enabled: mainWindowToolTip.visible
            NumberAnimation {
                duration: AnimationDuration.Follow
                easing.type: Easing.OutCubic
            }
        }

        Behavior on x {
            enabled: mainWindowToolTip.visible
            SmoothedAnimation {
                duration: AnimationDuration.Follow
                velocity: -1
            }
        }

        Behavior on y {
            enabled: mainWindowToolTip.visible
            SmoothedAnimation {
                duration: AnimationDuration.Follow
                velocity: -1
            }
        }
    }

    // agent module
    Dialog {
        id: agentModuleRenameDialog
        parent: mainScreen
        anchors.centerIn: parent
        width: 600
        modal: true
        title: qsTr("Rename Conversation")
        standardButtons: Dialog.Ok
        property string oldTitle

        onOpened: {
            root.mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            agentModuleRenameTextField.text = agentModuleRenameDialog.oldTitle
            agentModuleRenameTextField.forceActiveFocus()
            agentModuleRenameTextField.selectAll()
        }
        onAccepted: global.agentModule.conversationRename(agentModuleRenameTextField.text)

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
            root.mainWindow.overlayFlagSet(false, true)
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
                        color: modeItem.index === 3 ? global.theme.general.warningFore3 : global.theme.general.fore
                        source: modeItem.modelData.icon
                        sourceSize.width: 18; sourceSize.height: 18
                        Layout.preferredWidth: 20; Layout.preferredHeight: 20
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true

                        Label {
                            text: modeItem.modelData.title
                            color: modeItem.index === 3 ? global.theme.general.warningFore3 : global.theme.general.fore
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Label {
                            text: modeItem.modelData.description
                            color: modeItem.index === 3 ? global.theme.general.warningFore3 : global.theme.general.stroke
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    IconImage {
                        visible: agentModuleModeMenu.selectedIndex === modeItem.index
                        color: modeItem.index === 3 ? global.theme.general.warningFore3 : global.theme.general.fore
                        source: "qrc:/icon/checkmark.svg"
                        sourceSize.width: 16; sourceSize.height: 16
                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    }
                }

                onTriggered: global.agentModule.conversationModeSet(index)
            }

            onObjectAdded: (index, object) => agentModuleModeMenu.addItem(object)
            onObjectRemoved: (index, object) => agentModuleModeMenu.removeItem(object)
        }
    }

    Menu {
        id: agentModuleModelMenu
        property var providerModel

        onOpened: {
            root.mainWindow.overlayFlagSet(false, true)
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
                        onTriggered: global.agentModule.conversationModelSet(providerMenu.providerId, model.id)
                    }

                    onObjectAdded: (index, object) => providerMenu.addItem(object)
                    onObjectRemoved: (index, object) => providerMenu.removeItem(object)
                }
            }

            onObjectAdded: (index, object) => agentModuleModelMenu.insertMenu(index, object)
            onObjectRemoved: (index, object) => agentModuleModelMenu.removeMenu(object)
        }
    }

    // document module
    ToolTip {
        id: documentModuleDwellToolTip
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        property point position
        property var dwellWidget
        property var codeActions
        property var suggestions

        onOpened: {
            root.mainWindow.overlayFlagSet(false, false)
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
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
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
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
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
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
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
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
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
                                global.documentModule.documentOpen(documentModuleDwellHoverTextArea.hoveredLink)
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

    Component.onCompleted: {
        const objects = {
            "mainOverlay": root,
            "mainScreen": mainScreen,
            "mainWindowCloseDialog": mainWindowCloseDialog,
            "mainWindowMessageDialog": mainWindowMessageDialog,
            "mainWindowQuitDialog": mainWindowQuitDialog,
            "mainWindowLinkMenu": mainWindowLinkMenu,
            "mainWindowTextView": mainWindowTextView,
            "mainWindowToolTip": mainWindowToolTip,

            "agentModuleRenameDialog": agentModuleRenameDialog,
            "agentModuleModeMenu": agentModuleModeMenu,
            "agentModuleModelMenu": agentModuleModelMenu,

            "documentModuleDwellToolTip": documentModuleDwellToolTip,
            "documentModuleDwellDiagnosticTextArea": documentModuleDwellDiagnosticTextArea,
            "documentModuleDwellHoverTextArea": documentModuleDwellHoverTextArea,
            "documentModuleDwellCodeActionMenu": documentModuleDwellCodeActionMenu,
            "documentModuleDwellSuggestionMenu": documentModuleDwellSuggestionMenu
        };
        root.mainWindow.propertyGet(objects)
    }
}
