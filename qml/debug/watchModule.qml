pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import "../util"

import UniComm

Item {
    id: root
    required property WatchModule watchModule
    anchors.fill: parent

    function loadEditor() {
        const index = treeView.selectionModel.currentIndex
        const result = root.watchModule.model.data(index, WatchModule.ResultRole)
        valueField.text = result?.value ?? ""
        typeBox.currentIndex = typeBox.find(result?.type ?? "")
        editor.writable = result?.writable === true
    }

    Connections {
        target: root.watchModule.model
        function onDataChanged(first, last) {
            const row = treeView.currentRow
            if (row >= first.row && row <= last.row) root.loadEditor()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Layout.fillHeight: false
            spacing: 0

            Button {
                id: addButton
                flat: true
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/addSquare.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: qsTr("Add Watch")
                onClicked: root.watchModule.addWatch(global.documentModule.focusedUrl, "")

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = addButton.mapToGlobal(point.position)
                        mainTooltip.text = addButton.Accessible.name
                    }
                }
            }

            Button {
                id: deleteButton
                flat: true
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/delete.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: qsTr("Delete watch")
                enabled: treeView.currentRow >= 0
                onClicked: {
                    root.watchModule.watchExpressionDelete(treeView.currentRow)
                    global.threadpoolModule.watchValuesGet(global.debugModule.currentThread)
                }

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = deleteButton.mapToGlobal(point.position)
                        mainTooltip.text = deleteButton.Accessible.name
                    }
                }
            }

            Button {
                id: refreshButton
                flat: true
                checkable: true
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/arrowRepeat.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: checked ? qsTr("Disable Refresh") : qsTr("Enable Refresh")

                Timer {
                    interval: 1000
                    repeat: true
                    running: refreshButton.checked && treeView.rows > 0
                             && global.debugModule.currentThread.length > 0 && global.debugModule.callStack.length === 0
                    triggeredOnStart: true
                    onTriggered: global.threadpoolModule.watchValuesGet(global.debugModule.currentThread)
                }

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = refreshButton.mapToGlobal(point.position)
                        mainTooltip.text = refreshButton.Accessible.name
                    }
                }
            }

            Button {
                id: detailsButton
                flat: true
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/moreVertical.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: qsTr("Watch details")
                checkable: true
                checked: true
                enabled: treeView.rows > 0

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = detailsButton.mapToGlobal(point.position)
                        mainTooltip.text = detailsButton.Accessible.name
                    }
                }
            }
        }

        SplitView {
            id: splitView
            Layout.fillWidth: true; Layout.fillHeight: true
            orientation: Qt.Horizontal

            background: Item {
                visible: treeView.rows === 0

                RowLayout {
                    anchors.centerIn: parent

                    Label {
                        text: qsTr("Nothing watched.")
                        font.pixelSize: 16
                    }

                    IconImage {
                        source: "qrc:/icon/eye.svg"
                        color: global.theme.general.fore
                    }
                }
            }

            handle: Item {
                implicitWidth: 5

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: 1
                    color: global.theme.general.stroke
                }
            }

            TreeView {
                id: treeView
                SplitView.fillWidth: true
                clip: true
                editTriggers: TableView.DoubleTapped | TableView.EditKeyPressed
                model: root.watchModule.model
                selectionModel: ItemSelectionModel {
                    onCurrentChanged: root.loadEditor()
                }
                Keys.onDeletePressed: deleteButton.click()

                ScrollBar.vertical: ScrollBar {
                    id: treeScrollBar
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    palette {
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
                    }
                }

                delegate: Item {
                    id: node
                    required property int depth
                    required property string display
                    required property var result
                    required property bool current
                    required property bool editing
                    implicitWidth: treeView.width; implicitHeight: 24

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: node.current ? global.theme.general.backSelected
                             : hoverHandler.hovered ? global.theme.general.backHover : "transparent"
                    }

                    RowLayout {
                        visible: !node.editing
                        anchors.fill: parent
                        anchors.rightMargin: 6 + treeScrollBar.width
                        spacing: 0

                        Item {
                            Layout.preferredWidth: node.depth * 24; Layout.preferredHeight: 24
                        }

                        Item {
                            Layout.preferredWidth: 24; Layout.preferredHeight: 24

                            IconImage {
                                anchors.centerIn: parent
                                width: 16; height: 16
                                source: "qrc:/icon/arrowCollapse.svg"
                                color: global.theme.general.fore
                            }
                        }

                        Label {
                            text: node.display
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            text: node.result.value ?? ""
                            Layout.maximumWidth: treeView.width / 2
                            elide: Text.ElideRight
                        }
                    }

                    HoverHandler { id: hoverHandler }

                    TableView.editDelegate: TextField {
                        required property int row
                        required property string display
                        anchors.fill: parent
                        anchors.rightMargin: treeScrollBar.width
                        topPadding: 0; bottomPadding: 0
                        text: display
                        placeholderText: qsTr("e.g. x + 1")
                        Component.onCompleted: selectAll()
                        TableView.onCommit: root.watchModule.watchExpressionSet(row, text)
                    }
                }
            }

            ScrollView {
                id: settingsView
                property real reveal: detailsButton.checked ? 1 : 0
                visible: treeView.rows > 0
                SplitView.preferredWidth: splitView.width - Math.min(240, splitView.width / 2)
                SplitView.maximumWidth: reveal === 1 ? splitView.width : Math.min(SplitView.preferredWidth, splitView.width) * reveal
                enabled: detailsButton.checked
                clip: true
                padding: 8
                rightPadding: padding + ScrollBar.vertical.width
                contentWidth: availableWidth

                Behavior on reveal {
                    NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                }

                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.minimumSize: Math.min(1, 20 / Math.max(1, ScrollBar.vertical.height))
                ScrollBar.vertical.palette.mid: global.theme.general.stroke
                ScrollBar.vertical.palette.dark: global.theme.general.strokePressed

                ColumnLayout {
                    id: editor
                    property bool writable: false
                    width: settingsView.availableWidth
                    visible: treeView.currentRow >= 0
                    enabled: writable && global.debugModule.callStack.length > 0

                    Label { text: qsTr("Value") }

                    LineEdit {
                        id: valueField
                        clearable: true
                        tooltip: mainTooltip
                        buttonSize: 24
                        Layout.fillWidth: true
                        onAccepted: applyButton.click()
                    }

                    Label { text: qsTr("Type") }

                    ComboBox {
                        id: typeBox
                        model: ["boolean", "number", "string"]
                        Layout.fillWidth: true

                        popup.parent: mainOverlay
                        popup.popupType: Popup.Item
                        popup.focus: true
                        popup.contentItem.focus: true
                        popup.contentItem.Keys.forwardTo: [typeBox]
                        popup.closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                        popup.topMargin: -1
                        popup.bottomMargin: -1
                        popup.onAboutToShow: {
                            const position = mainOverlay.mapFromGlobal(typeBox.mapToGlobal(0, typeBox.height))
                            popup.x = position.x
                            popup.y = position.y
                            global.mainWindow.overlayFlagSet(false, true)
                            mainOverlay.widgetCount += 1
                        }
                        popup.onClosed: mainOverlay.widgetCount -= 1
                    }

                    Button {
                        id: applyButton
                        text: qsTr("Apply")
                        enabled: typeBox.currentIndex >= 0
                        onClicked: {
                            const expression = treeView.model.data(treeView.selectionModel.currentIndex, Qt.DisplayRole)
                            global.threadpoolModule.watchValueSet(global.debugModule.currentThread, expression, valueField.text, typeBox.currentText)
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: expressionDialog
        parent: mainOverlay
        popupType: Popup.Item
        x: mainOverlay.mainGeometry.x + (mainOverlay.mainGeometry.width - width) / 2
        y: mainOverlay.mainGeometry.y + (mainOverlay.mainGeometry.height - height) / 2
        width: 600
        modal: true
        focus: true
        title: qsTr("Enter Watch")
        standardButtons: Dialog.Ok

        Connections {
            target: root.watchModule
            function onAddWatch(documentUrl, expression) {
                urlField.text = documentUrl
                expressionField.text = expression
                expressionDialog.open()
            }
        }

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
            expressionField.forceActiveFocus()
            expressionField.selectAll()
        }
        onClosed: mainOverlay.widgetCount -= 1
        onAccepted: {
            root.watchModule.watchExpressionAdd(urlField.text, expressionField.text)
            global.threadpoolModule.watchValuesGet(global.debugModule.currentThread)
        }

        ColumnLayout {
            width: parent.width

            Label { text: qsTr("Url") }

            TextField {
                id: urlField
                Layout.fillWidth: true
                placeholderText: qsTr("Enter url:")
                onAccepted: expressionDialog.accept()
                Keys.onEscapePressed: expressionDialog.reject()
            }

            Label { text: qsTr("Expression") }

            TextField {
                id: expressionField
                Layout.fillWidth: true
                placeholderText: qsTr("Enter expression:")
                onAccepted: expressionDialog.accept()
                Keys.onEscapePressed: expressionDialog.reject()
            }
        }
    }
}
