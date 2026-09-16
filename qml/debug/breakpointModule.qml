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

            Label {
                text: qsTr("No breakpoints.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            IconImage {
                source: "qrc:/icon/activateBreakpoints.svg"
                color: global.fore
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    TreeView {
        id: treeView
        anchors.fill: parent
        clip: true
        model: standardItemModel
        visible: !standardItemModel.empty
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
            required property bool isTreeNode
            required property bool expanded
            required property bool hasChildren
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
                        visible: isTreeNode && hasChildren
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        gesturePolicy: TapHandler.DragWithinBounds

                        onTapped: {
                            treeView.selectedRow = row
                            if (isTreeNode && hasChildren) {
                                treeView.toggleExpanded(row)
                            }
                        }
                    }
                }

                Item {
                    visible: !(isTreeNode && hasChildren)
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    IconImage {
                        anchors.centerIn: parent
                        width: 16; height: 16
                        source: !model.enable ? "qrc:/icon/breakpointDisabled.svg" :
                            model.condition ? "qrc:/icon/breakpointConditional.svg" :
                                "qrc:/icon/breakpoint.svg"
                        color: global.fore
                    }
                }

                Label {
                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                    text: model.display
                    elide: Text.ElideRight
                    Layout.fillWidth: true; Layout.preferredHeight: 24
                }
            }

            HoverHandler {
                id: hoverHandler
            }

            TapHandler {
                acceptedButtons: Qt.LeftButton
                gesturePolicy: TapHandler.DragWithinBounds

                onTapped: {
                    treeView.selectedRow = row
                    if (isTreeNode && hasChildren) {
                        breakpointModule.documentOpen(model.documentUrl)
                    } else {
                        breakpointModule.markerAdd(model.documentUrl, model.display)
                    }
                }
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                gesturePolicy: TapHandler.DragWithinBounds

                onTapped: {
                    if (isTreeNode && hasChildren) {
                        fileMenu.documentUrl = model.documentUrl
                        fileMenu.treeView = treeView
                        fileMenu.popup()
                    } else {
                        lineMenu.documentUrl = model.documentUrl
                        lineMenu.line = model.display
                        lineMenu.treeView = treeView
                        lineMenu.popup()
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
                rootMenu.treeView = treeView
                rootMenu.popup()
            }
        }
    }
    Component.onCompleted: {
        const objects = {
            "treeView": treeView
        };
        breakpointModule.propertyGet(objects)
    }
}

