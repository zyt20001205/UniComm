pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import UniComm
import "../util"

Item {
    id: root
    anchors.fill: parent

    ListModel { id: draft }

    Connections {
        target: root.Window.window
        function onVisibleChanged() {
            if (root.Window.window.visible) {
                draft.clear()
                for (const terminal of global.terminalModule.terminals)
                    draft.append(terminal)
                listView.currentIndex = draft.count ? 0 : -1
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 0

            Button {
                id: addButton
                flat: true
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/addSquare.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: qsTr("Add terminal")

                onClicked: {
                    draft.append({id: "", name: "new", program: "", arguments: "", workingDirectory: "", environment: ""})
                    listView.currentIndex = draft.count - 1
                    listView.positionViewAtIndex(listView.currentIndex, ListView.Contain)
                }

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
                checkable: true
                enabled: draft.count > 0
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/delete.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: checked ? qsTr("Confirm deletion") : qsTr("Delete terminal")

                onToggled: { if (!checked) draft.remove(listView.currentIndex) }

                Timer {
                    interval: 1000
                    running: deleteButton.checked
                    onTriggered: deleteButton.checked = false
                }

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = deleteButton.mapToGlobal(point.position)
                        mainTooltip.text = deleteButton.Accessible.name
                    }
                }
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true
            focusPolicy: Qt.ClickFocus
            keyNavigationEnabled: true
            model: draft

            moveDisplaced: Transition {
                NumberAnimation {
                    property: "y"
                    duration: AnimationDuration.Reorder
                    easing.type: Easing.OutCubic
                }
            }

            onCurrentItemChanged: {
                deleteButton.checked = false
                if (!currentItem) return
                const terminal = currentItem.model
                nameField.text = terminal.name ?? ""
                programField.text = terminal.program ?? ""
                argumentsField.text = terminal.arguments ?? ""
                directoryField.text = terminal.workingDirectory ?? ""
                environmentField.text = terminal.environment ?? ""
            }

            ScrollBar.vertical: ScrollBar {
                id: scrollBar
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: DropArea {
                id: row
                required property int index
                required property var model
                width: listView.width
                height: 28
                keys: ["terminal"]
                onEntered: drag => {
                    const from = drag.source.index
                    if (from !== index) draft.move(from, index, 1)
                }

                ItemDelegate {
                    id: entry
                    anchors.left: parent.left; anchors.top: parent.top
                    width: row.width; height: row.height
                    leftPadding: 11; rightPadding: 6 + scrollBar.width
                    topPadding: 0; bottomPadding: 0
                    highlighted: row.ListView.isCurrentItem
                    focusPolicy: Qt.ClickFocus
                    onClicked: listView.currentIndex = row.index

                    Drag.active: handle.drag.active
                    Drag.source: row
                    Drag.keys: ["terminal"]
                    Drag.hotSpot.x: leftPadding + handle.width / 2
                    Drag.hotSpot.y: height / 2

                    contentItem: RowLayout {
                        spacing: 4
                        IconImage {
                            source: "qrc:/icon/drag.svg"
                            color: global.theme.general.fore
                            Layout.preferredWidth: 16; Layout.preferredHeight: 16
                            MouseArea {
                                id: handle
                                anchors.fill: parent
                                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                                preventStealing: true
                                drag.target: entry
                                drag.axis: Drag.YAxis
                                onPressed: {
                                    listView.forceActiveFocus()
                                    listView.currentIndex = row.index
                                }
                            }
                        }
                        Label {
                            text: row.model.name
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    states: State {
                        when: handle.pressed
                        ParentChange { target: entry; parent: listView.contentItem }
                        AnchorChanges { target: entry; anchors.top: undefined; anchors.left: undefined }
                        PropertyChanges { entry.z: 2 }
                    }
                }
            }
        }

        ColumnLayout {
            visible: listView.currentItem !== null
            Layout.fillWidth: true

            Label { text: qsTr("Name") }
            LineEdit {
                id: nameField
                Layout.fillWidth: true
                clearable: true
                onValueCommitted: draft.setProperty(listView.currentIndex, "name", text.trim())
            }

            Label { text: qsTr("Program") }
            LineEdit {
                id: programField
                Layout.fillWidth: true
                clearable: true
                browsable: true
                nameFilters: ["Executable files (*.exe)", "All files (*)"]
                onValueCommitted: draft.setProperty(listView.currentIndex, "program", text)
            }

            Label { text: qsTr("Arguments") }
            LineEdit {
                id: argumentsField
                Layout.fillWidth: true
                clearable: true
                onValueCommitted: draft.setProperty(listView.currentIndex, "arguments", text)
            }

            Label { text: qsTr("Working Directory") }
            LineEdit {
                id: directoryField
                Layout.fillWidth: true
                clearable: true
                fileMode: LineEdit.Folder
                onValueCommitted: draft.setProperty(listView.currentIndex, "workingDirectory", text)
            }

            Label { text: qsTr("Environment") }
            LineEdit {
                id: environmentField
                Layout.fillWidth: true
                clearable: true
                placeholderText: "NAME=value;NAME2=value"
                onValueCommitted: draft.setProperty(listView.currentIndex, "environment", text)
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Save and Exit")
            onClicked: {
                listView.forceActiveFocus()
                const terminals = []
                for (let i = 0; i < draft.count; ++i)
                    terminals.push(Object.assign({}, draft.get(i)))
                global.terminalModule.terminalSave(terminals)
            }
        }
    }
}
