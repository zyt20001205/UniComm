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
        visible: standardItemModel.empty

        RowLayout {
            anchors.centerIn: parent

            Button {
                flat: true
                text: qsTr("Click to create key.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter

                onClicked: databaseModule.databaseInsert()
            }

            IconImage {
                source: "qrc:/icon/database.svg"
                color: global.fore
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: !standardItemModel.empty

        VerticalHeaderView {
            id: verticalHeaderView
            anchors.left: parent.left
            width: 32; height: parent.height
            syncView: tableView
            clip: true
            interactive: false
            movableRows: true
            delegate: VerticalHeaderViewDelegate {
                id: verticalHeaderViewDelegate
                implicitWidth: verticalHeaderView.width; implicitHeight: 32
                padding: 0

                contentItem: Rectangle {
                    width: 32; height: 32
                    color: global.back

                    IconImage {
                        anchors.centerIn: parent
                        width: 16; height: 16
                        color: global.fore
                        source: "qrc:/icon/drag.svg"
                    }
                }

                HoverHandler {
                    onHoveredChanged: cursorShape = Qt.OpenHandCursor
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
                    for (let i = 0; i < verticalHeaderView.moves.length; ++i) {
                        let move = verticalHeaderView.moves[i]
                        currentDistance = Math.abs(move.oldVisualIndex - move.newVisualIndex)
                        if (currentDistance > distance) {
                            distance = currentDistance
                            index = i
                        }
                    }
                    let move = verticalHeaderView.moves[index]
                    const key = standardItemModel.data(standardItemModel.index(move.oldVisualIndex, 0))
                    const targetKey = standardItemModel.data(standardItemModel.index(move.newVisualIndex, 0))
                    databaseModule.databaseMove(key, targetKey)
                    verticalHeaderView.clearRowReordering()
                    tableView.clearRowReordering()
                    verticalHeaderView.moves = []
                }
            }

            onRowMoved: (logicalIndex, oldVisualIndex, newVisualIndex) => {
                moves.push({oldVisualIndex, newVisualIndex})
                timer.restart()
            }
        }

        TableView {
            id: tableView
            anchors.left: verticalHeaderView.right; anchors.right: parent.right
            height: parent.height
            alternatingRows: false
            clip: true
            rowSpacing: 1; columnSpacing: 1
            resizableColumns: true
            editTriggers: TableView.NoEditTriggers
            model: standardItemModel
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
                implicitWidth: {
                    if (column === tableView.columns - 1) {
                        let usedWidth = 0
                        for (let i = 0; i < tableView.columns - 1; i++) {
                            usedWidth += tableView.columnWidth(i)
                        }
                        return tableView.width - usedWidth
                    }
                    return Math.max(textMetrics.width + 12, 60)
                }
                implicitHeight: 32
                required property int column
                required property int row
                property bool valueChanged: false

                Rectangle {
                    anchors.fill: parent
                    color: global.back
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: global.backHover
                    opacity: {
                        if (column === 0 && hoverHandler.hovered) {
                            return 1
                        } else if (column === 1 && valueChanged) {
                            return 1
                        } else {
                            return 0
                        }
                    }
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 150
                        }
                    }
                }

                TextMetrics {
                    id: textMetrics
                    font: label.font
                    text: model.display || ""
                }

                Label {
                    id: label
                    anchors.fill: parent
                    leftPadding: 6
                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                    text: model.display || ""
                    elide: Text.ElideRight

                    onTextChanged: {
                        if (column === 1) {
                            valueChanged = true
                            highlightTimer.restart()
                        }
                    }
                }

                Timer {
                    id: highlightTimer
                    interval: 500

                    onTriggered: valueChanged = false
                }

                HoverHandler {
                    id: hoverHandler
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onDoubleTapped: {
                        if (column === 0) tableView.edit(tableView.index(row, column))
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onSingleTapped: {
                        tableMenu.databaseKey = standardItemModel.data(standardItemModel.index(model.row, 0))
                        tableMenu.popup()
                    }
                }

                TableView.editDelegate: TextField {
                    anchors.fill: parent
                    leftPadding: 6
                    rightPadding: 6
                    verticalAlignment: Text.AlignVCenter
                    text: display || ""
                    selectByMouse: true
                    property string oldKey
                    background: Rectangle {
                        color: global.backSelected
                        radius: 6
                    }

                    Component.onCompleted: {
                        oldKey = display || ""
                        forceActiveFocus()
                        selectAll()
                    }

                    TableView.onCommit: {
                        const view = tableView
                        const index = view.index(row, column)
                        if (databaseModule.databaseRename(oldKey, text) !== "") {
                            Qt.callLater(() => view.edit(index))
                        }
                    }
                }
            }

            TapHandler {
                acceptedButtons: Qt.RightButton

                onSingleTapped: rootMenu.popup()
            }
        }
    }
}
