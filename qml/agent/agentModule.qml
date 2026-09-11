import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent
    property var turnMap: ({})
    property var chatMap: ({})
    property var subagentMap: ({})

    function requestSubmit(): void {
        const text = textArea.text.trim()
        if (agentModule.state === 0 && text === "/goal") {
            goalCard.opened = true
            textArea.clear()
            return
        }
        if (text.length > 0) {
            agentModule.state === 0 ? agentModule.pre() : agentModule.steer()
        } else {
            agentModule.abort()
        }
    }

    ListModel {
        id: attachmentModel
    }

    FileDialog {
        id: attachmentDialog
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("All files (*)")]

        onAccepted: {
            for (let index = 0; index < selectedFiles.length; ++index) {
                agentModule.attachmentAdd(selectedFiles[index])
            }
        }
    }

    component FlipLabel: Flipable {
        id: flipLabel
        property string text
        property color color: global.stroke
        property string frontText
        property string backText
        Layout.preferredWidth: Math.max(frontLabel.implicitWidth, backLabel.implicitWidth)
        Layout.preferredHeight: 28

        function updateText(value: string): void {
            if (flipAnimation.running) flipAnimation.complete()
            if (value.length === 0) {
                flipRotation.angle = 0
                frontText = ""
                backText = ""
            } else if (frontText.length === 0) {
                frontText = value
                backText = value
            } else if (frontText !== value) {
                backText = value
                flipAnimation.restart()
            }
        }

        onTextChanged: updateText(text)
        Component.onCompleted: updateText(text)

        front: Label {
            id: frontLabel
            anchors.fill: parent
            text: flipLabel.frontText
            color: flipLabel.color
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
        }

        back: Label {
            id: backLabel
            anchors.fill: parent
            text: flipLabel.backText
            color: flipLabel.color
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter

            transform: Rotation {
                origin.x: backLabel.width / 2
                origin.y: backLabel.height / 2
                axis.x: 1; axis.y: 0; axis.z: 0
                angle: 180
            }
        }

        transform: Rotation {
            id: flipRotation
            origin.x: flipLabel.width / 2
            origin.y: flipLabel.height / 2
            axis.x: 1; axis.y: 0; axis.z: 0
            angle: 0
        }

        NumberAnimation {
            id: flipAnimation
            target: flipRotation
            property: "angle"
            from: 0
            to: -180
            duration: 300
            easing.type: Easing.InOutCubic

            onFinished: {
                flipLabel.frontText = flipLabel.backText
                flipRotation.angle = 0
            }
        }
    }

    ListModel {
        id: permissionModel
    }

    ListModel {
        id: userInputModel
    }

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    ToolTip {
        id: turnToolTip
        parent: Overlay.overlay
        closePolicy: Popup.NoAutoClose
        visible: prompt.length > 0
        width: Math.min(implicitWidth, 320)
        height: implicitHeight
        property string prompt
        property string response

        FontMetrics {
            id: turnToolTipFontMetrics
            font: turnToolTip.font
        }

        contentItem: ColumnLayout {
            Label {
                text: turnToolTip.prompt
                elide: Text.ElideRight
                maximumLineCount: 1
                wrapMode: Text.NoWrap
                color: global.fore
                font.bold: true
                Layout.fillWidth: true
            }

            TextArea {
                visible: text.length > 0
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                readOnly: true
                text: turnToolTip.response
                textFormat: TextEdit.MarkdownText
                wrapMode: TextEdit.Wrap
                verticalAlignment: TextEdit.AlignTop
                clip: true
                color: global.stroke
                background: null
                ContextMenu.menu: null
                Layout.fillWidth: true
                Layout.maximumHeight: turnToolTipFontMetrics.lineSpacing * 4
            }
        }

        Behavior on width {
            enabled: turnToolTip.visible
            NumberAnimation {
                duration: 80
                easing.type: Easing.OutCubic
            }
        }

        Behavior on height {
            enabled: turnToolTip.visible
            NumberAnimation {
                duration: 80
                easing.type: Easing.OutCubic
            }
        }

        Behavior on x {
            enabled: turnToolTip.visible
            NumberAnimation {
                duration: 80
                easing.type: Easing.OutCubic
            }
        }

        Behavior on y {
            enabled: turnToolTip.visible
            NumberAnimation {
                duration: 80
                easing.type: Easing.OutCubic
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6

        RowLayout {
            Layout.fillWidth: true; Layout.preferredHeight: 30

            ComboBox {
                id: conversationComboBox
                enabled: agentModule.state === 0 && agentModule.goalState === 0
                model: conversationModel
                textRole: "display"
                valueRole: "id"
                Layout.fillWidth: true; Layout.preferredHeight: 30

                onActivated: agentModule.conversationGet(conversationComboBox.currentValue)
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                enabled: agentModule.state === 0 && agentModule.goalState === 0
                flat: true
                icon.source: "qrc:/icon/rename.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: {
                    renameDialog.oldTitle = conversationComboBox.currentText
                    renameDialog.open()
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                enabled: agentModule.state === 0 && agentModule.goalState === 0
                flat: true
                icon.source: "qrc:/icon/add.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: agentModule.conversationInsert()
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                checkable: true
                enabled: conversationComboBox.currentIndex >= 0 && agentModule.state === 0 && agentModule.goalState === 0
                flat: true
                icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/delete.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onToggled: {
                    if (!checked) {
                        agentModule.conversationDelete()
                    }
                }

                Timer {
                    interval: 1000
                    running: parent.checked
                    onTriggered: parent.checked = false
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                flat: true
                icon.source: "qrc:/icon/settings.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: agentModule.agentManage()
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                enabled: conversationComboBox.currentIndex >= 0
                flat: true
                icon.source: "qrc:/icon/dataPie.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: agentModule.evalOpen()
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: 4

            ListView {
                id: turnListView
                visible: count > 0
                model: chatColumn.children.length
                property int hoveredIndex: -1
                readonly property real viewportPosition: Math.max(0, Math.min(chatScrollBar.position, 1 - chatScrollBar.size))
                readonly property real viewportTop: viewportPosition * chatColumn.height
                readonly property real viewportBottom: viewportTop + chatView.availableHeight
                topMargin: Math.max(0, (height - count * 12) / 2)
                bottomMargin: topMargin
                boundsBehavior: Flickable.StopAtBounds
                snapMode: ListView.SnapToItem
                clip: true
                Layout.preferredWidth: 24; Layout.fillHeight: true

                onCountChanged: {
                    if (count === 0) return
                    currentIndex = count - 1
                    positionViewAtIndex(currentIndex, ListView.Visible)
                }

                delegate: Item {
                    id: turnDelegate
                    required property int index
                    width: turnListView.width
                    height: 12
                    readonly property int hoverDistance: turnListView.hoveredIndex < 0 ? 4 : Math.min(4, Math.abs(index - turnListView.hoveredIndex))
                    readonly property var turn: chatColumn.children[index]
                    readonly property bool turnVisible: turn.y < turnListView.viewportBottom && turn.y + turn.height > turnListView.viewportTop

                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 24 - turnDelegate.hoverDistance * 4
                        height: 2
                        radius: 1
                        color: turnListView.hoveredIndex < 0
                            ? turnDelegate.turnVisible ? global.fore : global.stroke
                            : hoverHandler.hovered ? global.fore : global.stroke
                        opacity: turnListView.hoveredIndex < 0 ? 1 : 1 - turnDelegate.hoverDistance * 0.15

                        Behavior on width {
                            NumberAnimation {
                                duration: 150
                            }
                        }
                    }

                    HoverHandler {
                        id: hoverHandler
                        onHoveredChanged: {
                            if (hovered) {
                                turnListView.hoveredIndex = turnDelegate.index
                                const position = turnToolTip.parent.mapFromItem(parent, parent.width, parent.height / 2)
                                turnToolTip.prompt = turnDelegate.turn.prompt
                                turnToolTip.response = turnDelegate.turn.response
                                turnToolTip.x = position.x + 10
                                turnToolTip.y = position.y - turnToolTip.implicitHeight / 2
                            } else if (turnListView.hoveredIndex === turnDelegate.index) {
                                turnListView.hoveredIndex = -1
                                turnToolTip.prompt = ""
                                turnToolTip.response = ""
                            }
                        }
                    }

                    TapHandler {
                        onTapped: {
                            turnListView.currentIndex = turnDelegate.index
                            turnListView.positionViewAtIndex(turnDelegate.index, ListView.Visible)
                            rootItem.navigateTo(turnDelegate.turn.y)
                        }
                    }
                }
            }

            ScrollView {
                id: chatView
                rightPadding: 14
                Layout.fillWidth: true; Layout.fillHeight: true
                property bool followTail: true

                ScrollBar.vertical: ScrollBar {
                    id: chatScrollBar
                    x: parent.mirrored ? 0 : parent.width - width
                    y: parent.topPadding
                    height: parent.availableHeight
                    active: parent.ScrollBar.horizontal.active
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }

                    onPositionChanged: {
                        if (followAnimation.running || navigationAnimation.running) return
                        const viewportPosition = turnListView.viewportPosition
                        let index = viewportPosition === 0 ? 0 : turnListView.count - 1
                        if (viewportPosition > 0 && viewportPosition < 1 - size) {
                            const viewportY = viewportPosition * chatColumn.height + chatView.availableHeight / 2
                            for (let i = 0; i < chatColumn.children.length; ++i) {
                                const turn = chatColumn.children[i]
                                if (viewportY < turn.y + turn.height) {
                                    index = i
                                    break
                                }
                            }
                        }
                        if (index === turnListView.currentIndex) return
                        turnListView.currentIndex = index
                        turnListView.positionViewAtIndex(index, ListView.Visible)
                    }

                    onPressedChanged: {
                        if (pressed) {
                            chatView.followTail = false
                            rootItem.scrollStop()
                            return
                        }
                        chatView.followTail = chatView.contentItem.atYEnd
                    }
                }

                ColumnLayout {
                    id: chatColumn
                    width: chatView.availableWidth
                }
            }
        }

        Item {
            id: goalCard
            visible: opened || goalState !== idleState
            clip: true
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? goalLayout.implicitHeight + 24 : 0
            property bool opened: false
            property int goalState: agentModule.goalState
            property double remaining: 0
            readonly property int idleState: 0
            readonly property int runningState: 1
            readonly property int pausedState: 2

            function remainingText(): string {
                const seconds = Math.ceil(remaining / 1000)
                const hours = Math.floor(seconds / 3600)
                const minutes = Math.floor(seconds % 3600 / 60)
                const remainder = seconds % 60
                return hours.toString().padStart(2, "0") + ":"
                    + minutes.toString().padStart(2, "0") + ":"
                    + remainder.toString().padStart(2, "0")
            }

            Rectangle {
                anchors.fill: parent
                color: global.backSelected
                border.color: global.stroke
                border.width: 1
                radius: 6
            }

            Item {
                id: goalRoad
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                anchors.leftMargin: 1; anchors.rightMargin: 1; anchors.topMargin: 1
                height: 4
                clip: true

                Rectangle {
                    id: goalRainbow
                    x: -goalRoad.width
                    width: goalRoad.width * 2
                    height: goalRoad.height
                    opacity: goalCard.goalState === goalCard.idleState ? 0.4 : 1
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.00; color: "#ff5f57" }
                        GradientStop { position: 0.08; color: "#ffbd2e" }
                        GradientStop { position: 0.16; color: "#28c840" }
                        GradientStop { position: 0.24; color: "#27c2ff" }
                        GradientStop { position: 0.32; color: "#2f80ed" }
                        GradientStop { position: 0.40; color: "#9b51e0" }
                        GradientStop { position: 0.50; color: "#ff5f57" }
                        GradientStop { position: 0.58; color: "#ffbd2e" }
                        GradientStop { position: 0.66; color: "#28c840" }
                        GradientStop { position: 0.74; color: "#27c2ff" }
                        GradientStop { position: 0.82; color: "#2f80ed" }
                        GradientStop { position: 0.90; color: "#9b51e0" }
                        GradientStop { position: 1.00; color: "#ff5f57" }
                    }

                    NumberAnimation on x {
                        running: goalCard.goalState === goalCard.runningState
                        from: -goalRoad.width
                        to: 0
                        duration: 5000
                        loops: Animation.Infinite
                    }
                }
            }

            ColumnLayout {
                id: goalLayout
                anchors.fill: parent
                anchors.leftMargin: 10; anchors.rightMargin: 10
                anchors.topMargin: 12; anchors.bottomMargin: 10
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        id: goalTextField
                        enabled: goalCard.goalState === goalCard.idleState
                        placeholderText: qsTr("What should the agent explore?")
                        selectByMouse: true
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    IconImage {
                        color: global.stroke
                        source: "qrc:/icon/clock.svg"
                        sourceSize.width: 16; sourceSize.height: 16
                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    }

                    SpinBox {
                        id: goalHoursSpinBox
                        from: 0
                        to: 99
                        value: 8
                        editable: true
                        enabled: goalCard.goalState === goalCard.idleState
                        textFromValue: function(value) { return value.toString().padStart(2, "0") }
                        valueFromText: function(text) { return parseInt(text, 10) }
                        Layout.preferredWidth: 96; Layout.preferredHeight: 36
                    }

                    Label {
                        text: ":"
                        color: global.stroke
                    }

                    SpinBox {
                        id: goalMinutesSpinBox
                        from: 0
                        to: 59
                        editable: true
                        enabled: goalCard.goalState === goalCard.idleState
                        textFromValue: function(value) { return value.toString().padStart(2, "0") }
                        valueFromText: function(text) { return parseInt(text, 10) }
                        Layout.preferredWidth: 96; Layout.preferredHeight: 36
                    }

                    Label {
                        visible: goalCard.goalState !== goalCard.idleState
                        text: goalCard.remainingText()
                        color: goalCard.goalState === goalCard.pausedState ? global.warningFore3 : global.stroke
                        Layout.leftMargin: 4
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        flat: true
                        icon.source: "qrc:/icon/play.svg"
                        icon.width: 16; icon.height: 16
                        enabled: goalCard.goalState === goalCard.idleState
                            && agentModule.state === 0
                            && goalTextField.text.trim().length > 0
                            && (goalHoursSpinBox.value > 0 || goalMinutesSpinBox.value > 0)
                        Layout.preferredWidth: 28; Layout.preferredHeight: 28

                        onClicked: agentModule.goalStart(
                            goalTextField.text.trim(),
                            goalHoursSpinBox.value,
                            goalMinutesSpinBox.value)
                    }

                    Button {
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        flat: true
                        icon.source: goalCard.goalState === goalCard.pausedState ? "qrc:/icon/play.svg" : "qrc:/icon/pause.svg"
                        icon.width: 16; icon.height: 16
                        enabled: goalCard.goalState !== goalCard.idleState
                        Layout.preferredWidth: 28; Layout.preferredHeight: 28

                        onClicked: goalCard.goalState === goalCard.pausedState
                            ? agentModule.goalResume()
                            : agentModule.goalPause()
                    }

                    Button {
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        flat: true
                        icon.source: "qrc:/icon/stop.svg"
                        icon.width: 16; icon.height: 16
                        enabled: goalCard.goalState !== goalCard.idleState
                        Layout.preferredWidth: 28; Layout.preferredHeight: 28

                        onClicked: agentModule.goalStop()
                    }
                }
            }

            Timer {
                interval: 1000
                repeat: true
                running: goalCard.goalState !== goalCard.idleState
                triggeredOnStart: true

                onTriggered: goalCard.remaining = agentModule.goalRemainingGet()
            }

            Connections {
                target: agentModule

                function onChangeGoal(): void {
                    goalCard.remaining = agentModule.goalRemainingGet()
                }
            }
        }

        Item {
            id: planCard
            visible: steps.length > 0 && !minimized
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? planLayout.implicitHeight + 20 : 0
            property string explanation
            property var steps: []
            property bool minimized: true
            readonly property int completedCount: {
                let count = 0
                for (let i = 0; i < steps.length; ++i) {
                    if (steps[i].status === "completed") ++count
                }
                return count
            }

            Rectangle {
                anchors.fill: parent
                color: global.backSelected
                border.color: global.stroke
                border.width: 1
                radius: 6
            }

            ColumnLayout {
                id: planLayout
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    IconImage {
                        color: global.fore
                        source: "qrc:/icon/taskList.svg"
                        sourceSize.width: 16; sourceSize.height: 16
                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    }

                    Label {
                        text: qsTr("Plan")
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Label {
                        text: planCard.completedCount + " / " + planCard.steps.length
                        color: global.fore
                    }
                }

                RowLayout {
                    visible: planCard.explanation.length > 0
                    Layout.fillWidth: true
                    spacing: 8

                    Item {
                        Layout.preferredWidth: 16
                    }

                    Label {
                        text: planCard.explanation
                        color: global.fore
                        opacity: 0.72
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: planCard.steps

                        delegate: RowLayout {
                            id: stepDelegate
                            required property var modelData
                            readonly property string stepStatus: modelData.status
                            Layout.fillWidth: true
                            spacing: 8

                            Item {
                                Layout.preferredWidth: 16
                            }

                            Item {
                                Layout.preferredWidth: 16; Layout.preferredHeight: 16

                                IconImage {
                                    anchors.fill: parent
                                    color: stepDelegate.stepStatus === "in_progress" ? global.fore : global.stroke
                                    source: stepDelegate.stepStatus === "completed" ? "qrc:/icon/taskCompleted.svg" :
                                            stepDelegate.stepStatus === "in_progress" ? "qrc:/icon/taskInProgress.svg" :
                                            "qrc:/icon/taskPending.svg"
                                    sourceSize.width: 16; sourceSize.height: 16
                                }
                            }

                            Label {
                                text: stepDelegate.modelData.step
                                color: stepDelegate.stepStatus === "in_progress" ? global.fore : global.stroke
                                font.bold: stepDelegate.stepStatus === "in_progress"
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: permissionCards
            visible: permissionModel.count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: visible && permissionSwipeView.currentItem
                ? permissionSwipeView.currentItem.cardImplicitHeight
                + (permissionSwipeView.count > 1 ? permissionIndicator.implicitHeight : 0)
                : 0

            Rectangle {
                anchors.fill: parent
                color: global.backSelected
                border.color: global.stroke
                border.width: 1
                radius: 6
            }

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: global.fore
                border.width: 1
                radius: 6
                opacity: 0.2

                SequentialAnimation on opacity {
                    running: permissionCards.visible
                    loops: Animation.Infinite

                    NumberAnimation {
                        from: 0.2
                        to: 0.8
                        duration: 450
                        easing.type: Easing.InOutSine
                    }

                    NumberAnimation {
                        from: 0.8
                        to: 0.2
                        duration: 850
                        easing.type: Easing.InOutSine
                    }

                    PauseAnimation {
                        duration: 250
                    }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                SwipeView {
                    id: permissionSwipeView
                    clip: true
                    interactive: true
                    orientation: Qt.Horizontal
                    Layout.fillWidth: true
                    Layout.preferredHeight: currentItem ? currentItem.cardImplicitHeight : 0

                    Repeater {
                        model: permissionModel

                        delegate: Item {
                            id: permissionPage
                            required property string runtimeId
                            required property string role
                            required property string message
                            readonly property real cardImplicitHeight: permissionLayout.implicitHeight + 20

                            ColumnLayout {
                                id: permissionLayout
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    IconImage {
                                        color: global.fore
                                        source: "qrc:/icon/shield.svg"
                                        sourceSize.width: 16; sourceSize.height: 16
                                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                    }

                                    Label {
                                        text: qsTr("Allow this action?")
                                        font.bold: true
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: permissionPage.role === "data" ? qsTr("Data") :
                                                permissionPage.role === "hardware" ? qsTr("Hardware") :
                                                    permissionPage.role === "software" ? qsTr("Software") :
                                                        permissionPage.role === "supervisor" ? qsTr("Supervisor") : qsTr("Agent")
                                        color: global.stroke
                                    }

                                    Button {
                                        text: qsTr("Deny")
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        Layout.preferredWidth: 80; Layout.preferredHeight: 28

                                        onClicked: {
                                            rootItem.permissionResponse(permissionPage.runtimeId, false)
                                        }
                                    }

                                    Button {
                                        text: qsTr("Allow")
                                        highlighted: true
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        Layout.preferredWidth: 80; Layout.preferredHeight: 28

                                        onClicked: {
                                            rootItem.permissionResponse(permissionPage.runtimeId, true)
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Item {
                                        Layout.preferredWidth: 16
                                    }

                                    Label {
                                        text: permissionPage.message
                                        color: global.fore
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }

                PageIndicator {
                    id: permissionIndicator
                    visible: permissionSwipeView.count > 1
                    count: permissionSwipeView.count
                    currentIndex: permissionSwipeView.currentIndex
                    Layout.fillWidth: false
                    Layout.preferredHeight: visible ? implicitHeight : 0
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    background: Rectangle {
                        color: "transparent"
                    }
                }
            }
        }

        Item {
            id: userInputCards
            visible: userInputModel.count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: visible && userInputSwipeView.currentItem
                ? userInputSwipeView.currentItem.cardImplicitHeight
                + (userInputSwipeView.count > 1 ? userInputIndicator.implicitHeight : 0)
                : 0

            Rectangle {
                anchors.fill: parent
                color: global.backSelected
                border.color: global.stroke
                border.width: 1
                radius: 6
            }

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: global.fore
                border.width: 1
                radius: 6
                opacity: 0.2

                SequentialAnimation on opacity {
                    running: userInputCards.visible
                    loops: Animation.Infinite

                    NumberAnimation {
                        from: 0.2
                        to: 0.8
                        duration: 450
                        easing.type: Easing.InOutSine
                    }

                    NumberAnimation {
                        from: 0.8
                        to: 0.2
                        duration: 850
                        easing.type: Easing.InOutSine
                    }

                    PauseAnimation {
                        duration: 250
                    }
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                SwipeView {
                    id: userInputSwipeView
                    clip: true
                    interactive: true
                    orientation: Qt.Horizontal
                    Layout.fillWidth: true
                    Layout.preferredHeight: currentItem ? currentItem.cardImplicitHeight : 0

                    Repeater {
                        model: userInputModel

                        delegate: Item {
                            id: userInputPage
                            required property string runtimeId
                            required property string role
                            required property string question
                            required property var options
                            readonly property real cardImplicitHeight: userInputLayout.implicitHeight + 20

                            function submit(): void {
                                const answer = answerTextField.text.trim()
                                if (answer.length === 0) return
                                rootItem.userInputResponse(userInputPage.runtimeId, answer)
                            }

                            ColumnLayout {
                                id: userInputLayout
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    IconImage {
                                        color: global.fore
                                        source: "qrc:/icon/chat.svg"
                                        sourceSize.width: 16; sourceSize.height: 16
                                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                    }

                                    Label {
                                        text: qsTr("Input required")
                                        font.bold: true
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: userInputPage.role === "data" ? qsTr("Data") :
                                                userInputPage.role === "hardware" ? qsTr("Hardware") :
                                                    userInputPage.role === "software" ? qsTr("Software") :
                                                        userInputPage.role === "supervisor" ? qsTr("Supervisor") : qsTr("Agent")
                                        color: global.stroke
                                    }
                                }

                                Label {
                                    text: userInputPage.question
                                    color: global.fore
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 24
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 24
                                    spacing: 8

                                    Repeater {
                                        model: userInputPage.options

                                        delegate: Button {
                                            id: optionButton
                                            required property var modelData
                                            text: modelData.description
                                                ? modelData.label + " — " + modelData.description
                                                : modelData.label
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 28

                                            contentItem: Label {
                                                text: optionButton.text
                                                color: optionButton.palette.buttonText
                                                font: optionButton.font
                                                elide: Text.ElideRight
                                                horizontalAlignment: Text.AlignLeft
                                                verticalAlignment: Text.AlignVCenter
                                            }

                                            onClicked: {
                                                answerTextField.text = modelData.label
                                                answerTextField.forceActiveFocus()
                                                answerTextField.selectAll()
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 24
                                    spacing: 8

                                    TextField {
                                        id: answerTextField
                                        placeholderText: qsTr("Enter your answer")
                                        selectByMouse: true
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 28

                                        onAccepted: userInputPage.submit()
                                    }

                                    Button {
                                        text: qsTr("Don't ask")
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        Layout.preferredWidth: 80; Layout.preferredHeight: 28

                                        onClicked: {
                                            rootItem.userInputResponse(userInputPage.runtimeId, "")
                                        }
                                    }

                                    Button {
                                        text: qsTr("Submit")
                                        highlighted: true
                                        enabled: answerTextField.text.trim().length > 0
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        Layout.preferredWidth: 80; Layout.preferredHeight: 28

                                        onClicked: userInputPage.submit()
                                    }
                                }
                            }
                        }
                    }
                }

                PageIndicator {
                    id: userInputIndicator
                    visible: userInputSwipeView.count > 1
                    count: userInputSwipeView.count
                    currentIndex: userInputSwipeView.currentIndex
                    Layout.fillWidth: false
                    Layout.preferredHeight: visible ? implicitHeight : 0
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    background: Rectangle {
                        color: "transparent"
                    }
                }
            }
        }

        Item {
            id: changeCard
            visible: hasChanges && !minimized
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? changeLayout.implicitHeight + 20 : 0
            property var changes: ({})
            property bool minimized: true
            readonly property var steps: changes.steps ? changes.steps : []
            readonly property var fileDiffs: changes.fileDiffs ? changes.fileDiffs : ({})
            readonly property int additions: changes.additions ? changes.additions : 0
            readonly property int deletions: changes.deletions ? changes.deletions : 0
            readonly property bool committed: changes.committed === true
            readonly property bool applied: changes.applied === true
            readonly property string error: changes.error ? changes.error : ""
            readonly property var urls: Object.keys(fileDiffs)
            readonly property int fileCount: urls.length
            readonly property int changeCount: Math.max(steps.length, fileCount > 0 ? 1 : 0)
            readonly property bool hasChanges: changeCount > 0

            Rectangle {
                anchors.fill: parent
                color: global.backSelected
                border.color: global.stroke
                border.width: 1
                radius: 6
            }

            ColumnLayout {
                id: changeLayout
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    IconImage {
                        color: global.fore
                        source: "qrc:/icon/edit.svg"
                        sourceSize.width: 16; sourceSize.height: 16
                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    }

                    Label {
                        text: qsTr("Changes")
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Button {
                        id: changeUndoButton
                        checkable: true
                        enabled: agentModule.state === 0 && changeCard.committed && changeCard.applied && changeCard.error.length === 0
                        leftPadding: 4; rightPadding: 4; topPadding: 0; bottomPadding: 0
                        flat: true
                        text: {
                            if (changeCard.error.length > 0) return qsTr("Unavailable")
                            if (changeCard.committed && !changeCard.applied) return qsTr("Undone")
                            return checked ? qsTr("Confirm") : qsTr("Undo")
                        }
                        icon.source: checked || changeCard.committed && !changeCard.applied ? "qrc:/icon/checkmark.svg" : "qrc:/icon/undo.svg"
                        icon.width: 16; icon.height: 16
                        implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
                        Layout.preferredHeight: 24

                        onToggled: {
                            if (!checked) agentModule.changeRevert()
                        }

                        onEnabledChanged: {
                            if (!enabled) checked = false
                        }

                        Timer {
                            interval: 1000
                            running: parent.checked
                            onTriggered: parent.checked = false
                        }
                    }
                }

                ScrollView {
                    id: changeScrollView
                    clip: true
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(changeContent.implicitHeight, 240)
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        id: changeContent
                        width: changeScrollView.availableWidth
                        spacing: 6

                        Label {
                            visible: changeCard.error.length > 0
                            text: changeCard.error
                            color: global.dangerFore3
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        Label {
                            visible: changeCard.fileCount > 0
                            text: qsTr("Files")
                            color: global.fore
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: changeCard.urls

                            delegate: RowLayout {
                                required property var modelData
                                readonly property var fileDiff: changeCard.fileDiffs[modelData]
                                Layout.fillWidth: true
                                spacing: 8

                                Item {
                                    Layout.preferredWidth: 16
                                }

                                Label {
                                    text: fileDiff.path
                                    color: global.fore
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: "+" + fileDiff.additions
                                    color: global.successFore3
                                }

                                Label {
                                    text: "-" + fileDiff.deletions
                                    color: global.dangerFore3
                                }
                            }
                        }

                        Label {
                            visible: changeCard.steps.length > 0
                            text: qsTr("Operations")
                            color: global.fore
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        Repeater {
                            model: changeCard.steps

                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: 8

                                Item {
                                    Layout.preferredWidth: 16
                                }

                                Label {
                                    text: modelData
                                    color: global.fore
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: overviewCard
            visible: planCard.steps.length > 0 || changeCard.hasChanges
            Layout.preferredWidth: visible ? overviewLayout.implicitWidth + 12 : 0
            Layout.preferredHeight: visible ? overviewLayout.implicitHeight + 12 : 0
            Layout.alignment: Qt.AlignHCenter

            Rectangle {
                anchors.fill: parent
                color: global.backSelected
                border.color: global.stroke
                border.width: 1
                radius: 6
            }

            RowLayout {
                id: overviewLayout
                anchors.fill: parent
                anchors.margins: 6

                Button {
                    id: planButton
                    visible: planCard.steps.length > 0
                    leftPadding: 4; rightPadding: 4; topPadding: 0; bottomPadding: 0
                    flat: true
                    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
                    Layout.preferredHeight: 24

                    contentItem: RowLayout {
                        spacing: 8

                        IconImage {
                            color: global.fore
                            source: "qrc:/icon/taskList.svg"
                            sourceSize.width: 16; sourceSize.height: 16
                            Layout.preferredWidth: 16; Layout.preferredHeight: 16
                        }

                        Label {
                            text: planCard.completedCount + " / " + planCard.steps.length
                            Layout.preferredWidth: implicitWidth
                        }
                    }

                    onClicked: planCard.minimized = !planCard.minimized
                }

                Button {
                    id: changeButton
                    visible: changeCard.hasChanges
                    leftPadding: 4; rightPadding: 4; topPadding: 0; bottomPadding: 0
                    flat: true
                    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
                    Layout.preferredHeight: 24

                    contentItem: RowLayout {
                        spacing: 8

                        IconImage {
                            color: global.fore
                            source: "qrc:/icon/edit.svg"
                            sourceSize.width: 16; sourceSize.height: 16
                            Layout.preferredWidth: 16; Layout.preferredHeight: 16
                        }

                        FlipLabel {
                            text: changeCard.changeCount.toString()
                            color: global.fore
                            Layout.preferredHeight: 24
                        }

                        Label {
                            text: qsTr("changes")
                            color: global.fore
                            Layout.preferredWidth: implicitWidth
                        }

                        FlipLabel {
                            visible: changeCard.fileCount > 0
                            text: "+" + changeCard.additions
                            color: global.successFore3
                            Layout.preferredHeight: 24
                        }

                        FlipLabel {
                            visible: changeCard.fileCount > 0
                            text: "-" + changeCard.deletions
                            color: global.dangerFore3
                            Layout.preferredHeight: 24
                        }
                    }

                    onClicked: changeCard.minimized = !changeCard.minimized
                }
            }
        }

        Item {
            clip: true
            Layout.fillWidth: true
            Layout.minimumHeight: 84
            Layout.maximumHeight: 220
            Layout.preferredHeight: Math.max(Layout.minimumHeight, Math.min(Layout.maximumHeight,
                textArea.contentHeight + textArea.topPadding + textArea.bottomPadding + 42
                + attachmentFlow.height + (attachmentFlow.visible ? 12 : 0)))

            Behavior on Layout
            .
            preferredHeight {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutCubic
                }
            }

            Rectangle {
                anchors.fill: parent
                color: global.backSelected
                border.color: global.stroke
                border.width: 1
                radius: 6
            }

            Flow {
                id: attachmentFlow
                visible: attachmentModel.count > 0
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                anchors.leftMargin: 10; anchors.rightMargin: 10; anchors.topMargin: visible ? 8 : 0
                spacing: 4
                height: visible ? implicitHeight : 0

                add: Transition {
                    ParallelAnimation {
                        NumberAnimation {
                            property: "opacity"
                            from: 0;
                            to: 1
                            duration: 160
                            easing.type: Easing.OutCubic
                        }
                        NumberAnimation {
                            property: "scale"
                            from: 0.92;
                            to: 1
                            duration: 160
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                move: Transition {
                    NumberAnimation {
                        properties: "x,y"
                        duration: 180
                        easing.type: Easing.OutCubic
                    }
                }

                Repeater {
                    model: attachmentModel

                    delegate: Rectangle {
                        id: attachmentChip
                        required property int index
                        required property url attachmentUrl
                        required property string fileName
                        required property var iconSource
                        implicitWidth: Math.min(attachmentContent.implicitWidth, 220)
                        implicitHeight: 24
                        color: global.backSelected
                        border.color: global.stroke
                        border.width: 1
                        radius: 4

                        RowLayout {
                            id: attachmentContent
                            anchors.fill: parent
                            spacing: 0

                            Button {
                                id: attachmentButton
                                leftPadding: 4; rightPadding: 4; topPadding: 0; bottomPadding: 0
                                implicitWidth: attachmentButtonContent.implicitWidth + leftPadding + rightPadding
                                implicitHeight: attachmentButtonContent.implicitHeight
                                flat: true
                                Layout.fillWidth: true; Layout.fillHeight: true

                                contentItem: RowLayout {
                                    id: attachmentButtonContent
                                    spacing: 4

                                    Image {
                                        source: attachmentChip.iconSource
                                        sourceSize.width: 16; sourceSize.height: 16
                                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                    }

                                    Label {
                                        text: attachmentChip.fileName
                                        elide: Text.ElideRight
                                        Layout.maximumWidth: 160
                                    }
                                }

                                onClicked: documentModule.documentOpen(attachmentChip.attachmentUrl)
                            }

                            Button {
                                leftPadding: 0; rightPadding: 4; topPadding: 0; bottomPadding: 0
                                flat: true
                                icon.source: "qrc:/icon/dismiss.svg"
                                icon.width: 12; icon.height: 12
                                Layout.preferredWidth: 24; Layout.fillHeight: true

                                onClicked: agentModule.attachmentRemove(attachmentChip.attachmentUrl)
                            }
                        }
                    }
                }
            }

            ScrollView {
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                anchors.top: attachmentFlow.bottom
                anchors.topMargin: attachmentFlow.visible ? 4 : 0
                bottomPadding: 42

                ScrollBar.vertical: ScrollBar {
                    x: parent.mirrored ? 0 : parent.width - width
                    y: parent.topPadding
                    height: parent.availableHeight
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                TextArea {
                    id: textArea
                    padding: 10
                    topPadding: 12
                    placeholderText: agentModule.state === 0 ? qsTr("Ask a question or describe a task") : qsTr("Steer the current task")
                    textFormat: TextEdit.PlainText
                    verticalAlignment: TextEdit.AlignTop
                    wrapMode: TextEdit.Wrap
                    background: null
                    ContextMenu.menu: null

                    Keys.onPressed: (event) => {
                        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                            rootItem.requestSubmit()
                            event.accepted = true
                        }
                    }
                }
            }

            RowLayout {
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                anchors.leftMargin: 7; anchors.rightMargin: 7; anchors.bottomMargin: 7
                spacing: 4

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/add.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 28; Layout.preferredHeight: 28

                    onClicked: attachmentDialog.open()
                }

                Button {
                    id: strategyButton
                    property int strategy: 0
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    checkable: true
                    checked: strategy === 1
                    enabled: agentModule.state === 0 && agentModule.goalState === 0
                    flat: true
                    icon.source: checked ? "qrc:/icon/team.svg" : "qrc:/icon/solo.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 28; Layout.preferredHeight: 28

                    onClicked: agentModule.conversationStrategySet(checked ? 1 : 0)

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) mainToolTip.text = ""
                        }
                        onPointChanged: {
                            mainToolTip.position = parent.mapToGlobal(point.position)
                            mainToolTip.text = strategyButton.strategy === 0
                                ? qsTr("Solo mode\nA general agent completes the task directly.\nClick to switch to Team mode.")
                                : qsTr("Team mode\nA supervisor coordinates specialized agents.\nClick to switch to Solo mode.")
                        }
                    }
                }

                Button {
                    id: modeButton
                    property int mode: 0
                    leftPadding: 4; rightPadding: 4; topPadding: 0; bottomPadding: 0
                    enabled: agentModule.state === 0 && agentModule.goalState === 0 && mode >= 0
                    flat: true
                    text: mode === 0 ? qsTr("Chat") :
                            mode === 1 ? qsTr("Read") :
                                mode === 2 ? qsTr("Write") :
                                    mode === 3 ? qsTr("Full access") : ""
                    icon.source: mode === 0 ? "qrc:/icon/chat.svg" :
                            mode === 1 ? "qrc:/icon/eye.svg" :
                                mode === 2 ? "qrc:/icon/edit.svg" :
                                    mode === 3 ? "qrc:/icon/lockOpen.svg" : ""
                    icon.color: !enabled ? global.foreDisabled : mode === 3 ? global.warningFore3 : global.fore
                    icon.width: 16; icon.height: 16
                    palette.buttonText: !enabled ? global.foreDisabled : mode === 3 ? global.warningFore3 : global.fore
                    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
                    Layout.preferredHeight: 28

                    onClicked: {
                        const globalPos = modeButton.mapToGlobal(0, -modeMenu.implicitHeight - 4);
                        const localPos = modeMenu.parent.mapFromGlobal(globalPos.x, globalPos.y);
                        modeMenu.popup(localPos.x, localPos.y)
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    id: modelButton
                    leftPadding: 4; rightPadding: 4; topPadding: 0; bottomPadding: 0
                    enabled: agentModule.state === 0 && agentModule.goalState === 0
                    flat: true
                    text: qsTr("Select model")
                    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
                    Layout.preferredHeight: 28

                    onClicked: {
                        const globalPos = modelButton.mapToGlobal(0, -modelMenu.implicitHeight - 4);
                        const localPos = modelMenu.parent.mapFromGlobal(globalPos.x, globalPos.y);
                        modelMenu.popup(localPos.x, localPos.y)
                    }
                }

                Flipable {
                    id: usageLayout
                    property double currentUsage: 0
                    property double contextWindow: 0
                    property bool compactPending: false
                    visible: contextWindow > 0
                    Layout.leftMargin: 4; Layout.rightMargin: 4
                    Layout.preferredWidth: Math.max(usageContent.implicitWidth, compactButton.implicitWidth)
                    Layout.preferredHeight: 28

                    function formatTokens(tokens: double): string {
                        if (tokens >= 1000000) return (tokens / 1000000).toFixed(tokens % 1000000 === 0 ? 0 : 1) + "M"
                        if (tokens >= 1000) return (tokens / 1000).toFixed(tokens % 1000 === 0 ? 0 : 1) + "k"
                        return tokens.toString()
                    }

                    function exactTokens(tokens: double): string {
                        return tokens > 0 ? Number(tokens).toLocaleString(Qt.locale(), "f", 0) : "-"
                    }

                    front: Item {
                        anchors.fill: parent

                        RowLayout {
                            id: usageContent
                            anchors.centerIn: parent
                            spacing: 4

                            FlipLabel {
                                id: contextLabel
                                text: usageLayout.currentUsage > 0
                                    ? usageLayout.formatTokens(usageLayout.currentUsage)
                                    : "-"
                            }

                            Label {
                                text: "/"
                                color: global.stroke
                                verticalAlignment: Text.AlignVCenter
                                Layout.preferredHeight: 28
                            }

                            Label {
                                text: usageLayout.formatTokens(usageLayout.contextWindow)
                                color: global.stroke
                                verticalAlignment: Text.AlignVCenter
                                Layout.preferredHeight: 28
                            }
                        }
                    }

                    back: RowLayout {
                        id: compactButton
                        anchors.centerIn: parent
                        spacing: 5

                        IconImage {
                            color: global.warningFore3
                            source: "qrc:/icon/arrowMinimize.svg"
                            sourceSize.width: 16; sourceSize.height: 16
                            Layout.preferredWidth: 16; Layout.preferredHeight: 16
                        }

                        Label {
                            id: compactLabel
                            text: qsTr("Compact")
                            color: global.warningFore3
                        }
                    }

                    transform: Rotation {
                        origin.x: usageLayout.width / 2
                        origin.y: usageLayout.height / 2
                        axis.x: 0; axis.y: 1; axis.z: 0
                        angle: usageLayout.compactPending ? -180 : 0

                        Behavior on angle {
                            NumberAnimation {
                                duration: 200
                                easing.type: Easing.InOutCubic
                            }
                        }
                    }

                    TapHandler {
                        enabled: agentModule.state === 0 && usageLayout.currentUsage > 0

                        onTapped: {
                            if (!usageLayout.compactPending) {
                                usageLayout.compactPending = true
                                compactTimer.restart()
                            } else {
                                compactTimer.stop()
                                usageLayout.compactPending = false
                                agentModule.compact()
                            }
                        }
                    }

                    Timer {
                        id: compactTimer
                        interval: 1500

                        onTriggered: usageLayout.compactPending = false
                    }

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) mainToolTip.text = ""
                        }
                        onPointChanged: {
                            mainToolTip.position = parent.mapToGlobal(point.position)
                            const action = usageLayout.currentUsage > 0 ? qsTr("Click to compact context") : qsTr("Context will update after the next response")
                            mainToolTip.text = qsTr("Context: %1 / %2\n%3")
                                .arg(usageLayout.exactTokens(usageLayout.currentUsage))
                                .arg(usageLayout.exactTokens(usageLayout.contextWindow))
                                .arg(action)
                        }
                    }
                }

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    enabled: agentModule.state === 0 && agentModule.goalState === 0 && chatColumn.children.length > 0
                    flat: true
                    icon.source: "qrc:/icon/undo.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 28; Layout.preferredHeight: 28

                    onClicked: agentModule.conversationRollback()
                }

                Button {
                    id: micButton
                    visible: false
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    checkable: true
                    flat: true
                    icon.source: "qrc:/icon/mic.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 28; Layout.preferredHeight: 28
                }

                Button {
                    id: sendButton
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    enabled: agentModule.state !== 0 || textArea.text.trim().length > 0
                    icon.source: agentModule.state === 0 || textArea.text.trim().length > 0 ? "qrc:/icon/send.svg" : "qrc:/icon/stop.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 28; Layout.preferredHeight: 28

                    onClicked: rootItem.requestSubmit()
                }
            }
        }
    }

    Component {
        id: turnComponent

        ColumnLayout {
            id: turnItem
            Layout.fillWidth: true
            Layout.preferredWidth: chatColumn.width
            spacing: 6
            property string turnId
            property double startedAt
            property double finishedAt: 0
            property int elapsedSeconds: 0
            property bool collapsed: false
            property string prompt
            property string response
            property string lastAssistantId
            property string lastActivityType
            property var activityItem
            property var currentToolGroup
            property alias subagents: subagentColumn
            property alias messages: messageColumn
            readonly property bool running: finishedAt === 0

            function elapsedUpdate(): void {
                const end = finishedAt === 0 ? Date.now() : finishedAt
                elapsedSeconds = Math.max(0, Math.floor((end - startedAt) / 1000))
            }

            function durationText(): string {
                if (elapsedSeconds < 60) return elapsedSeconds + "s"
                return Math.floor(elapsedSeconds / 60) + "m " + elapsedSeconds % 60 + "s"
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    verticalAlignment: Text.AlignVCenter
                    text: (turnItem.running ? "Working for " : "Worked for ") + turnItem.durationText()
                    Layout.preferredHeight: 24
                }

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: turnItem.collapsed ? "qrc:/icon/arrowCollapse.svg" : "qrc:/icon/arrowExpand.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    onClicked: turnItem.collapsed = !turnItem.collapsed
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                color: global.stroke
                Layout.fillWidth: true; Layout.preferredHeight: 1
            }

            ColumnLayout {
                id: turnContent
                Layout.fillWidth: true; Layout.preferredWidth: chatColumn.width
                spacing: 6

                ColumnLayout {
                    id: subagentColumn
                    visible: !turnItem.collapsed
                    Layout.fillWidth: true; Layout.preferredWidth: chatColumn.width
                    spacing: 6
                }

                ColumnLayout {
                    id: messageColumn
                    Layout.fillWidth: true; Layout.preferredWidth: chatColumn.width
                }
            }

            Timer {
                interval: 1000
                repeat: true
                running: turnItem.running
                triggeredOnStart: true
                onTriggered: turnItem.elapsedUpdate()
            }

            onFinishedAtChanged: elapsedUpdate()
            Component.onCompleted: elapsedUpdate()
        }
    }

    Component {
        id: activityComponent

        RowLayout {
            id: activityItem
            Layout.fillWidth: true; Layout.preferredHeight: 24
            spacing: 6
            property string activity: "thinking"
            property int reconnectAttempt: 0
            property int reconnectLimit: 0
            readonly property string activityText: activity === "reconnecting"
                                                   ? qsTr("Reconnecting %1 / %2...").arg(reconnectAttempt).arg(reconnectLimit)
                                                   : activity === "compacting" ? qsTr("Compacting context...")
                                                   : qsTr("Thinking...")
            readonly property url activityIcon: activity === "reconnecting" ? "qrc:/icon/wifiOff.svg" :
                                                activity === "compacting" ? "qrc:/icon/arrowMinimize.svg" :
                                                "qrc:/icon/thinking.svg"

            Item {
                id: activityVisual
                Layout.preferredWidth: activityContent.implicitWidth
                Layout.preferredHeight: 24

                RowLayout {
                    id: activityContent
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    IconImage {
                        color: global.stroke
                        source: activityItem.activityIcon
                        sourceSize.width: 16; sourceSize.height: 16
                        Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    }

                    Label {
                        text: activityItem.activityText
                        color: global.stroke
                    }
                }

                Item {
                    id: activityHighlightSource
                    anchors.fill: parent
                    visible: false
                    layer.enabled: true

                    RowLayout {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6

                        IconImage {
                            color: global.fore
                            source: activityItem.activityIcon
                            sourceSize.width: 16; sourceSize.height: 16
                            Layout.preferredWidth: 16; Layout.preferredHeight: 16
                        }

                        Label {
                            text: activityItem.activityText
                            color: global.fore
                        }
                    }
                }

                Item {
                    id: activityHighlightMask
                    anchors.fill: parent
                    visible: false
                    layer.enabled: true

                    Rectangle {
                        id: activitySweep
                        width: 32
                        height: parent.height
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: "transparent" }
                            GradientStop { position: 0.5; color: "white" }
                            GradientStop { position: 1; color: "transparent" }
                        }

                        SequentialAnimation on x {
                            running: activityItem.visible
                            loops: Animation.Infinite

                            NumberAnimation {
                                from: -activitySweep.width
                                to: activityVisual.width
                                duration: 1600
                                easing.type: Easing.InOutCubic
                            }

                            PauseAnimation {
                                duration: 500
                            }
                        }
                    }
                }

                MultiEffect {
                    anchors.fill: parent
                    source: activityHighlightSource
                    maskEnabled: true
                    maskSource: activityHighlightMask
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    Component {
        id: toolGroupComponent

        ColumnLayout {
            id: toolGroup
            Layout.fillWidth: true
            spacing: 2
            visible: !turn.collapsed
            property var turn
            property int count
            property bool expanded: false
            property alias messages: toolColumn

            RowLayout {
                Layout.fillWidth: true; Layout.preferredHeight: 24
                spacing: 4

                Label {
                    text: toolGroup.count === 1 ? qsTr("Used 1 tool") : qsTr("Used %1 tools").arg(toolGroup.count)
                    color: global.stroke
                }

                IconImage {
                    color: global.stroke
                    source: toolGroup.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                    sourceSize.width: 16; sourceSize.height: 16
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16

                    TapHandler {
                        onTapped: toolGroup.expanded = !toolGroup.expanded
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                id: toolColumn
                visible: toolGroup.expanded
                Layout.fillWidth: true
                spacing: 2
            }
        }
    }

    Component {
        id: subagentComponent

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            property string runtimeId
            property string role
            property string message

            IconImage {
                color: global.fore
                source: role === "data" ? "qrc:/icon/database.svg" :
                        role === "hardware" ? "qrc:/icon/hardware.svg" :
                            role === "vision" ? "qrc:/icon/eye.svg" : "qrc:/icon/code.svg"
                sourceSize.width: 24; sourceSize.height: 24
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Layout.alignment: Qt.AlignTop
            }

            Label {
                text: message
                color: global.fore
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.NoWrap
                Layout.fillWidth: true; Layout.preferredHeight: 24
            }
        }
    }

    Component {
        id: chatComponent

        ColumnLayout {
            id: chatItem
            spacing: 4
            visible: (contentBuffer.length > 0 || attachments.length > 0) && (!turn.collapsed || role === "user" || role === "assistant")
            Layout.preferredWidth: role === "assistant" || role === "comment" || role === "tool"
                                   ? chatView.availableWidth
                                   : Math.min(chatView.availableWidth * 0.8, implicitWidth)
            Layout.alignment: role === "user" || role === "steering" ? Qt.AlignRight : Qt.AlignLeft
            property var turn
            property string messageId
            property string role
            property string contentBuffer
            property var attachments: []

            function flush(): void {
                markdownModel.flush()
            }

            MarkdownModel {
                id: markdownModel
                source: chatItem.contentBuffer
            }

            Control {
                id: messageBody
                visible: chatItem.contentBuffer.length > 0
                leftPadding: chatItem.role === "user" || chatItem.role === "steering" ? 12 : 0
                rightPadding: leftPadding
                topPadding: chatItem.role === "user" || chatItem.role === "steering" ? 8 : 0
                bottomPadding: topPadding
                Layout.fillWidth: chatItem.role === "assistant" || chatItem.role === "comment" || chatItem.role === "tool"
                Layout.maximumWidth: chatItem.role === "user" || chatItem.role === "steering"
                                     ? chatView.availableWidth * 0.8
                                     : chatView.availableWidth
                Layout.alignment: chatItem.role === "user" || chatItem.role === "steering" ? Qt.AlignRight : Qt.AlignLeft

                background: Rectangle {
                    color: chatItem.role === "user" || chatItem.role === "steering" ? global.backSelected :
                            chatItem.role === "comment" || chatItem.role === "assistant" ? "transparent" :
                                chatItem.role === "tool" ? "transparent" : global.dangerBack2
                    radius: 16
                }

                contentItem: ColumnLayout {
                    spacing: 8

                    Repeater {
                        model: markdownModel
                        delegate: DelegateChooser {
                            role: "type"

                            DelegateChoice {
                                roleValue: MarkdownModel.Markdown
                                delegate: TextArea {
                                    id: markdownText
                                    required property string content
                                    text: content
                                    readOnly: true
                                    textFormat: TextEdit.MarkdownText
                                    wrapMode: Text.Wrap
                                    ContextMenu.menu: null
                                    color: chatItem.role === "tool" ? global.stroke : global.fore
                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                    background: null
                                    Layout.fillWidth: true

                                    HoverHandler {
                                        cursorShape: markdownText.hoveredLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                                    }

                                    TapHandler {
                                        acceptedButtons: Qt.LeftButton

                                        onTapped: {
                                            if (markdownText.hoveredLink) documentModule.documentOpen(markdownText.hoveredLink)
                                        }
                                    }

                                    TapHandler {
                                        acceptedButtons: Qt.RightButton

                                        onTapped: {
                                            if (!markdownText.hoveredLink) return
                                            mainLinkMenu.url = markdownText.hoveredLink
                                            mainLinkMenu.popup()
                                        }
                                    }
                                }
                            }

                            DelegateChoice {
                                roleValue: MarkdownModel.Code
                                delegate: Control {
                                    id: codeBlock
                                    required property string content
                                    required property string language
                                    padding: 0
                                    topPadding: 8
                                    Layout.fillWidth: true

                                    background: Rectangle {
                                        color: global.backSelected
                                        radius: 16
                                    }

                                    contentItem: ColumnLayout {
                                        spacing: 0

                                        RowLayout {
                                            Layout.fillWidth: true; Layout.preferredHeight: 24
                                            Layout.leftMargin: 12; Layout.rightMargin: 8
                                            spacing: 4

                                            IconImage {
                                                color: global.fore
                                                source: "qrc:/icon/code.svg"
                                                sourceSize.width: 16; sourceSize.height: 16
                                                Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                                Layout.alignment: Qt.AlignVCenter
                                            }

                                            Label {
                                                text: codeBlock.language || qsTr("Code")
                                                verticalAlignment: Text.AlignVCenter
                                                Layout.fillWidth: true; Layout.fillHeight: true
                                            }

                                            Button {
                                                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                                flat: true
                                                icon.source: "qrc:/icon/copy.svg"
                                                icon.width: 16; icon.height: 16
                                                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                                onClicked: fileModule.copyToClipboard(codeBlock.content)
                                            }
                                        }

                                        Flickable {
                                            id: codeFlickable
                                            clip: true
                                            interactive: false
                                            boundsBehavior: Flickable.StopAtBounds
                                            contentWidth: Math.max(width, codeText.contentWidth + codeText.leftPadding + codeText.rightPadding)
                                            contentHeight: codeText.implicitHeight
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: codeText.implicitHeight + (codeScrollBar.visible ? codeScrollBar.implicitHeight : 0)

                                            ScrollBar.horizontal: ScrollBar {
                                                id: codeScrollBar
                                                policy: ScrollBar.AsNeeded
                                                palette {
                                                    mid: global.stroke
                                                    dark: global.strokePressed
                                                }
                                            }

                                            TextArea {
                                                id: codeText
                                                width: codeFlickable.contentWidth
                                                text: codeBlock.content
                                                font.family: "Consolas"
                                                readOnly: true
                                                textFormat: TextEdit.PlainText
                                                wrapMode: TextEdit.NoWrap
                                                ContextMenu.menu: null
                                                color: global.fore
                                                leftPadding: 12; rightPadding: 12; topPadding: 8; bottomPadding: 8
                                                background: null
                                            }
                                        }
                                    }
                                }
                            }

                            DelegateChoice {
                                roleValue: MarkdownModel.Table
                                delegate: Control {
                                    id: tableBlock
                                    required property string content
                                    required property var tableModel
                                    padding: 0
                                    topPadding: 8
                                    clip: true
                                    Layout.fillWidth: true

                                    background: Rectangle {
                                        color: global.backSelected
                                        radius: 16
                                    }

                                    contentItem: ColumnLayout {
                                        spacing: 0

                                        RowLayout {
                                            Layout.fillWidth: true; Layout.preferredHeight: 24
                                            Layout.leftMargin: 12; Layout.rightMargin: 8
                                            spacing: 4

                                            IconImage {
                                                color: global.fore
                                                source: "qrc:/icon/table.svg"
                                                sourceSize.width: 16; sourceSize.height: 16
                                                Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                                Layout.alignment: Qt.AlignVCenter
                                            }

                                            Label {
                                                text: qsTr("Table")
                                                verticalAlignment: Text.AlignVCenter
                                                Layout.fillWidth: true; Layout.fillHeight: true
                                            }

                                            Button {
                                                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                                flat: true
                                                icon.source: "qrc:/icon/copy.svg"
                                                icon.width: 16; icon.height: 16
                                                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                                onClicked: fileModule.copyToClipboard(tableBlock.content)
                                            }
                                        }

                                        Rectangle {
                                            color: global.backSelected
                                            Layout.fillWidth: true
                                            Layout.leftMargin: 12; Layout.rightMargin: 12
                                            Layout.topMargin: 8; Layout.bottomMargin: 8
                                            Layout.preferredHeight: 32 + tableBlock.tableModel.rows * 32 + (tableScrollBar.visible ? tableScrollBar.implicitHeight : 0)

                                            ColumnLayout {
                                                anchors.fill: parent
                                                spacing: 0

                                                HorizontalHeaderView {
                                                    id: tableHeader
                                                    syncView: tableView
                                                    interactive: false
                                                    columnSpacing: 0
                                                    clip: true
                                                    Layout.fillWidth: true; Layout.preferredHeight: 32

                                                    delegate: HorizontalHeaderViewDelegate {
                                                        id: tableHeaderDelegate
                                                        implicitWidth: 160; implicitHeight: 32
                                                        padding: 0

                                                        background: Rectangle {
                                                            color: global.backHover

                                                            Rectangle {
                                                                anchors.right: parent.right
                                                                width: 1; height: parent.height
                                                                color: global.stroke
                                                                visible: tableHeaderDelegate.column < tableBlock.tableModel.columns - 1
                                                            }
                                                        }

                                                        contentItem: Label {
                                                            id: tableHeaderText
                                                            text: tableHeaderDelegate.model.display
                                                            textFormat: Text.MarkdownText
                                                            color: global.fore
                                                            font.bold: true
                                                            leftPadding: 8; rightPadding: 8
                                                            horizontalAlignment: tableHeaderDelegate.model.alignment
                                                            verticalAlignment: Text.AlignVCenter
                                                            elide: Text.ElideRight

                                                            HoverHandler {
                                                                cursorShape: tableHeaderText.hoveredLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                                                            }

                                                            TapHandler {
                                                                acceptedButtons: Qt.LeftButton

                                                                onTapped: {
                                                                    if (tableHeaderText.hoveredLink) documentModule.documentOpen(tableHeaderText.hoveredLink)
                                                                }
                                                            }

                                                            TapHandler {
                                                                acceptedButtons: Qt.RightButton

                                                                onTapped: {
                                                                    if (!tableHeaderText.hoveredLink) return
                                                                    mainLinkMenu.url = tableHeaderText.hoveredLink
                                                                    mainLinkMenu.popup()
                                                                }
                                                            }
                                                        }
                                                    }
                                                }

                                                TableView {
                                                    id: tableView
                                                    model: tableBlock.tableModel
                                                    alternatingRows: false
                                                    interactive: false
                                                    resizableColumns: true
                                                    rowSpacing: 0; columnSpacing: 0
                                                    boundsBehavior: Flickable.StopAtBounds
                                                    clip: true
                                                    Layout.fillWidth: true
                                                    Layout.preferredHeight: tableBlock.tableModel.rows * 32 + (tableScrollBar.visible ? tableScrollBar.implicitHeight : 0)

                                                    columnWidthProvider: function(column: int): real {
                                                        const explicitWidth = explicitColumnWidth(column)
                                                        if (explicitWidth >= 0) return explicitWidth
                                                        return Math.max(160, width / tableBlock.tableModel.columns)
                                                    }

                                                    ScrollBar.horizontal: ScrollBar {
                                                        id: tableScrollBar
                                                        policy: ScrollBar.AsNeeded
                                                        palette {
                                                            mid: global.stroke
                                                            dark: global.strokePressed
                                                        }
                                                    }

                                                    delegate: TableViewDelegate {
                                                        id: tableCell
                                                        implicitWidth: 160; implicitHeight: 32
                                                        padding: 0

                                                        background: Rectangle {
                                                            color: tableCell.row % 2 === 0 ? global.backSelected : global.backHover

                                                            Rectangle {
                                                                anchors.right: parent.right
                                                                width: 1; height: parent.height
                                                                color: global.stroke
                                                                visible: tableCell.column < tableBlock.tableModel.columns - 1
                                                            }
                                                        }

                                                        contentItem: Label {
                                                            id: tableCellText
                                                            text: tableCell.model.display
                                                            textFormat: Text.MarkdownText
                                                            color: global.fore
                                                            leftPadding: 8; rightPadding: 8
                                                            horizontalAlignment: tableCell.model.alignment
                                                            verticalAlignment: Text.AlignVCenter
                                                            elide: Text.ElideRight

                                                            HoverHandler {
                                                                cursorShape: tableCellText.hoveredLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                                                            }

                                                            TapHandler {
                                                                acceptedButtons: Qt.LeftButton

                                                                onTapped: {
                                                                    if (tableCellText.hoveredLink) documentModule.documentOpen(tableCellText.hoveredLink)
                                                                }
                                                            }

                                                            TapHandler {
                                                                acceptedButtons: Qt.RightButton

                                                                onTapped: {
                                                                    if (!tableCellText.hoveredLink) return
                                                                    mainLinkMenu.url = tableCellText.hoveredLink
                                                                    mainLinkMenu.popup()
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Flow {
                id: messageAttachmentFlow
                layoutDirection: Qt.RightToLeft
                spacing: 4
                visible: chatItem.attachments.length > 0
                Layout.preferredWidth: Math.min(implicitWidth, chatView.availableWidth * 0.8)
                Layout.preferredHeight: visible ? implicitHeight : 0
                Layout.alignment: Qt.AlignRight

                Repeater {
                    model: chatItem.attachments

                    delegate: Rectangle {
                        id: messageAttachmentChip
                        required property var modelData
                        implicitWidth: Math.min(messageAttachmentButton.implicitWidth, 220)
                        implicitHeight: 24
                        color: global.backSelected
                        border.color: global.stroke
                        border.width: 1
                        radius: 4

                        Button {
                            id: messageAttachmentButton
                            anchors.fill: parent
                            leftPadding: 4; rightPadding: 4; topPadding: 0; bottomPadding: 0
                            flat: true

                            contentItem: RowLayout {
                                spacing: 4

                                Image {
                                    source: messageAttachmentChip.modelData.iconSource
                                    sourceSize.width: 16; sourceSize.height: 16
                                    Layout.preferredWidth: 16; Layout.preferredHeight: 16
                                }

                                Label {
                                    text: messageAttachmentChip.modelData.fileName
                                    elide: Text.ElideRight
                                    Layout.maximumWidth: 160
                                }
                            }

                            onClicked: documentModule.documentOpen(messageAttachmentChip.modelData.attachmentUrl)
                        }
                    }
                }
            }

            RowLayout {
                id: messageInfoRow
                visible: chatItem.role === "user" || chatItem.role === "assistant"
                opacity: chatHover.hovered ? 1 : 0
                enabled: opacity > 0
                spacing: 2
                Layout.preferredHeight: visible ? 24 : 0
                Layout.alignment: chatItem.role === "user" ? Qt.AlignRight : Qt.AlignLeft

                Label {
                    text: Qt.formatDateTime(new Date(chatItem.role === "user" ? turn.startedAt : turn.finishedAt), "HH:mm")
                    color: global.stroke
                }

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/copy.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    onClicked: fileModule.copyToClipboard(chatItem.contentBuffer)
                }

                Behavior on opacity {
                    NumberAnimation {
                        duration: 150
                    }
                }
            }

            HoverHandler {
                id: chatHover
                cursorShape: Qt.IBeamCursor
            }
        }
    }

    SmoothedAnimation {
        id: followAnimation
        target: chatView.contentItem
        property: "contentY"
        duration: 160
        velocity: -1
        maximumEasingTime: 60
    }

    NumberAnimation {
        id: navigationAnimation
        target: chatView.contentItem
        property: "contentY"
        duration: 220
        easing.type: Easing.OutCubic
    }

    Connections {
        target: chatView.contentItem

        function onMovementStarted(): void {
            chatView.followTail = false
            rootItem.scrollStop()
        }

        function onMovementEnded(): void {
            chatView.followTail = chatView.contentItem.atYEnd
        }

        function onContentHeightChanged(): void {
            rootItem.followToTail()
        }
    }

    function attachmentAdd(documentUrl: url, fileName: string, iconSource: url): void {
        attachmentModel.append({
            "attachmentUrl": documentUrl,
            "fileName": fileName,
            "iconSource": iconSource
        })
    }

    function attachmentRemove(index: int): void {
        attachmentModel.remove(index)
    }

    function attachmentsClear(): void {
        attachmentModel.clear()
    }

    function followToTail(): void {
        if (!chatView.followTail) return
        const flickable = chatView.contentItem
        if (navigationAnimation.running) navigationAnimation.stop()
        const target = Math.max(flickable.originY, flickable.originY + flickable.contentHeight - flickable.height)
        followAnimation.stop()
        followAnimation.from = flickable.contentY
        followAnimation.to = target
        followAnimation.start()
    }

    function navigateTo(position: double): void {
        scrollStop()
        const flickable = chatView.contentItem
        const bottom = Math.max(flickable.originY, flickable.originY + flickable.contentHeight - flickable.height)
        const target = Math.max(flickable.originY, Math.min(position, bottom))
        chatView.followTail = target === bottom
        navigationAnimation.to = target
        navigationAnimation.restart()
    }

    function scrollStop(): void {
        followAnimation.stop()
        navigationAnimation.stop()
    }

    function turnCreate(turnId: string, startedAt: double): void {
        planCard.explanation = ""
        planCard.steps = []
        planCard.minimized = false
        changeCard.changes = ({})
        changeCard.minimized = true
        const obj = turnComponent.createObject(chatColumn, {
            turnId: turnId,
            startedAt: startedAt,
        })
        rootItem.turnMap[turnId] = obj
    }

    function turnFinish(turnId: string, finishedAt: double): void {
        const turn = rootItem.turnMap[turnId]
        if (turn.lastAssistantId) {
            rootItem.chatMap[turn.lastAssistantId].flush()
            rootItem.chatMap[turn.lastAssistantId].role = "assistant"
        }
        turn.finishedAt = finishedAt
        turn.collapsed = true
        planCard.minimized = true
        changeCard.minimized = false
    }

    function thinkingStart(turnId: string): void {
        activityStart(turnId, "thinking")
    }

    function compactStart(turnId: string): void {
        activityStart(turnId, "compacting")
    }

    function reconnectStart(turnId: string, attempt: int, limit: int): void {
        const activity = activityStart(turnId, "reconnecting")
        if (!activity) return
        activity.reconnectAttempt = attempt
        activity.reconnectLimit = limit
    }

    function activityStart(turnId: string, type: string): var {
        const turn = rootItem.turnMap[turnId] || chatColumn.children[chatColumn.children.length - 1]
        if (!turn) return null
        if (!turn.activityItem) turn.activityItem = activityComponent.createObject(turn.messages)
        turn.activityItem.activity = type
        return turn.activityItem
    }

    function activityFinish(turnId: string): void {
        const turn = rootItem.turnMap[turnId] || chatColumn.children[chatColumn.children.length - 1]
        if (!turn || !turn.activityItem) return
        turn.activityItem.destroy()
        turn.activityItem = null
    }

    function chatClear(): void {
        scrollStop()
        chatView.followTail = true
        planCard.explanation = ""
        planCard.steps = []
        planCard.minimized = true
        changeCard.changes = ({})
        changeCard.minimized = true
        requestsClear()
        usageUpdate(0)
        for (let i = chatColumn.children.length - 1; i >= 0; --i) {
            chatColumn.children[i].destroy();
        }
        rootItem.turnMap = ({})
        rootItem.chatMap = ({})
        rootItem.subagentMap = ({})
    }

    function chatCreate(turnId: string, messageId: string, role: string, attachments: var): void {
        const turn = rootItem.turnMap[turnId]
        if (role === "user" && turn.prompt) role = "steering"
        else if (role === "assistant") role = "comment"
        let container = turn.messages
        if (role === "tool") {
            if (turn.lastActivityType !== "tool") {
                turn.currentToolGroup = toolGroupComponent.createObject(turn.messages, {
                    turn: turn,
                })
            }
            container = turn.currentToolGroup.messages
            turn.currentToolGroup.count += 1
            turn.lastActivityType = "tool"
        } else if (role === "steering") {
            turn.lastActivityType = "steering"
        }
        const obj = chatComponent.createObject(container, {
            turn: turn,
            messageId: messageId,
            role: role,
            attachments: attachments,
        })
        rootItem.chatMap[messageId] = obj
    }

    function chatAppend(messageId: string, text: string): void {
        const chat = rootItem.chatMap[messageId]
        chat.contentBuffer += text
        if (chat.role === "user") chat.turn.prompt += text
        else if (chat.role === "comment") {
            chat.turn.lastAssistantId = messageId
            chat.turn.lastActivityType = "comment"
            chat.turn.currentToolGroup = null
            chat.turn.response = chat.contentBuffer
        }
    }

    function chatReset(messageId: string): void {
        const chat = rootItem.chatMap[messageId]
        chat.contentBuffer = ""
        chat.flush()
        chat.turn.response = ""
    }

    function subagentCreate(turnId: string, runtimeId: string, role: string, message: string): void {
        const turn = rootItem.turnMap[turnId]
        const obj = subagentComponent.createObject(turn.subagents, {
            runtimeId: runtimeId,
            role: role,
            message: message,
        })
        rootItem.subagentMap[runtimeId] = obj
    }

    function subagentUpdate(runtimeId: string, message: string): void {
        rootItem.subagentMap[runtimeId].message = message
    }

    function permissionRequest(runtimeId: string, role: string, message: string): void {
        permissionModel.append({
            "runtimeId": runtimeId,
            "role": role,
            "message": message
        })
        permissionSwipeView.currentIndex = permissionModel.count - 1
    }

    function permissionResponse(runtimeId: string, status: bool): void {
        agentModule.permission(runtimeId, status)
        for (let index = 0; index < permissionModel.count; ++index) {
            if (permissionModel.get(index).runtimeId !== runtimeId) continue
            permissionModel.remove(index)
            permissionSwipeView.currentIndex = Math.min(permissionSwipeView.currentIndex, permissionModel.count - 1)
            return
        }
    }

    function userInputRequest(runtimeId: string, role: string, request): void {
        const question = request && request.question ? request.question : ""
        const options = request && request.options ? request.options : []
        userInputModel.append({
            "runtimeId": runtimeId,
            "role": role,
            "question": question,
            "options": options
        })
        userInputSwipeView.currentIndex = userInputModel.count - 1
    }

    function userInputResponse(runtimeId: string, answer: string): void {
        agentModule.userInput(runtimeId, answer)
        for (let index = 0; index < userInputModel.count; ++index) {
            if (userInputModel.get(index).runtimeId !== runtimeId) continue
            userInputModel.remove(index)
            userInputSwipeView.currentIndex = Math.min(userInputSwipeView.currentIndex, userInputModel.count - 1)
            return
        }
    }

    function requestsClear(): void {
        permissionModel.clear()
        userInputModel.clear()
    }

    function planUpdate(plan): void {
        planCard.explanation = plan.explanation ? plan.explanation : ""
        planCard.steps = plan.plan ? plan.plan : []
    }

    function changeUpdate(changes): void {
        changeCard.changes = changes ? changes : ({})
    }

    function usageUpdate(totalTokens: double): void {
        usageLayout.currentUsage = totalTokens || 0
    }

    function modelUpdate(contextWindow: double): void {
        usageLayout.contextWindow = contextWindow || 0
    }

    Component.onCompleted: {
        const objects = {
            "conversationComboBox": conversationComboBox,
            "textArea": textArea,
            "strategyButton": strategyButton,
            "modeButton": modeButton,
            "modelButton": modelButton
        };
        agentModule.propertyGet(objects)
    }
}
