import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Item {
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

            Label {
                text: qsTr("No active threads.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            IconImage {
                source: "qrc:/icon/snooze.svg"
                color: global.fore
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    HorizontalHeaderView {
        id: horizontalHeaderView
        anchors.top: parent.top
        width: parent.width; height: 32
        syncView: tableView
        clip: true
        interactive: false
        visible: !standardItemModel.empty
        delegate: HorizontalHeaderViewDelegate {
            id: horizontalHeaderViewDelegate
            required property int index

            background: Rectangle {
                color: "transparent"
                border.width: 0
            }

            contentItem: Label {
                anchors.fill: parent
                elide: Text.ElideRight
                leftPadding: 6
                horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                text: horizontalHeader[horizontalHeaderViewDelegate.index]
            }
        }
    }

    TableView {
        id: tableView
        anchors.top: horizontalHeaderView.bottom; anchors.bottom: parent.bottom
        width: parent.width
        alternatingRows: false
        clip: true
        editTriggers: TableView.NoEditTriggers
        rowSpacing: 1
        model: standardItemModel
        visible: !standardItemModel.empty
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

        delegate: DelegateChooser {
            DelegateChoice {
                column: 0
                delegate: iconCellDelegate
            }
            DelegateChoice {
                delegate: textCellDelegate
            }
        }

        Component {
            id: iconCellDelegate

            Item {
                implicitWidth: 12; implicitHeight: 24
                required property int row

                Rectangle {
                    anchors.fill: parent
                    color: global.back
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    topRightRadius: 0; bottomRightRadius: 0
                    color: global.backHover
                    opacity: tableView.hoveredRow === row ? 1 : 0
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 150
                        }
                    }
                }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    width: 3; height: 14
                    radius: 1.5
                    color: model.status === 0 ? global.successBack3 :
                           model.status === 1 ? global.warningBack3 :
                           model.status === 2 ? global.brandBack : global.dangerBack3
                }

                HoverHandler {
                    onHoveredChanged: {
                        if (hovered) {
                            tableView.hoveredRow = row
                        } else if (tableView.hoveredRow === row) {
                            tableView.hoveredRow = -1
                        }
                    }
                }
            }
        }

        Component {
            id: textCellDelegate

            Item {
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
                    radius: 6
                    topLeftRadius: 0; bottomLeftRadius: 0
                    topRightRadius: column === tableView.columns - 1 ? radius : 0
                    bottomRightRadius: column === tableView.columns - 1 ? radius : 0
                    color: global.back
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    topLeftRadius: 0; bottomLeftRadius: 0
                    topRightRadius: column === tableView.columns - 1 ? radius : 0
                    bottomRightRadius: column === tableView.columns - 1 ? radius : 0
                    color: global.backHover
                    opacity: tableView.hoveredRow === row ? 1 : 0
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
                    onHoveredChanged: {
                        if (hovered) {
                            tableView.hoveredRow = row
                        } else if (tableView.hoveredRow === row) {
                            tableView.hoveredRow = -1
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: {
                        threadMenu.threadId = model.threadId
                        threadMenu.popup()
                    }
                }
            }
        }
    }
}
