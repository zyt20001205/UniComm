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

    Connections {
        target: root.gitCommitModule

        function onCommitted(push) {
            messageArea.clear()
            amendBox.checked = false
            if (push) global.gitModule.gitPushPre()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Vertical
        handle: Item {
            implicitHeight: 5

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: global.theme.general.stroke
            }
        }

        ColumnLayout {
            SplitView.fillHeight: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/arrowClockwise.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    Accessible.name: qsTr("Refresh")

                    onClicked: global.serviceModule.git.indexChanged()

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) mainTooltip.text = ""
                        }
                        onPointChanged: {
                            mainTooltip.position = parent.mapToGlobal(point.position)
                            mainTooltip.text = parent.Accessible.name
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/chevronDownUp.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    onClicked: treeView.collapseRecursively()

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) {
                                mainTooltip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainTooltip.position = parent.mapToGlobal(point.position)
                            mainTooltip.text = qsTr("Collapse All")
                        }
                    }
                }

                Button {
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/chevronUpDown.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    onClicked: treeView.expandRecursively()

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) {
                                mainTooltip.text = ""
                            }
                        }
                        onPointChanged: {
                            mainTooltip.position = parent.mapToGlobal(point.position)
                            mainTooltip.text = qsTr("Expand All")
                        }
                    }
                }
            }

            TreeView {
                id: treeView
                Layout.fillWidth: true; Layout.fillHeight: true
                clip: true
                reuseItems: false
                model: root.gitCommitModule.changes
                columnWidthProvider: column => Math.max(width, implicitColumnWidth(column))
                onWidthChanged: forceLayout()
                selectionModel: ItemSelectionModel {
                    model: root.gitCommitModule.changes
                }

                Connections {
                    target: root.gitCommitModule

                    function onIndexUpdated() {
                        treeView.expandRecursively()
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

                ScrollBar.horizontal: ScrollBar {
                    minimumSize: Math.min(1, 20 / Math.max(1, width))
                    palette {
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
                    }
                }

                component CommitRow: Item {
                    id: node
                    implicitHeight: 24
                    required property TreeView treeView
                    required property bool current
                    required property bool hasChildren
                    required property int row
                    required property int checkState
                    property string toolTipText: ""
                    onImplicitWidthChanged: Qt.callLater(treeView.forceLayout)

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: global.theme.general.backHover
                        opacity: hoverHandler.hovered ? 1 : 0
                        Behavior on opacity {
                            NumberAnimation {
                                duration: AnimationDuration.Fade
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: node.current ? global.theme.general.backSelected : "transparent"

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            onTapped: {
                                treeView.forceActiveFocus()
                                treeView.selectionModel.setCurrentIndex(treeView.index(node.row, 0), ItemSelectionModel.NoUpdate)
                                if (node.hasChildren) treeView.toggleExpanded(node.row)
                            }
                        }
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
                }

                component CommitCheckBox: CheckBox {
                    required property CommitRow node
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    indicator.width: 16; indicator.height: 16
                    checkState: node.checkState
                    nextCheckState: () => node.checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked
                    onClicked: root.gitCommitModule.checkStateSet(treeView.index(node.row, 0), checked)
                }

                delegate: DelegateChooser {
                    role: "documentUrl"

                    DelegateChoice {
                        roleValue: ""
                        delegate: CommitRow {
                            id: groupRow
                            implicitWidth: groupContent.implicitWidth + treeScrollBar.width
                            required property bool expanded
                            required property string display
                            required property int fileCount

                            RowLayout {
                                id: groupContent
                                anchors.verticalCenter: parent.verticalCenter
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

                                CommitCheckBox {
                                    node: groupRow
                                    enabled: groupRow.fileCount > 0
                                }

                                Label {
                                    text: groupRow.display
                                    textFormat: Text.PlainText
                                    font.bold: true
                                    color: global.theme.general.fore
                                }

                                Label {
                                    Layout.leftMargin: 6
                                    text: groupRow.fileCount === 1 ? qsTr("1 file") : qsTr("%1 files").arg(groupRow.fileCount)
                                    color: global.theme.general.foreDisabled
                                }
                            }
                        }
                    }

                    DelegateChoice {
                        delegate: CommitRow {
                            id: fileRow
                            implicitWidth: fileContent.x + fileContent.implicitWidth + treeScrollBar.width
                            required property int depth
                            required property string display
                            required property int fileStatus
                            required property string directory
                            toolTipText: fileStatus === GitFileStatus.Unmodified ? "" : Qt.enumValueToString(GitFileStatus.Value, fileStatus)

                            TapHandler {
                                onDoubleTapped: root.gitCommitModule.diffOpen(fileRow.treeView.index(fileRow.row, 0))
                            }

                            RowLayout {
                                id: fileContent
                                x: (fileRow.depth + 1) * 24
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 6

                                CommitCheckBox {
                                    node: fileRow
                                }

                                Label {
                                    text: fileRow.display
                                    textFormat: Text.PlainText
                                    color: GitStyle.fileStatusColor(fileRow.fileStatus)
                                }

                                Label {
                                    text: fileRow.directory
                                    textFormat: Text.PlainText
                                    color: global.theme.general.foreDisabled
                                }
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            SplitView.minimumHeight: Layout.minimumHeight
            spacing: 8

            RowLayout {
                Layout.leftMargin: 8; Layout.topMargin: 8
                spacing: 4

                CheckBox {
                    id: amendBox
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    indicator.width: 16; indicator.height: 16
                    Accessible.name: amendLabel.text
                }

                Label {
                    id: amendLabel
                    text: qsTr("Amend last commit")
                }
            }

            ScrollView {
                id: messageView
                Layout.leftMargin: 8; Layout.rightMargin: 8
                Layout.fillWidth: true; Layout.fillHeight: true
                Layout.minimumHeight: messageMetrics.lineSpacing + messageArea.topPadding + messageArea.bottomPadding
                Layout.preferredHeight: 6 * messageMetrics.lineSpacing + messageArea.topPadding + messageArea.bottomPadding
                clip: true
                contentWidth: availableWidth
                ScrollBar.vertical.palette.mid: global.theme.general.stroke
                ScrollBar.vertical.palette.dark: global.theme.general.strokePressed

                FontMetrics {
                    id: messageMetrics
                    font: messageArea.font
                }

                TextArea {
                    id: messageArea
                    rightPadding: leftPadding + messageView.ScrollBar.vertical.width
                    placeholderText: qsTr("Commit Message")
                    textFormat: TextEdit.PlainText
                    verticalAlignment: TextEdit.AlignTop
                    wrapMode: TextEdit.Wrap
                }
            }

            RowLayout {
                Layout.leftMargin: 8; Layout.rightMargin: 8; Layout.bottomMargin: 8
                spacing: 6

                Button {
                    text: amendBox.checked ? qsTr("Amend Commit") : qsTr("Commit")
                    highlighted: true
                    onClicked: root.gitCommitModule.commit(messageArea.text, amendBox.checked, false)
                }

                Button {
                    text: amendBox.checked ? qsTr("Amend Commit and Push…") : qsTr("Commit and Push…")
                    onClicked: root.gitCommitModule.commit(messageArea.text, amendBox.checked, true)
                }
            }
        }
    }
}
