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

    Button {
        anchors.centerIn: parent
        visible: listView.count === 0
        flat: true
        text: qsTr("Click to create port.")
        font.pixelSize: 16
        onClicked: root.portModule.portManage()
    }

    ListView {
        id: listView
        anchors.fill: parent
        visible: count > 0
        clip: true
        focusPolicy: Qt.ClickFocus
        keyNavigationEnabled: true
        model: visualModel

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

        TapHandler {
            acceptedButtons: Qt.RightButton
            onSingleTapped: (eventPoint) => {
                const point = listView.contentItem.mapFromItem(listView, eventPoint.position)
                const row = listView.itemAt(point.x, point.y)
                if (row) {
                    contextMenu.portName = row.modelData.name
                    contextMenu.popup()
                } else {
                    rootMenu.popup()
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
                            checked = Qt.binding(() => row.modelData.active)
                            root.portModule.portToggle(row.index)
                        }
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

    Menu {
        id: contextMenu
        parent: mainOverlay
        popupType: Popup.Item
        property string portName
        readonly property int portIndex: root.portModule.ports.findIndex(port => port.name === portName)

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16
            onTriggered: root.portModule.portManage()
        }
        MenuItem {
            text: qsTr("Edit")
            icon.source: "qrc:/icon/edit.svg"
            icon.width: 16; icon.height: 16
            enabled: contextMenu.portIndex >= 0
            onTriggered: root.portModule.portManage(contextMenu.portIndex)
        }
        Menu {
            title: qsTr("Delete")
            icon.source: "qrc:/icon/delete.svg"
            icon.width: 16; icon.height: 16
            enabled: contextMenu.portIndex >= 0
            DelayButton {
                delay: 1000
                text: qsTr("Confirm")
                onActivated: {
                    root.portModule.portRemove(contextMenu.portName)
                    progress = 0
                    contextMenu.close()
                }
            }
        }
    }

    Menu {
        id: rootMenu
        parent: mainOverlay
        popupType: Popup.Item
        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("New")
            icon.source: "qrc:/icon/add.svg"
            icon.width: 16; icon.height: 16
            onTriggered: root.portModule.portManage()
        }
    }
}
