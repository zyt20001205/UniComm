import QtQuick
import QtQuick.Controls

Item {
    id: rootItem
    anchors.fill: parent
    readonly property var selectedService: serviceModule.services[serviceView.currentIndex]
    property double currentTime: Date.now()

    function uptimeFormat(milliseconds) {
        const seconds = Math.max(0, Math.floor(milliseconds / 1000))
        return [Math.floor(seconds / 3600), Math.floor(seconds / 60) % 60, seconds % 60]
            .map(value => String(value).padStart(2, "0")).join(":")
    }

    Timer {
        interval: 1000
        repeat: true
        triggeredOnStart: true
        running: rootItem.visible && uptimeLabel.visible
        onTriggered: rootItem.currentTime = Date.now()
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.back
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
            id: serviceDelegate
            required property int index
            required property var modelData

            width: serviceView.width; height: 36
            text: modelData.name
            rightPadding: 28
            highlighted: serviceView.currentIndex === index

            onClicked: serviceView.currentIndex = index

            Rectangle {
                id: statusIndicator
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: 8; height: 8
                radius: 4
                color: serviceDelegate.modelData.status === 1 ? global.theme.successFore3
                    : serviceDelegate.modelData.status === 2 ? global.theme.dangerFore3
                    : global.theme.stroke

                SequentialAnimation {
                    id: flashAnimation
                    NumberAnimation { target: statusIndicator; property: "opacity"; to: 0.25; duration: 150 }
                    PauseAnimation { duration: 350 }
                    NumberAnimation { target: statusIndicator; property: "opacity"; to: 1; duration: 150 }
                }

                Connections {
                    target: serviceDelegate.modelData.eventModel

                    function onRowsInserted() {
                        flashAnimation.restart()
                    }
                }
            }
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
                color: global.theme.stroke
            }
        }

        Item {
            SplitView.preferredWidth: 240; SplitView.minimumWidth: 120

            Label {
                id: uptimeLabel
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                height: visible ? implicitHeight : 0
                visible: rootItem.selectedService ? rootItem.selectedService.status === 1 : false
                text: visible ? qsTr("Uptime: %1").arg(rootItem.uptimeFormat(rootItem.currentTime - rootItem.selectedService.createdAt)) : ""
                color: global.theme.fore
                wrapMode: Text.Wrap
            }

            ScrollView {
                id: detailScrollView
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                anchors.top: uptimeLabel.bottom
                anchors.topMargin: uptimeLabel.visible ? 4 : 0
                clip: true
                contentWidth: availableWidth
                rightPadding: effectiveScrollBarWidth

                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: ScrollBar {
                    parent: detailScrollView
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    x: parent.mirrored ? 0 : parent.width - width
                    y: parent.topPadding
                    height: parent.availableHeight
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.theme.stroke
                        dark: global.theme.strokePressed
                    }
                }

                TextArea {
                    id: detailTextArea
                    text: rootItem.selectedService ? rootItem.selectedService.detail : ""
                    textFormat: TextEdit.RichText
                    readOnly: true
                    selectByMouse: true
                    ContextMenu.menu: null
                    verticalAlignment: TextEdit.AlignTop
                    wrapMode: TextEdit.Wrap
                    color: global.theme.fore
                    background: null
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0

                    HoverHandler {
                        id: detailHoverHandler
                        cursorShape: detailTextArea.hoveredLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                    }

                    ToolTip {
                        visible: detailTextArea.hoveredLink
                        text: qsTr("Ctrl + Click")
                        x: detailHoverHandler.point.position.x + 10
                        y: detailHoverHandler.point.position.y + 10
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        acceptedModifiers: Qt.ControlModifier
                        onTapped: {
                            if (detailTextArea.hoveredLink) Qt.openUrlExternally(detailTextArea.hoveredLink)
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: {
                            if (!detailTextArea.hoveredLink) return
                            serviceModuleContextMenu.url = detailTextArea.hoveredLink
                            serviceModuleContextMenu.popup()
                        }
                    }
                }
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
                        color: global.theme.fore
                        font.bold: true
                        leftPadding: 6; rightPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                        height: 1
                        color: global.theme.stroke
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
                rowSpacing: 1; columnSpacing: 0
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

                Rectangle {
                    parent: eventTable.contentItem
                    anchors.fill: parent
                    color: global.theme.stroke
                }

                ScrollBar.vertical: ScrollBar {
                    id: verticalScrollBar
                    parent: eventPane
                    anchors.left: eventTable.right; anchors.top: eventTable.top; anchors.bottom: eventTable.bottom
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.theme.stroke
                        dark: global.theme.strokePressed
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    id: horizontalScrollBar
                    parent: eventPane
                    anchors.left: eventTable.left; anchors.right: eventTable.right; anchors.top: eventTable.bottom
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.theme.stroke
                        dark: global.theme.strokePressed
                    }
                }

                delegate: Item {
                    id: eventCell
                    required property var display
                    required property var toolTip
                    implicitWidth: 120; implicitHeight: 32

                    Rectangle {
                        anchors.fill: parent
                        color: global.theme.back
                    }

                    Label {
                        anchors.fill: parent
                        text: eventCell.display ?? ""
                        textFormat: Text.PlainText
                        color: global.theme.fore
                        leftPadding: 6; rightPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
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
