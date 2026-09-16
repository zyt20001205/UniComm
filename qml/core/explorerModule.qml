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

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            id: toolBar
            Layout.fillWidth: true
            spacing: 0

            Item {
                Layout.fillWidth: true
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                flat: true
                icon.source: "qrc:/icon/collapse.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: {
                    for (let i = 0; i < treeView.rows; ++i) {
                        treeView.collapseRecursively(i)
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
                        mainToolTip.text = qsTr("Collapse All")
                    }
                }
            }

            Button {
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                flat: true
                icon.source: "qrc:/icon/expand.svg"
                icon.width: 16; icon.height: 16
                Layout.preferredWidth: 24; Layout.preferredHeight: 24

                onClicked: {
                    for (let i = 0; i < treeView.rows; ++i) {
                        treeView.expandRecursively(i)
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
                        mainToolTip.text = qsTr("Expand All")
                    }
                }
            }
        }

        TreeView {
            id: treeView
            clip: true
            model: sortFilterProxyModel
            reuseItems: false
            rootIndex: modelRootIndex
            Layout.fillWidth: true; Layout.fillHeight: true
            property int selectedRow: -1

            ScrollBar.vertical: ScrollBar {
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.stroke
                    dark: global.strokePressed
                }
            }

            delegate: Item {
                implicitWidth: treeView.width; implicitHeight: 24
                required property TreeView treeView
                required property bool expanded
                required property int depth
                required property int row

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

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: treeView.selectedRow === row ? global.backSelected : "transparent"
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Item {
                        Layout.preferredWidth: depth * 24; Layout.preferredHeight: 24
                    }

                    Item {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                        IconImage {
                            anchors.centerIn: parent
                            width: 16; height: 16
                            source: expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                            color: global.fore
                            visible: model.isDir === true
                        }

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            gesturePolicy: TapHandler.DragWithinBounds

                            onTapped: {
                                treeView.selectedRow = row
                                if (model.isDir) {
                                    treeView.toggleExpanded(row)
                                }
                            }
                        }
                    }

                    Item {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                        Image {
                            anchors.centerIn: parent
                            width: 20; height: 20
                            source: !expanded ? model.source
                                : model.source == "qrc:/icon/fileTypeFolder.svg" ? "qrc:/icon/fileTypeFolderOpen.svg"
                                    : model.source == "qrc:/icon/fileTypeFolderGit.svg" ? "qrc:/icon/fileTypeFolderOpenGit.svg"
                                        : model.source == "qrc:/icon/fileTypeFolderIntellij.svg" ? "qrc:/icon/fileTypeFolderOpenIntellij.svg"
                                            : model.source
                        }
                    }

                    Label {
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        color: !model.git ? global.fore
                            : model.git.indexStatus === 0 ? global.successFore3
                                : model.git.indexStatus === 1 ? global.stroke
                                    : model.git.indexStatus === 5 ? global.successFore2
                                        : model.git.indexStatus === 7 ? global.warningFore3
                                            : model.git.workingTreeStatus === 2 ? global.fore
                                                : model.git.workingTreeStatus === 3 ? global.brandBack
                                                    : model.git.workingTreeStatus === 9 ? global.dangerFore3
                                                        : global.fore
                        text: model.display || ""
                        elide: Text.ElideRight
                        Layout.fillWidth: true; Layout.preferredHeight: 24
                    }
                }

                HoverHandler {
                    id: hoverHandler

                    onHoveredChanged: {
                        if (!hovered) {
                            mainToolTip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainToolTip.position = parent.mapToGlobal(point.position)
                        if (model.git) {
                            if (model.git.indexStatus === 0) {
                                mainToolTip.text = "Untracked"
                            } else if (model.git.indexStatus === 1) {
                                mainToolTip.text = "Ignored"
                            } else if (model.git.indexStatus === 5) {
                                mainToolTip.text = "Added"
                            } else if (model.git.indexStatus === 7) {
                                mainToolTip.text = "Renamed"
                            } else if (model.git.workingTreeStatus === 2) {
                                // mainToolTip.text = "Unmodified"
                            } else if (model.git.workingTreeStatus === 3) {
                                mainToolTip.text = "Modified"
                            } else if (model.git.workingTreeStatus === 9) {
                                mainToolTip.text = "Unmerged"
                            }
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onTapped: treeView.selectedRow = row
                    onDoubleTapped: {
                        if (!model.isDir) {
                            explorerModule.documentOpen(model.documentUrl)
                        }
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onTapped: {
                        if (model.isDir) {
                            folderMenu.documentUrl = model.documentUrl
                            folderMenu.popup()
                        } else {
                            fileMenu.gitUntracked = model.git ? model.git.indexStatus === 0 : false
                            fileMenu.gitIgnored = model.git ? model.git.indexStatus === 1 : false
                            fileMenu.documentUrl = model.documentUrl
                            fileMenu.popup()
                        }
                    }
                }
            }

            TapHandler {
                acceptedButtons: Qt.LeftButton

                onTapped: treeView.selectedRow = -1
            }

            TapHandler {
                acceptedButtons: Qt.RightButton

                onTapped: {
                    rootMenu.documentUrl = modelRootUrl
                    rootMenu.popup()
                }
            }
        }
    }

    Component.onCompleted: {
        const objects = {
            "treeView": treeView
        };
        explorerModule.propertyGet(objects)
    }
}
