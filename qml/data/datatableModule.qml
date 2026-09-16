import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    Item {
        anchors.fill: parent
        visible: headerItemModel.empty

        RowLayout {
            anchors.centerIn: parent

            Button {
                flat: true
                text: qsTr("Click to create key.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter

                onClicked: datatableModule.datatableInsert()
            }

            IconImage {
                source: "qrc:/icon/table.svg"
                color: global.fore
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: !headerItemModel.empty

        HorizontalHeaderView {
            id: horizontalHeaderView
            anchors.left: verticalHeaderView.right; anchors.top: parent.top
            width: parent.width; height: 32
            model: headerItemModel
            syncView: tableView
            clip: true
            interactive: false
            movableColumns: true
            editTriggers: TableView.NoEditTriggers
            delegate: HorizontalHeaderViewDelegate {
                id: horizontalHeaderViewDelegate
                implicitWidth: 80; implicitHeight: 32
                padding: 0

                contentItem: Rectangle {
                    width: 80; height: 32
                    color: global.back

                    Label {
                        anchors.fill: parent
                        leftPadding: 6
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        text: model.display
                        elide: Text.ElideRight
                    }
                }

                HoverHandler {
                    onHoveredChanged: cursorShape = Qt.OpenHandCursor
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton

                    onDoubleTapped: horizontalHeaderView.edit(horizontalHeaderView.index(row, column))
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onSingleTapped: {
                        tableMenu.datatableKey = model.display
                        tableMenu.popup()
                    }
                }

                TableView.editDelegate: TextField {
                    anchors.fill: parent
                    leftPadding: 6
                    rightPadding: 6
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: display || ""
                    selectByMouse: true
                    property string oldKey
                    background: Rectangle {
                        color: global.backSelected
                    }

                    Component.onCompleted: {
                        oldKey = display || ""
                        forceActiveFocus()
                        selectAll()
                    }

                    TableView.onCommit: {
                        const view = horizontalHeaderView
                        const index = view.index(row, column)
                        if (datatableModule.datatableRename(oldKey, text) !== "") {
                            Qt.callLater(() => view.edit(index))
                        }
                    }
                }
            }
            property var moves: []

            Rectangle {
                anchors.fill: parent
                color: global.stroke
            }

            Timer {
                id: timer
                interval: 10
                onTriggered: {
                    let index = -1
                    let distance = -1
                    let currentDistance;
                    for (let i = 0; i < horizontalHeaderView.moves.length; ++i) {
                        let move = horizontalHeaderView.moves[i]
                        currentDistance = Math.abs(move.oldVisualIndex - move.newVisualIndex)
                        if (currentDistance > distance) {
                            distance = currentDistance
                            index = i
                        }
                    }
                    let move = horizontalHeaderView.moves[index]
                    const key = headerItemModel.data(headerItemModel.index(0, move.oldVisualIndex))
                    const targetKey = headerItemModel.data(headerItemModel.index(0, move.newVisualIndex))
                    datatableModule.datatableMove(key, targetKey)
                    horizontalHeaderView.clearColumnReordering()
                    tableView.clearColumnReordering()
                    horizontalHeaderView.moves = []
                }
            }

            onColumnMoved: (logicalIndex, oldVisualIndex, newVisualIndex) => {
                moves.push({oldVisualIndex, newVisualIndex})
                timer.restart()
            }
        }

        VerticalHeaderView {
            id: verticalHeaderView
            anchors.left: parent.left; anchors.top: horizontalHeaderView.bottom
            width: 40; height: parent.height
            syncView: tableView
            clip: true
            interactive: false
            delegate: VerticalHeaderViewDelegate {
                implicitWidth: 40; implicitHeight: 32
                padding: 0
                contentItem: Rectangle {
                    width: 40; height: 32
                    color: global.back

                    Label {
                        anchors.fill: parent
                        leftPadding: 6
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: row + 1
                        elide: Text.ElideRight
                    }
                }
            }

            Rectangle {
                anchors.fill: parent
                color: global.stroke
            }
        }

        TableView {
            id: tableView
            anchors.left: verticalHeaderView.right; anchors.right: parent.right
            anchors.top: horizontalHeaderView.bottom; anchors.bottom: parent.bottom
            alternatingRows: false
            clip: true
            rowSpacing: 1; columnSpacing: 1
            resizableColumns: true
            model: standardItemModel

            ScrollBar.horizontal: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, width))
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.stroke
                    dark: global.strokePressed
                }
            }

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
                implicitWidth: 80; implicitHeight: 32

                Rectangle {
                    anchors.fill: parent
                    color: global.back
                }

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

                Label {
                    id: label
                    anchors.fill: parent
                    leftPadding: 6
                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                    text: model.display || ""
                    elide: Text.ElideRight
                }

                HoverHandler {
                    id: hoverHandler
                }
            }

            TapHandler {
                acceptedButtons: Qt.RightButton

                onSingleTapped: rootMenu.popup()
            }
        }
    }

}
