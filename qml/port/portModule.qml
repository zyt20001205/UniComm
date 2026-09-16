import QtGraphs
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent

    function formatBytes(value) {
        if (value < 1024) return value + " B"
        if (value < 1048576) return (value / 1024).toFixed(value < 10240 ? 1 : 0) + " KB"
        return (value / 1048576).toFixed(value < 10485760 ? 1 : 0) + " MB"
    }

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    Item {
        anchors.fill: parent
        visible: portModel.empty

        RowLayout {
            anchors.centerIn: parent

            Button {
                flat: true
                text: qsTr("Click to create port.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter

                onClicked: portModule.portSetting()
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: !portModel.empty

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
                        width: 16; height: 16
                        anchors.centerIn: parent
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
                    portModule.portMove(move.oldVisualIndex, move.newVisualIndex)
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
            editTriggers: TableView.NoEditTriggers
            rowSpacing: 1
            model: portModel
            contentWidth: width
            property int hoveredRow: -1

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
                id: portDelegate
                implicitWidth: tableView.width; implicitHeight: detailButton.checked ? 156 : 36

                Rectangle {
                    anchors.fill: parent
                    color: global.back
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: global.backHover
                    opacity: tableView.hoveredRow === row ? 1 : 0
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 150
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true; Layout.preferredHeight: 24

                        Label {
                            horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                            text: model.display || ""
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Button {
                            id: detailButton
                            enabled: model.active
                            checkable: true
                            flat: true
                            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                            icon.source:"qrc:/icon/moreHorizontal.svg"
                            icon.width: 16; icon.height: 16
                            Layout.preferredWidth: 24; Layout.preferredHeight: 24

                            onToggled: portModule.portMonitor(model.row, checked)

                            Binding {
                                target: detailButton
                                property: "checked"
                                when: !model.active
                                value: false
                                restoreMode: Binding.RestoreNone
                            }
                        }

                        Switch {
                            checked: model.active

                            onClicked: portModule.portToggle(model.row)
                        }
                    }

                    SplitView {
                        visible: detailButton.checked
                        orientation: Qt.Horizontal
                        Layout.fillWidth: true; Layout.preferredHeight: visible ? 120 : 0
                        onVisibleChanged: Qt.callLater(tableView.forceLayout)
                        handle: Item {
                            implicitWidth: 5

                            Rectangle {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: 1
                                color: global.stroke
                            }
                        }

                        ColumnLayout {
                            spacing: 0
                            SplitView.fillWidth: true; SplitView.fillHeight: true

                            RowLayout {
                                SplitView.fillWidth: true

                                Label {
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    text: qsTr("Lifetime: ")
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Label {
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    text: model.lifetime
                                    elide: Text.ElideRight
                                }
                            }

                            RowLayout {
                                SplitView.fillWidth: true

                                Label {
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    text: qsTr("Read: ")
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Label {
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    text: model.readCount + "/" + model.readBytes
                                    elide: Text.ElideRight
                                }
                            }

                            RowLayout {
                                SplitView.fillWidth: true

                                Label {
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    text: qsTr("Write: ")
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Label {
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    text: model.writeCount + "/" + model.writeBytes
                                    elide: Text.ElideRight
                                }
                            }

                            Item {
                                Layout.fillWidth: true; Layout.fillHeight: true
                            }
                        }

                        Item {
                            SplitView.preferredWidth: 120; SplitView.fillHeight: true

                            Label {
                                anchors.centerIn: parent
                                text: (usedSlice.percentage * 100).toFixed(1) + "%"
                            }

                            GraphsView {
                                anchors.fill: parent
                                marginLeft: 0; marginTop: 0; marginRight: 0; marginBottom: 0
                                theme: GraphsTheme {
                                    seriesColors: [global.brandBack, global.back]
                                    borderColors: [global.stroke]
                                    backgroundVisible: false
                                }

                                PieSeries {
                                    pieSize: 0.9
                                    holeSize: 0.7

                                    PieSlice {
                                        id: usedSlice
                                        value: model.used
                                        color: percentage < 0.3 ? global.successBack3
                                            : percentage < 0.6 ? global.warningBack3
                                                : global.dangerBack3
                                    }

                                    PieSlice {
                                        value: model.capacity - model.used
                                    }
                                }
                            }
                        }
                    }
                }

                HoverHandler {
                    id: hoverHandler
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onSingleTapped: {
                        tableMenu.portIndex = model.row
                        tableMenu.portName = model.display
                        tableMenu.popup()
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
