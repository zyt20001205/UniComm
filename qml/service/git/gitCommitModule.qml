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

        Connections {
            target: root.gitCommitModule
            function onIndexUpdated() { treeView.expandRecursively() }
        }

        ScrollBar.vertical: ScrollBar {
            id: treeScrollBar
            minimumSize: Math.min(1, 20 / Math.max(1, height))
            palette {
                mid: global.theme.general.stroke
                dark: global.theme.general.strokePressed
            }
        }

        component CommitRow: Item {
            id: node
            implicitWidth: treeView.width; implicitHeight: 24
            required property TreeView treeView
            required property bool current
            required property bool hasChildren
            required property int row
            property string toolTipText: ""

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

            HoverHandler {
                id: hoverHandler
                onHoveredChanged: {
                    if (!hovered) mainTooltip.text = ""
                }
                onPointChanged: {
                    mainTooltip.position = node.mapToGlobal(point.position)
                    mainTooltip.text = node.toolTipText
                }
            }
            TapHandler {
                acceptedButtons: Qt.LeftButton
                onTapped: {
                    treeView.forceActiveFocus()
                    treeView.selectionModel.setCurrentIndex(treeView.index(node.row, 0), ItemSelectionModel.NoUpdate)
                    if (node.hasChildren) treeView.toggleExpanded(node.row)
                }
            }
        }

        delegate: DelegateChooser {
            role: "documentUrl"

            DelegateChoice {
                roleValue: ""
                delegate: CommitRow {
                    id: groupRow
                    required property bool expanded
                    required property string display
                    required property int fileCount

                    RowLayout {
                        anchors.fill: parent
                        anchors.rightMargin: treeScrollBar.width
                        spacing: 0

                        Item {
                            Layout.preferredWidth: 24; Layout.preferredHeight: 24

                            IconImage {
                                anchors.centerIn: parent
                                width: 16; height: 16
                                visible: groupRow.hasChildren
                                source: groupRow.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                                color: global.theme.general.fore
                            }
                        }

                        Label {
                            text: groupRow.display
                            textFormat: Text.PlainText
                            font.bold: true
                            color: global.theme.general.fore
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.leftMargin: 6
                            text: groupRow.fileCount === 1 ? qsTr("1 file") : qsTr("%1 files").arg(groupRow.fileCount)
                            color: global.theme.general.foreDisabled
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            DelegateChoice {
                delegate: CommitRow {
                    id: fileRow
                    required property int depth
                    required property string display
                    required property int fileStatus
                    required property string directory
                    toolTipText: fileStatus === GitFileStatus.Unmodified ? "" : Qt.enumValueToString(GitFileStatus.Value, fileStatus)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: fileRow.depth * 24
                        anchors.rightMargin: treeScrollBar.width
                        spacing: 6

                        Label {
                            Layout.fillWidth: true
                            Layout.maximumWidth: Math.ceil(implicitWidth)
                            text: fileRow.display
                            textFormat: Text.PlainText
                            color: GitStyle.fileStatusColor(fileRow.fileStatus)
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: fileRow.directory
                            textFormat: Text.PlainText
                            color: global.theme.general.foreDisabled
                            elide: Text.ElideLeft
                        }
                    }
                }
            }
        }
    }
}
