import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent
    property bool modelVisible: standardItemModel.rowCount() > 0

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    Timer {
        id: refreshTimer
        interval: 1000
        repeat: true

        onTriggered: global.refresh = true
    }

    Item {
        anchors.fill: parent
        visible: !modelVisible

        RowLayout {
            anchors.centerIn: parent

            Label {
                text: qsTr("Nothing watched.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            IconImage {
                source: "qrc:/icon/eye.svg"
                color: global.fore
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4

        RowLayout {

            Button {
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                checkable: true
                icon.source: "qrc:/icon/arrowRepeat.svg"
                icon.width: 16; icon.height: 16

                onToggled: {
                    if (checked) {
                        refreshTimer.start()
                    } else {
                        refreshTimer.stop()
                    }
                }

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = parent.checked ? qsTr("Disable Refresh") : qsTr("Enable Refresh")
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: modelVisible

            VerticalHeaderView {
                id: verticalHeaderView
                anchors.left: parent.left
                width: 24; height: parent.height
                syncView: tableView
                clip: true
                interactive: false
                movableRows: true
                delegate: VerticalHeaderViewDelegate {
                    id: verticalHeaderViewDelegate
                    implicitWidth: verticalHeaderView.width; implicitHeight: 24
                    padding: 0

                    contentItem: Rectangle {
                        width: 24; height: 24
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
                        watchModule.watchSwap(move.oldVisualIndex, move.newVisualIndex)
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
                    implicitHeight: 24
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
                        text: model.display
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
                        acceptedButtons: Qt.RightButton
                        gesturePolicy: TapHandler.DragWithinBounds

                        onTapped: {
                            if (column === 0) {
                                expressionMenu.watchUrl = model.whatsThis
                                expressionMenu.watchExpression = model.display
                                expressionMenu.popup()
                            } else if (column === 1) {
                                const expressionIndex = tableView.index(row, 0);
                                valueMenu.watchUrl = tableView.model.data(expressionIndex, Qt.WhatsThisRole)
                                valueMenu.watchExpression = tableView.model.data(expressionIndex, Qt.DisplayRole)
                                valueMenu.currentValue = model.display
                                valueMenu.currentType = model.whatsThis
                                valueMenu.popup()
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

    Connections {
        target: standardItemModel

        function onRowsInserted() {
            modelVisible = true
        }

        function onRowsRemoved() {
            modelVisible = standardItemModel.rowCount() > 0
        }

        function onModelReset() {
            modelVisible = false
        }
    }
}
