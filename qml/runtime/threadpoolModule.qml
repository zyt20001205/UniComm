pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import UniComm

Rectangle {
    id: root
    required property ThreadpoolModule threadpoolModule
    anchors.fill: parent
    color: global.theme.general.back

    RowLayout {
        anchors.centerIn: parent
        visible: listView.count === 0

        Label {
            text: qsTr("No active threads.")
            font.pixelSize: 16
        }

        IconImage {
            source: "qrc:/icon/snooze.svg"
            color: global.theme.general.fore
        }
    }

    Button {
        id: stopButton
        anchors.top: parent.top; anchors.right: parent.right
        width: 24; height: 24
        flat: true
        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
        icon.source: "qrc:/icon/stop.svg"
        icon.width: 16; icon.height: 16
        Accessible.name: qsTr("Terminate")
        enabled: listView.currentItem !== null && listView.currentItem.thread.status !== ThreadStatus.Terminating
        onClicked: root.threadpoolModule.threadStatusSet(listView.currentItem.threadId, ThreadStatus.Terminating)

        HoverHandler {
            onHoveredChanged: { if (!hovered) mainTooltip.text = "" }
            onPointChanged: {
                mainTooltip.position = stopButton.mapToGlobal(point.position)
                mainTooltip.text = stopButton.Accessible.name
            }
        }
    }

    ListView {
        id: listView
        anchors.left: parent.left; anchors.right: parent.right
        anchors.top: stopButton.bottom; anchors.bottom: parent.bottom
        clip: true
        model: Object.entries(root.threadpoolModule.threads)
        currentIndex: -1
        focusPolicy: Qt.ClickFocus
        keyNavigationEnabled: true

        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBar
            minimumSize: Math.min(1, 20 / Math.max(1, height))
            palette {
                mid: global.theme.general.stroke
                dark: global.theme.general.strokePressed
            }
        }

        delegate: ItemDelegate {
            id: row
            required property int index
            required property var modelData
            readonly property string threadId: modelData[0]
            readonly property var thread: modelData[1]
            width: listView.width
            height: 28
            leftPadding: 11; rightPadding: 6 + verticalScrollBar.width
            topPadding: 0; bottomPadding: 0
            focusPolicy: Qt.ClickFocus
            highlighted: ListView.isCurrentItem
            onClicked: listView.currentIndex = index

            contentItem: RowLayout {
                spacing: 8

                IconImage {
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    source: row.thread.status === ThreadStatus.Paused ? "qrc:/icon/pause.svg"
                          : row.thread.status === ThreadStatus.Terminating ? "qrc:/icon/stop.svg" : "qrc:/icon/play.svg"
                    color: row.thread.status === ThreadStatus.Paused ? global.theme.general.warningBack3
                         : row.thread.status === ThreadStatus.Terminating ? global.theme.general.dangerBack3 : global.theme.general.successBack3
                }
                Label {
                    text: row.threadId
                    Layout.preferredWidth: 110
                    elide: Text.ElideRight
                }
                Label {
                    text: row.thread.createdAt
                    Layout.preferredWidth: 165
                    elide: Text.ElideRight
                }
                Label {
                    text: row.thread.name
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

        }
    }
}
