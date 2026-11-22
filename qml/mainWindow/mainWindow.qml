import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.FluentWinUI3.impl
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQml

import UniComm

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
                border.color: global.theme.general.dangerFore3
                border.width: 3
            }

            ColumnLayout {
                anchors.centerIn: parent

                Label {
                    text: Application.styleHints.colorScheme === Qt.Light ? "Light" : "Dark"
                    color: global.theme.general.dangerFore3
                    font.pointSize: 16
                }

                Label {
                    id: debugWidgetCount
                    color: global.theme.general.dangerFore3
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

    // document module
    ToolTip {
        id: documentModuleCompletionToolTip
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        padding: 6
        property point position
        property var completionWidget
        readonly property real rowHeight: completionWidget ? completionWidget.rowHeight : 24

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: {
            widgetCount -= 1
            documentModuleCompletionDetailTimer.stop()
            documentModuleCompletionDetailToolTip.close()
            completionWidget.completionHide()
        }
        onAboutToShow: {
            position = Overlay.overlay.mapFromGlobal(position)
            x = position.x - leftPadding - 11 - 24
            y = position.y
            documentModuleCompletionDetailToolTip.open()
            documentModuleCompletionDetailTimer.restart()
        }

        contentItem: ListView {
            id: documentModuleCompletionListView
            model: documentModuleCompletionToolTip.completionWidget ? documentModuleCompletionToolTip.completionWidget.model : []
            implicitWidth: Math.max(200, documentModuleCompletionToolTip.completionWidget ? documentModuleCompletionToolTip.completionWidget.contentSize.width + 45 : 0)
            implicitHeight: documentModuleCompletionToolTip.completionWidget ? Math.min(documentModuleCompletionToolTip.completionWidget.contentSize.height, 150) : 0
            clip: true
            focus: false
            keyNavigationEnabled: false
            keyNavigationWraps: false
            flickableDirection: Flickable.VerticalFlick
            property int hoveredIndex: -1

            ScrollBar.vertical: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: ItemDelegate {
                id: completionDelegate
                required property int index
                required property var modelData
                width: documentModuleCompletionListView.width
                height: documentModuleCompletionToolTip.rowHeight
                font: documentModuleCompletionToolTip.font
                leftPadding: 11; rightPadding: 10; topPadding: 0; bottomPadding: 0
                focusPolicy: Qt.NoFocus
                hoverEnabled: true
                highlighted: documentModuleCompletionListView.currentIndex === index

                contentItem: RowLayout {
                    spacing: 0

                    Item {
                        Layout.preferredWidth: 24
                        Layout.fillHeight: true

                        IconImage {
                            anchors.centerIn: parent
                            width: 16; height: 16
                            source: completionDelegate.modelData.source
                            color: completionDelegate.modelData.iconColor
                        }
                    }

                    Label {
                        font: completionDelegate.font
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: completionDelegate.modelData.displayText
                        textFormat: Text.RichText
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }

                onHoveredChanged: {
                    if (hovered) {
                        documentModuleCompletionListView.hoveredIndex = index
                    } else if (documentModuleCompletionListView.hoveredIndex === index) {
                        documentModuleCompletionListView.hoveredIndex = -1
                    }
                }
                onClicked: documentModuleCompletionListView.currentIndex = index
                onDoubleClicked: {
                    documentModuleCompletionListView.currentIndex = index
                    documentModuleCompletionToolTip.completionWidget.textReplace()
                }
            }

            onModelChanged: {
                hoveredIndex = -1
                positionViewAtBeginning()
                if (documentModuleCompletionToolTip.visible) documentModuleCompletionDetailTimer.restart()
            }
            onHoveredIndexChanged: {
                if (documentModuleCompletionToolTip.visible) documentModuleCompletionDetailTimer.restart()
            }
            onCurrentIndexChanged: {
                if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
                if (documentModuleCompletionToolTip.visible) documentModuleCompletionDetailTimer.restart()
            }
            onContentYChanged: {
                if (documentModuleCompletionToolTip.visible) documentModuleCompletionDetailTimer.restart()
            }

            Timer {
                id: documentModuleCompletionDetailTimer
                interval: 150

                onTriggered: {
                    const completion = documentModuleCompletionToolTip.completionWidget
                    if (!documentModuleCompletionToolTip.visible || !completion) return
                    const interestIndex = documentModuleCompletionListView.hoveredIndex !== -1
                        ? documentModuleCompletionListView.hoveredIndex : documentModuleCompletionListView.currentIndex
                    completion.detailReload(interestIndex)
                    if (interestIndex < 0 || interestIndex >= documentModuleCompletionListView.count) return
                    const rowHeight = documentModuleCompletionToolTip.rowHeight
                    let idealY = interestIndex * rowHeight - documentModuleCompletionListView.contentY
                    idealY = Math.max(0, Math.min(documentModuleCompletionListView.height - rowHeight, idealY))
                    documentModuleCompletionDetailToolTip.y = idealY - 6
                }
            }

        }

        ToolTip {
            id: documentModuleCompletionDetailToolTip
            x: documentModuleCompletionToolTip.width - 5
            padding: 6

            contentItem: ListView {
                id: documentModuleCompletionDetailListView
                model: documentModuleCompletionToolTip.completionWidget ? documentModuleCompletionToolTip.completionWidget.detailModel : []
                implicitWidth: documentModuleCompletionToolTip.completionWidget ? documentModuleCompletionToolTip.completionWidget.detailContentSize.width + 4 : 0
                implicitHeight: documentModuleCompletionToolTip.completionWidget ? Math.min(documentModuleCompletionToolTip.completionWidget.detailContentSize.height, 150) : 0
                clip: true
                focus: false
                keyNavigationEnabled: false
                currentIndex: -1
                flickableDirection: Flickable.VerticalFlick

                ScrollBar.vertical: ScrollBar {
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
                    }
                }

                delegate: Label {
                    required property var modelData
                    width: documentModuleCompletionDetailListView.width
                    height: documentModuleCompletionToolTip.rowHeight
                    font: documentModuleCompletionToolTip.font
                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                    text: modelData.text
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                }

                onModelChanged: positionViewAtBeginning()
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

    ToolTip {
        id: documentModuleNavigationToolTip
        parent: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
        padding: 6
        property point position
        property var navigationWidget
        readonly property real rowHeight: navigationWidget ? navigationWidget.rowHeight : 24

        onOpened: {
            mainWindow.overlayFlagSet(false, false)
            widgetCount += 1
        }
        onClosed: {
            widgetCount -= 1
            documentModuleNavigationDetailTimer.stop()
            documentModuleNavigationDetailToolTip.close()
            navigationWidget.navigationHide()
        }
        onAboutToShow: {
            position = Overlay.overlay.mapFromGlobal(position)
            x = position.x - leftPadding - 11
            y = position.y
            documentModuleNavigationDetailToolTip.open()
            documentModuleNavigationDetailTimer.restart()
        }

        contentItem: ListView {
            id: documentModuleNavigationListView
            model: documentModuleNavigationToolTip.navigationWidget ? documentModuleNavigationToolTip.navigationWidget.model : []
            implicitWidth: Math.max(200, documentModuleNavigationToolTip.navigationWidget ? documentModuleNavigationToolTip.navigationWidget.contentSize.width + 45 : 0)
            implicitHeight: documentModuleNavigationToolTip.navigationWidget ? Math.min(documentModuleNavigationToolTip.navigationWidget.contentSize.height, 150) : 0
            clip: true
            focus: false
            keyNavigationEnabled: false
            keyNavigationWraps: false
            flickableDirection: Flickable.VerticalFlick
            property int hoveredIndex: -1

            ScrollBar.vertical: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: ItemDelegate {
                id: navigationDelegate
                required property int index
                required property var modelData
                width: documentModuleNavigationListView.width
                height: documentModuleNavigationToolTip.rowHeight
                font: documentModuleNavigationToolTip.font
                leftPadding: 11; rightPadding: 10; topPadding: 0; bottomPadding: 0
                focusPolicy: Qt.NoFocus
                hoverEnabled: true
                highlighted: documentModuleNavigationListView.currentIndex === index

                contentItem: Label {
                    font: navigationDelegate.font
                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                    text: navigationDelegate.modelData.text
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                }

                onHoveredChanged: {
                    if (hovered) {
                        documentModuleNavigationListView.hoveredIndex = index
                    } else if (documentModuleNavigationListView.hoveredIndex === index) {
                        documentModuleNavigationListView.hoveredIndex = -1
                    }
                }
                onClicked: documentModuleNavigationListView.currentIndex = index
                onDoubleClicked: {
                    documentModuleNavigationListView.currentIndex = index
                    documentModuleNavigationToolTip.navigationWidget.indicatorInsert()
                }
            }

            onModelChanged: {
                hoveredIndex = -1
                positionViewAtBeginning()
                if (documentModuleNavigationToolTip.visible) documentModuleNavigationDetailTimer.restart()
            }
            onHoveredIndexChanged: {
                if (documentModuleNavigationToolTip.visible) documentModuleNavigationDetailTimer.restart()
            }
            onCurrentIndexChanged: {
                if (currentIndex >= 0) positionViewAtIndex(currentIndex, ListView.Contain)
                if (documentModuleNavigationToolTip.visible) documentModuleNavigationDetailTimer.restart()
            }
            onContentYChanged: {
                if (documentModuleNavigationToolTip.visible) documentModuleNavigationDetailTimer.restart()
            }

            Timer {
                id: documentModuleNavigationDetailTimer
                interval: 150

                onTriggered: {
                    const navigation = documentModuleNavigationToolTip.navigationWidget
                    if (!documentModuleNavigationToolTip.visible || !navigation) return
                    const interestIndex = documentModuleNavigationListView.hoveredIndex !== -1
                        ? documentModuleNavigationListView.hoveredIndex : documentModuleNavigationListView.currentIndex
                    navigation.detailReload(interestIndex)
                    if (interestIndex < 0 || interestIndex >= documentModuleNavigationListView.count) return
                    const rowHeight = documentModuleNavigationToolTip.rowHeight
                    let idealY = interestIndex * rowHeight - documentModuleNavigationListView.contentY
                    idealY = Math.max(0, Math.min(documentModuleNavigationListView.height - rowHeight, idealY))
                    documentModuleNavigationDetailToolTip.y = idealY - 6
                }
            }

        }

        ToolTip {
            id: documentModuleNavigationDetailToolTip
            x: documentModuleNavigationToolTip.width - 5

            contentItem: Label {
                font: documentModuleNavigationToolTip.font
                text: documentModuleNavigationToolTip.navigationWidget ? documentModuleNavigationToolTip.navigationWidget.detailText : ""
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
            text: documentModuleSignatureToolTip.signatureWidget ? documentModuleSignatureToolTip.signatureWidget.text : ""
            font: documentModuleSignatureToolTip.font
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
            operationErrorShow(global.documentModule.documentCreate(explorerModuleFileNewDialog.documentUrl + "/" + explorerModuleFileNameTextField.text))
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
            global.documentModule.directoryCreate(explorerModuleFolderNewDialog.documentUrl + "/" + explorerModuleFolderNameTextField.text))

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

            onTriggered: global.fileModule.showRenameDialog(explorerModuleFileMenu.documentUrl)
        }

        Menu {
            title: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    operationErrorShow(global.documentModule.documentDelete(explorerModuleFileMenu.documentUrl))
                    progress = 0
                    explorerModuleFileMenu.close()
                }
            }
        }

        MenuItem {
            text: qsTr("Property")
            icon.source: "qrc:/icon/property.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.showPropertyDialog(explorerModuleFileMenu.documentUrl)
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Git")
            enabled: global.gitModule.gitEnabled
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

            onTriggered: global.fileModule.showRenameDialog(explorerModuleFolderMenu.documentUrl)
        }

        Menu {
            title: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16

            DelayButton {
                delay: 1000
                text: qsTr("Confirm")

                onActivated: {
                    operationErrorShow(global.documentModule.directoryDelete(explorerModuleFolderMenu.documentUrl))
                    progress = 0
                    explorerModuleFolderMenu.close()
                }
            }
        }

        MenuSeparator {
        }

        Menu {
            title: qsTr("Git")
            enabled: global.gitModule.gitEnabled
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
            enabled: global.gitModule.gitEnabled
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

    // git module
    Dialog {
        id: gitModuleContinueDialog
        parent: Overlay.overlay
        x: mainScreenItem.x + (mainScreenItem.width - width) / 2
        y: mainScreenItem.y + (mainScreenItem.height - height) / 2
        width: 600
        modal: true
        title: global.gitModule.gitStatus === 2 ? qsTr("Finish Merge") : qsTr("Finish Rebase")
        standardButtons: Dialog.Ok

        onOpened: {
            mainWindow.overlayFlagSet(false, true)
            widgetCount += 1
        }
        onClosed: widgetCount -= 1
        onAboutToShow: {
            gitModuleContinueCheckBox.checked = global.gitModule.gitStatus !== 2
            gitModuleContinueCheckBox.enabled = global.gitModule.gitStatus === 2
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
            enabled: global.gitModule.gitStatus === 0 && ["upstream", "remote"].includes(gitModuleBranchMenu.type)
            icon.source: "qrc:/icon/link.svg"
            icon.width: 16; icon.height: 16

            onTriggered: gitModuleBranchMenu.type === "upstream" ? gitModule.gitUpstreamUnset() : gitModule.gitUpstreamSet(gitModuleBranchMenu.name)
        }

        MenuItem {
            text: qsTr("Merge")
            enabled: global.gitModule.gitStatus === 0
            icon.source: "qrc:/icon/gitMerge.svg"
            icon.width: 16; icon.height: 16

            onTriggered: gitModule.gitMerge(gitModuleBranchMenu.name)
        }

        MenuItem {
            text: qsTr("Rebase")
            enabled: global.gitModule.gitStatus === 0
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
            enabled: global.gitModule.gitStatus === 0
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

            onTriggered: menuModuleOpenFileItemDialog.open()

            FileDialog {
                id: menuModuleOpenFileItemDialog
                fileMode: FileDialog.OpenFile
                nameFilters: ["All files (*)"]
                onAccepted: global.documentModule.documentOpen(selectedFile)
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
        onAboutToShow: menuModuleEditMenu.menuSession = global.documentModule.menuLoad("edit")

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
                    color: menuModuleEditCutItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
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
            id: menuModuleEditCopyItem
            enabled: menuModuleEditMenu.menuSession ? menuModuleEditMenu.menuSession.copiable : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/copy.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleEditCopyItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
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
            id: menuModuleEditPasteItem
            enabled: menuModuleEditMenu.menuSession ? menuModuleEditMenu.menuSession.pastable : false

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/paste.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleEditPasteItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
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

        MenuItem {
            text: qsTr("Services")
            checkable: true
            checked: serviceModuleAction ? serviceModuleAction.checked : false

            onTriggered: serviceModuleAction.toggle()
        }

        Menu {
            id: terminalModuleTerminalMenu
            title: qsTr("Terminal")

            onOpened: {
                mainWindow.overlayFlagSet(false, true)
                widgetCount += 1
            }
            onClosed: widgetCount -= 1

            MenuItem {
                text: qsTr("Manage")

                onTriggered: global.terminalModule.terminalManage()
            }

            MenuSeparator {
            }

            Instantiator {
                id: terminalModuleTerminalInstantiator
                model: global.terminalModule.terminals
                delegate: MenuItem {
                    required property var modelData
                    text: modelData.name
                    onTriggered: global.terminalModule.terminalOpen(modelData.id)
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
        onAboutToShow: menuModuleNavMenu.menuSession = global.documentModule.menuLoad("nav")

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
                    color: menuModuleNavPrevItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
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

            onTriggered: global.documentModule.navigationPrev()
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
                    color: menuModuleNavNextItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
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

            onTriggered: global.documentModule.navigationNext()
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

            onTriggered: global.documentModule.definitionRequest(menuModuleNavMenu.menuSession.documentUrl, menuModuleNavMenu.menuSession.line, menuModuleNavMenu.menuSession.character)
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

            onTriggered: global.documentModule.referencesRequest(menuModuleNavMenu.menuSession.documentUrl, menuModuleNavMenu.menuSession.line, menuModuleNavMenu.menuSession.character)
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

            onTriggered: global.documentModule.implementationRequest(menuModuleNavMenu.menuSession.documentUrl, menuModuleNavMenu.menuSession.line, menuModuleNavMenu.menuSession.character)
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

            onTriggered: global.documentModule.typeDefinitionRequest(menuModuleNavMenu.menuSession.documentUrl, menuModuleNavMenu.menuSession.line, menuModuleNavMenu.menuSession.character)
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
        onAboutToShow: menuModuleCodeMenu.menuSession = global.documentModule.menuLoad("code")

        MenuItem {
            id: menuModuleCodeCompletionItem

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

            Shortcut {
                sequence: "Ctrl+Space"
                onActivated: {
                    menuModuleCodeMenu.menuSession = global.documentModule.menuLoad("code")
                    menuModuleCodeCompletionItem.triggered()
                }
            }

            onTriggered: global.documentModule.completionRequest(menuModuleCodeMenu.menuSession.documentUrl, menuModuleCodeMenu.menuSession.line, menuModuleCodeMenu.menuSession.character)
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
                    color: global.theme.general.fore
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
                    menuModuleCodeMenu.menuSession = global.documentModule.menuLoad("code")
                    menuModuleCodeReformatItem.triggered()
                }
            }

            onTriggered: {
                if (menuModuleCodeMenu.menuSession.text) {
                    global.documentModule.rangeFormattingRequest(menuModuleCodeMenu.menuSession.documentUrl, menuModuleCodeMenu.menuSession.startLine, menuModuleCodeMenu.menuSession.startCharacter, menuModuleCodeMenu.menuSession.endLine, menuModuleCodeMenu.menuSession.endCharacter)
                } else {
                    global.documentModule.formattingRequest(menuModuleCodeMenu.menuSession.documentUrl)
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

                onTriggered: global.documentModule.foldContractTop(menuModuleCodeMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Contract Recursively")
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.documentModule.foldContractRecursively(menuModuleCodeMenu.menuSession.documentUrl)
            }

            MenuItem {
                text: qsTr("Expand Recursively")
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16

                onTriggered: global.documentModule.foldExpandRecursively(menuModuleCodeMenu.menuSession.documentUrl)
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
        onAboutToShow: menuModuleExecMenu.menuSession = global.documentModule.menuLoad("exec")

        MenuItem {
            id: menuModuleExecRunItem

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
                    text: qsTr("Run ") + (menuModuleExecMenu.menuSession ? menuModuleExecMenu.menuSession.documentName : "")
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
                    menuModuleExecMenu.menuSession = global.documentModule.menuLoad("exec")
                    menuModuleExecRunItem.triggered()
                }
            }

            onTriggered: global.threadpoolModule.threadStart(menuModuleExecMenu.menuSession.documentUrl, RunMode.Run)
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
                    color: global.theme.general.successFore3
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
                    menuModuleExecMenu.menuSession = global.documentModule.menuLoad("exec")
                    menuModuleExecDebugItem.triggered()
                }
            }

            onTriggered: global.threadpoolModule.threadStart(menuModuleExecMenu.menuSession.documentUrl, RunMode.Debug)
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
            enabled: global.gitModule.gitStatus === 0
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/gitFetch.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleFetchItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
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
            enabled: global.gitModule.gitStatus === 0
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/gitCommit.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModuleCommitItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
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
            enabled: global.gitModule.gitStatus === 0
            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12

                IconImage {
                    source: "qrc:/icon/gitPush.svg"
                    sourceSize.width: 16
                    sourceSize.height: 16
                    color: menuModulePushItem.enabled ? global.theme.general.fore : global.theme.general.foreDisabled
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
            statusModuleEolModeMenu.eolMode = global.documentModule.eolModeGet(documentUrl)
            statusModuleEolModeViewItem.checked = global.documentModule.eolViewGet(documentUrl)
        }

        MenuItem {
            text: "CRLF"
            enabled: statusModuleEolModeMenu.eolMode !== 0
            onTriggered: {
                global.documentModule.eolModeSet(statusModuleEolModeMenu.documentUrl, 0)
                statusModuleEolModeMenu.eolModeButton.text = "CRLF"
            }
        }

        MenuItem {
            text: "CR"
            enabled: statusModuleEolModeMenu.eolMode !== 1
            onTriggered: {
                global.documentModule.eolModeSet(statusModuleEolModeMenu.documentUrl, 1)
                statusModuleEolModeMenu.eolModeButton.text = "CR"
            }
        }

        MenuItem {
            text: "LF"
            enabled: statusModuleEolModeMenu.eolMode !== 2
            onTriggered: {
                global.documentModule.eolModeSet(statusModuleEolModeMenu.documentUrl, 2)
                statusModuleEolModeMenu.eolModeButton.text = "LF"
            }
        }

        MenuSeparator {
        }

        MenuItem {
            id: statusModuleEolModeViewItem
            checkable: true
            text: qsTr("View EOL")

            onTriggered: global.documentModule.eolViewSet(statusModuleEolModeMenu.documentUrl, statusModuleEolModeViewItem.checked)
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
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            Rectangle {
                anchors.fill: parent
                color: global.theme.general.stroke
            }

            delegate: Item {
                implicitWidth: statusModuleBackgroundTableView.width
                implicitHeight: 48

                Rectangle {
                    anchors.fill: parent
                    color: global.theme.general.back
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

    Component.onCompleted: {
        const objects = {
            "mainOverlay": mainItem,
            "mainWindowCloseDialog": mainWindowCloseDialog,
            "mainWindowMessageDialog": mainWindowMessageDialog,
            "mainWindowQuitDialog": mainWindowQuitDialog,
            "mainWindowLinkMenu": mainWindowLinkMenu,
            "mainWindowTextView": mainWindowTextView,
            "mainWindowToolTip": mainWindowToolTip,

            "agentModuleRenameDialog": agentModuleRenameDialog,
            "agentModuleModeMenu": agentModuleModeMenu,
            "agentModuleModelMenu": agentModuleModelMenu,

            "documentModuleCompletionToolTip": documentModuleCompletionToolTip,
            "documentModuleCompletionListView": documentModuleCompletionListView,
            "documentModuleDwellToolTip": documentModuleDwellToolTip,
            "documentModuleDwellDiagnosticTextArea": documentModuleDwellDiagnosticTextArea,
            "documentModuleDwellHoverTextArea": documentModuleDwellHoverTextArea,
            "documentModuleDwellCodeActionMenu": documentModuleDwellCodeActionMenu,
            "documentModuleDwellSuggestionMenu": documentModuleDwellSuggestionMenu,
            "documentModuleNavigationToolTip": documentModuleNavigationToolTip,
            "documentModuleNavigationListView": documentModuleNavigationListView,
            "documentModulePositionTooltip": documentModulePositionTooltip,
            "documentModuleSignatureToolTip": documentModuleSignatureToolTip,

            "explorerModuleFileMenu": explorerModuleFileMenu,
            "explorerModuleFolderMenu": explorerModuleFolderMenu,
            "explorerModuleRootMenu": explorerModuleRootMenu,

            "gitModuleContinueDialog": gitModuleContinueDialog,
            "gitModuleErrorDialog": gitModuleErrorDialog,
            "gitModuleProxyDialog": gitModuleProxyDialog,
            "gitModuleRemoteAddDialog": gitModuleRemoteAddDialog,
            "gitModuleBranchMenu": gitModuleBranchMenu,
            "gitModuleLogMenu": gitModuleLogMenu,

            "menuModuleFileMenu": menuModuleFileMenu,
            "menuModuleEditMenu": menuModuleEditMenu,
            "menuModuleViewMenu": menuModuleViewMenu,
            "menuModuleNavMenu": menuModuleNavMenu,
            "menuModuleCodeMenu": menuModuleCodeMenu,
            "menuModuleExecMenu": menuModuleExecMenu,
            "menuModuleGitMenu": menuModuleGitMenu,

            "statusModuleEolModeMenu": statusModuleEolModeMenu,
            "statusModuleBackgroundToolTip": statusModuleBackgroundToolTip
        };
        mainWindow.propertyGet(objects)
    }
}
