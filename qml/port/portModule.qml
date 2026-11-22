pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQml.Models

import UniComm

Item {
    id: root
    required property PortModule portModule
    anchors.fill: parent

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
                Accessible.name: qsTr("Add port")
                onClicked: root.portModule.portManage()

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = addButton.mapToGlobal(point.position)
                        mainTooltip.text = addButton.Accessible.name
                    }
                }
            }

            Button {
                id: editButton
                flat: true
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/edit.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: qsTr("Edit port")
                enabled: listView.currentItem !== null
                onClicked: root.portModule.portManage(listView.currentItem.index)

                HoverHandler {
                    onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
                    onPointChanged: {
                        mainTooltip.position = editButton.mapToGlobal(point.position)
                        mainTooltip.text = editButton.Accessible.name
                    }
                }
            }

            Button {
                id: deleteButton
                flat: true
                checkable: true
                enabled: listView.currentItem !== null
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/delete.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                Accessible.name: checked ? qsTr("Confirm deletion") : qsTr("Delete port")
                onToggled: { if (!checked) root.portModule.portRemove(listView.currentItem.modelData.name) }

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

        Item {
            visible: listView.count === 0
            Layout.fillWidth: true; Layout.fillHeight: true

            Button {
                anchors.centerIn: parent
                flat: true
                text: qsTr("Click to create port.")
                font.pixelSize: 16
                onClicked: root.portModule.portManage()
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: count > 0
            clip: true
            focusPolicy: Qt.ClickFocus
            keyNavigationEnabled: true
            model: visualModel
            onCurrentItemChanged: deleteButton.checked = false

            moveDisplaced: Transition {
                NumberAnimation {
                    property: "y"
                    duration: AnimationDuration.Reorder
                    easing.type: Easing.OutCubic
                }
            }

            ScrollBar.vertical: ScrollBar {
                id: scrollBar
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }
        }
    }

    DelegateModel {
        id: visualModel
        model: root.portModule.ports

        delegate: DropArea {
            id: row
            required property int index
            required property var modelData
            readonly property int visualIndex: DelegateModel.itemsIndex
            width: listView.width
            height: 28
            keys: ["port"]
            onEntered: drag => {
                const from = drag.source.visualIndex
                if (from !== visualIndex) visualModel.items.move(from, visualIndex)
            }

            ItemDelegate {
                id: entry
                anchors.left: parent.left; anchors.top: parent.top
                width: row.width; height: row.height
                leftPadding: 11; rightPadding: 6 + scrollBar.width
                topPadding: 0; bottomPadding: 0
                highlighted: row.ListView.isCurrentItem
                focusPolicy: Qt.ClickFocus
                onClicked: listView.currentIndex = row.visualIndex

                Drag.active: handle.drag.active
                Drag.source: row
                Drag.keys: ["port"]
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
                                listView.currentIndex = row.visualIndex
                            }
                            onReleased: {
                                const targetIndex = row.visualIndex
                                if (row.index >= 0 && row.index !== targetIndex) {
                                    root.portModule.portMove(row.index, targetIndex)
                                    listView.currentIndex = targetIndex
                                }
                            }
                            onCanceled: {
                                if (row.index >= 0 && row.visualIndex >= 0 && row.index !== row.visualIndex)
                                    visualModel.items.move(row.visualIndex, row.index)
                            }
                        }
                    }
                    Label {
                        text: row.modelData.name
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Switch {
                        checked: row.modelData.active
                        onClicked: {
                            listView.currentIndex = row.visualIndex
                            checked = Qt.binding(() => row.modelData.active)
                            root.portModule.portToggle(row.index)
                        }
                    }
                }

                states: State {
                    when: handle.pressed
                    ParentChange {
                        target: entry; parent: listView.contentItem
                    }
                    AnchorChanges {
                        target: entry; anchors.top: undefined; anchors.left: undefined
                    }
                    PropertyChanges {
                        entry.z: 2
                    }
                }
            }
        }
    }
}
