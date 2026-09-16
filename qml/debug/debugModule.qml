import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4

        RowLayout {

            Button {
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/stop.svg"
                icon.width: 16; icon.height: 16

                onClicked: debugModule.stateSet(threadComboBox.currentText, 0)

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = qsTr("Terminate")
                    }
                }
            }

            Button {
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/play.svg"
                icon.width: 16; icon.height: 16

                onClicked: debugModule.stateSet(threadComboBox.currentText, 1)

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = qsTr("Resume")
                    }
                }
            }

            Button {
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/pause.svg"
                icon.width: 16; icon.height: 16

                onClicked: debugModule.stateSet(threadComboBox.currentText, 2)

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = qsTr("Pause")
                    }
                }
            }

            Button {
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/debugStepOver.svg"
                icon.width: 16; icon.height: 16

                onClicked: debugModule.stateSet(threadComboBox.currentText, 3)

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = qsTr("Step Over")
                    }
                }
            }

            Button {
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/debugStepInto.svg"
                icon.width: 16; icon.height: 16

                onClicked: debugModule.stateSet(threadComboBox.currentText, 4)

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = qsTr("Step Into")
                    }
                }
            }

            Button {
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                icon.source: "qrc:/icon/debugStepOut.svg"
                icon.width: 16; icon.height: 16

                onClicked: debugModule.stateSet(threadComboBox.currentText, 5)

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        mainToolTip.text = qsTr("Step Out")
                    }
                }
            }
        }

        ComboBox {
            id: threadComboBox
            Layout.fillWidth: true;
            Layout.alignment: Qt.AlignTop
            model: stringListModel
            textRole: "display"
            onCurrentTextChanged: debugModule.callStackSwitch(currentText)
        }

        TableView {
            id: tableView
            Layout.fillWidth: true; Layout.fillHeight: true;
            alternatingRows: false
            clip: true
            editTriggers: TableView.NoEditTriggers
            rowSpacing: 1
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
                    return Math.max(textMetrics.width + 16, 60)
                }
                implicitHeight: 24
                required property int column
                required property int row

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
                }

                HoverHandler {
                    id: hoverHandler
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: tableView.markerAdd(row)
                }
            }

            function markerAdd(row) {
                const index = model.index(row, 0);
                debugModule.markerAdd(model.data(index, Qt.WhatsThisRole))
            }
        }
    }

    Component.onCompleted: {
        const objects = {
            "threadComboBox": threadComboBox
        };
        debugModule.propertyGet(objects)
    }
}
