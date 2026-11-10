import QtQuick
import QtQuick.Controls

Item {
    id: rootItem
    anchors.fill: parent
    readonly property var selectedService: serviceModule.services[serviceView.currentIndex]

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    ListView {
        id: serviceView
        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
        width: 240
        clip: true
        currentIndex: 0
        model: serviceModule.services
        spacing: 2

        delegate: ItemDelegate {
            required property int index
            required property var modelData

            width: serviceView.width; height: 36
            text: modelData.name
            highlighted: serviceView.currentIndex === index

            onClicked: serviceView.currentIndex = index
        }
    }

    SplitView {
        anchors.left: serviceView.right; anchors.right: parent.right
        anchors.top: parent.top; anchors.bottom: parent.bottom
        orientation: Qt.Horizontal

        handle: Item {
            implicitWidth: 5

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top; anchors.bottom: parent.bottom
                width: 1
                color: global.stroke
            }
        }

        ScrollView {
            SplitView.preferredWidth: 240; SplitView.minimumWidth: 120
            clip: true
            contentWidth: availableWidth
            rightPadding: effectiveScrollBarWidth

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                palette {
                    mid: global.stroke
                    dark: global.strokePressed
                }
            }

            TextArea {
                text: rootItem.selectedService ? rootItem.selectedService.detail : ""
                textFormat: TextEdit.RichText
                readOnly: true
                selectByMouse: true
                verticalAlignment: TextEdit.AlignTop
                wrapMode: TextEdit.Wrap
                color: global.fore
                background: null
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            }
        }

        Item {
            id: eventPane
            SplitView.fillWidth: true

            HorizontalHeaderView {
                id: eventHeader
                anchors.left: eventTable.left; anchors.right: eventTable.right; anchors.top: parent.top
                height: 32
                syncView: eventTable
                clip: true
                interactive: false
                resizableColumns: true
                columnSpacing: 0
                editTriggers: TableView.NoEditTriggers

                delegate: Item {
                    id: headerCell
                    required property string display
                    implicitWidth: 120; implicitHeight: 32

                    Label {
                        anchors.fill: parent
                        text: headerCell.display
                        color: global.fore
                        font.bold: true
                        leftPadding: 6; rightPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                        height: 1
                        color: global.stroke
                    }
                }
            }

            TableView {
                id: eventTable
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: eventHeader.bottom; anchors.bottom: parent.bottom
                anchors.rightMargin: verticalScrollBar.width
                anchors.bottomMargin: horizontalScrollBar.height
                model: rootItem.selectedService ? rootItem.selectedService.eventModel : null
                rowSpacing: 0; columnSpacing: 0
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                editTriggers: TableView.NoEditTriggers

                readonly property int timeWidth: 112
                readonly property int eventWidth: 144

                columnWidthProvider: function(column: int): real {
                    const resizedWidth = explicitColumnWidth(column)
                    if (resizedWidth >= 0) return resizedWidth
                    if (column === 0) return timeWidth
                    if (column === 1) return eventWidth
                    return Math.max(160, Math.floor(width) - timeWidth - eventWidth)
                }

                onWidthChanged: forceLayout()

                ScrollBar.vertical: ScrollBar {
                    id: verticalScrollBar
                    parent: eventPane
                    anchors.left: eventTable.right; anchors.top: eventTable.top; anchors.bottom: eventTable.bottom
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    id: horizontalScrollBar
                    parent: eventPane
                    anchors.left: eventTable.left; anchors.right: eventTable.right; anchors.top: eventTable.bottom
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                delegate: Item {
                    id: eventCell
                    required property var display
                    required property var toolTip
                    implicitWidth: 120; implicitHeight: 32

                    Label {
                        anchors.fill: parent
                        text: eventCell.display ?? ""
                        textFormat: Text.PlainText
                        color: global.fore
                        leftPadding: 6; rightPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                        height: 1
                        color: global.stroke
                    }

                    HoverHandler {
                        enabled: !!eventCell.toolTip

                        onHoveredChanged: {
                            if (!hovered) {
                                mainToolTip.text = ""
                            }
                        }

                        onPointChanged: {
                            mainToolTip.position = parent.mapToGlobal(point.position)
                            mainToolTip.text = eventCell.toolTip || ""
                        }
                    }
                }
            }
        }
    }
}
