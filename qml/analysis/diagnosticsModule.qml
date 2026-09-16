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
        visible: tabBar.currentIndex === -1

        RowLayout {
            anchors.centerIn: parent

            Label {
                text: qsTr("No errors found.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            IconImage {
                source: "qrc:/icon/checkmarkCircle.svg"
                color: global.successBack3
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            contentHeight: 24
            Layout.fillWidth: true; Layout.preferredHeight: 32
        }
        StackLayout {
            id: stackLayout
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: tabBar.currentIndex
        }
    }

    Component {
        id: tabButtonComponent

        TabButton {
            id: tabButton
            icon.color: global.fore
            width: contentItem.implicitWidth + 24; height: 24
            property var diagnosticsModel: null

            Connections {
                target: tabButton.diagnosticsModel
                enabled: tabButton.diagnosticsModel !== null

                function onRowsInserted() {
                    tabButton.width = contentItem.implicitWidth + 24
                    tabButton.visible = true
                    tabBar.currentIndex = tabButton.TabBar.index
                }

                function onModelReset() {
                    tabButton.width = 0
                    tabButton.visible = false
                    let showIndex = -1
                    for (let i = 0; i < tabBar.count; i++) {
                        const item = tabBar.itemAt(i)
                        if (item && item.visible) {
                            showIndex = i
                            break
                        }
                    }
                    tabBar.currentIndex = showIndex
                }
            }
        }
    }

    Component {
        id: pageComponent

        Item {
            id: pageItem
            Layout.fillWidth: true; Layout.fillHeight: true
            property var diagnosticsModel: null

            HorizontalHeaderView {
                id: horizontalHeaderView
                anchors.top: parent.top
                width: parent.width; height: 32
                syncView: tableView
                clip: true
                interactive: false

                delegate: HorizontalHeaderViewDelegate {
                    id: horizontalDelegate
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
                        text: horizontalHeader[horizontalDelegate.index]
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
                model: pageItem.diagnosticsModel
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
                        implicitWidth: 24; implicitHeight: 24

                        Rectangle {
                            anchors.fill: parent
                            color: global.back
                        }

                        IconImage {
                            anchors.centerIn: parent
                            width: 16; height: 16
                            source: model.decoration
                            color: model.severity === 1 ? global.dangerBack3
                                   : model.severity === 2 ? global.warningBack3
                                   : model.severity === 3 ? global.brandBack
                                   : global.fore
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
                            return Math.max(textMetrics.width + 12, 60)
                        }
                        implicitHeight: 24
                        required property int column

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
                            onTapped: {
                                diagnosticsModule.indicatorFill(model.position)
                            }
                        }

                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: {
                                diagnosticMenu.position = model.position
                                diagnosticMenu.popup()
                            }
                        }
                    }
                }
            }
        }
    }

    function append(name, diagnosticsModel) {
        const tabButton = tabButtonComponent.createObject(tabBar, {
            "text": name,
            "diagnosticsModel": diagnosticsModel
        });
        const page = pageComponent.createObject(stackLayout, {
            "diagnosticsModel": diagnosticsModel
        });
    }
}
