import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQml.Models

import "../../util"

import UniComm

Item {
    id: root
    required property GitCommitModule gitCommitModule

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    TreeView {
        id: treeView
        anchors.fill: parent
        clip: true
        reuseItems: false
        model: root.gitCommitModule.changes
        selectionModel: ItemSelectionModel { model: root.gitCommitModule.changes }

        // Expand groups when they first gain files.
        Instantiator {
            model: root.gitCommitModule.changes
            delegate: QtObject {
                required property int index
                required property int fileCount
                readonly property bool populated: fileCount > 0
                onPopulatedChanged: {
                    if (populated) Qt.callLater(() => treeView.expand(treeView.rowAtIndex(treeView.model.index(index, 0))))
                }
            }
        }

        ScrollBar.vertical: ScrollBar {
            id: treeScrollBar
            minimumSize: Math.min(1, 20 / Math.max(1, height))
            palette {
                mid: global.theme.general.stroke
                dark: global.theme.general.strokePressed
            }
        }

        delegate: Item {
            id: node
            implicitWidth: treeView.width; implicitHeight: 24
            required property var model
            required property bool current
            required property bool expanded
            required property bool hasChildren
            required property int depth
            required property int row
            readonly property bool group: depth === 0

            Rectangle {
                anchors.fill: parent
                radius: 6
                color: global.theme.general.backHover
                opacity: hoverHandler.hovered ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: AnimationDuration.Fade } }
            }

            Rectangle {
                anchors.fill: parent
                radius: 6
                color: node.current ? global.theme.general.backSelected : "transparent"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: node.depth * 24
                anchors.rightMargin: treeScrollBar.width
                spacing: 0

                Item {
                    visible: node.group
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    IconImage {
                        anchors.centerIn: parent
                        width: 16; height: 16
                        visible: node.hasChildren
                        source: node.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                        color: global.theme.general.fore
                    }
                }

                Label {
                    Layout.fillWidth: !node.group
                    Layout.maximumWidth: implicitWidth
                    text: node.model.display
                    textFormat: Text.PlainText
                    font.bold: node.group
                    elide: Text.ElideRight
                    color: node.group ? global.theme.general.fore : GitStyle.fileStatusColor(node.model.fileStatus)
                }

                Label {
                    visible: node.group
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    text: {
                        if (!node.group) return ""
                        return node.model.fileCount === 1 ? qsTr("1 file") : qsTr("%1 files").arg(node.model.fileCount)
                    }
                    color: global.theme.general.foreDisabled
                    elide: Text.ElideRight
                }

                Label {
                    visible: !node.group
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    text: node.group ? "" : node.model.directory
                    textFormat: Text.PlainText
                    color: global.theme.general.foreDisabled
                    elide: Text.ElideLeft
                }
            }

            HoverHandler { id: hoverHandler }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                onTapped: {
                    treeView.forceActiveFocus()
                    treeView.selectionModel.setCurrentIndex(treeView.index(node.row, 0), ItemSelectionModel.NoUpdate)
                    if (node.hasChildren) treeView.toggleExpanded(node.row)
                }
            }
        }
    }
}
