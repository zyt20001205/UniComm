pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import UniComm

Item {
    id: root
    required property ServiceModule serviceModule
    anchors.fill: parent
    readonly property var selectedService: root.serviceModule.services[serviceView.currentIndex]
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
        running: root.visible && uptimeLabel.visible
        onTriggered: root.currentTime = Date.now()
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    ListView {
        id: serviceView
        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
        width: 240
        clip: true
        currentIndex: 0
        model: root.serviceModule.services
        focusPolicy: Qt.ClickFocus
        keyNavigationEnabled: true

        ScrollBar.vertical: ScrollBar {
            id: serviceScrollBar
            minimumSize: Math.min(1, 20 / Math.max(1, height))
            palette {
                mid: global.theme.general.stroke
                dark: global.theme.general.strokePressed
            }
        }

        delegate: ItemDelegate {
            id: serviceDelegate
            required property int index
            required property var modelData

            width: serviceView.width; height: 28
            text: serviceDelegate.modelData.name
            leftPadding: 11; rightPadding: 6 + serviceScrollBar.width
            topPadding: 0; bottomPadding: 0
            focusPolicy: Qt.ClickFocus
            highlighted: ListView.isCurrentItem

            onClicked: serviceView.currentIndex = index

            contentItem: RowLayout {
                Label {
                    text: serviceDelegate.text
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Rectangle {
                    id: statusIndicator
                    implicitWidth: 8; implicitHeight: 8
                    radius: 4
                    color: serviceDelegate.modelData.status === 1 ? global.theme.general.successFore3
                        : serviceDelegate.modelData.status === 2 ? global.theme.general.dangerFore3
                        : global.theme.general.stroke

                    SequentialAnimation {
                        id: flashAnimation
                        NumberAnimation { target: statusIndicator; property: "opacity"; to: 0.25; duration: AnimationDuration.Fade }
                        PauseAnimation { duration: AnimationDuration.FlashHold }
                        NumberAnimation { target: statusIndicator; property: "opacity"; to: 1; duration: AnimationDuration.Fade }
                    }

                    Connections {
                        target: serviceDelegate.modelData.eventModel
                        function onRowsInserted() { flashAnimation.restart() }
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
                color: global.theme.general.stroke
            }
        }

        Item {
            SplitView.preferredWidth: 240; SplitView.minimumWidth: 120

            Label {
                id: uptimeLabel
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                height: visible ? implicitHeight : 0
                visible: root.selectedService?.status === 1
                text: visible ? qsTr("Uptime: %1").arg(root.uptimeFormat(root.currentTime - root.selectedService.createdAt)) : ""
                color: global.theme.general.fore
                wrapMode: Text.Wrap
            }

            ScrollView {
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                anchors.top: uptimeLabel.bottom
                anchors.topMargin: uptimeLabel.visible ? 4 : 0
                clip: true
                contentWidth: availableWidth
                rightPadding: effectiveScrollBarWidth

                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.minimumSize: Math.min(1, 20 / Math.max(1, ScrollBar.vertical.height))
                ScrollBar.vertical.palette.mid: global.theme.general.stroke
                ScrollBar.vertical.palette.dark: global.theme.general.strokePressed

                TextArea {
                    id: detailTextArea
                    text: root.selectedService?.detail ?? ""
                    textFormat: TextEdit.RichText
                    readOnly: true
                    selectByMouse: true
                    ContextMenu.menu: null
                    verticalAlignment: TextEdit.AlignTop
                    wrapMode: TextEdit.Wrap
                    color: global.theme.general.fore
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
                            mainLinkMenu.url = detailTextArea.hoveredLink
                            mainLinkMenu.popup()
                        }
                    }
                }
            }
        }

        Item {
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
                    required property int column
                    required property string display
                    implicitWidth: 120; implicitHeight: 32

                    Label {
                        anchors.fill: parent
                        text: headerCell.display
                        color: global.theme.general.fore
                        font.bold: true
                        leftPadding: headerCell.column === 1 ? 0 : 6; rightPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                        height: 1
                        color: global.theme.general.stroke
                    }
                }
            }

            TableView {
                id: eventTable
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: eventHeader.bottom; anchors.bottom: parent.bottom
                model: root.selectedService?.eventModel ?? null
                rowSpacing: 1; columnSpacing: 0
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                editTriggers: TableView.NoEditTriggers

                readonly property int statusWidth: 11
                readonly property int timeWidth: 112
                readonly property int eventWidth: 144

                columnWidthProvider: function(column: int): real {
                    if (column === 0) return statusWidth
                    const resizedWidth = explicitColumnWidth(column)
                    if (resizedWidth >= 0) return resizedWidth
                    if (column === 1) return timeWidth
                    if (column === 2) return eventWidth
                    return Math.max(160, Math.floor(width) - statusWidth - timeWidth - eventWidth)
                }

                onWidthChanged: forceLayout()

                Rectangle {
                    parent: eventTable.contentItem
                    anchors.fill: parent
                    color: global.theme.general.stroke
                }

                ScrollBar.vertical: ScrollBar {
                    id: verticalScrollBar
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

                delegate: Item {
                    id: eventCell
                    required property int column
                    required property var display
                    required property var toolTip
                    implicitWidth: 120; implicitHeight: 32

                    Rectangle {
                        anchors.fill: parent
                        color: global.theme.general.back
                    }

                    Label {
                        anchors.fill: parent
                        visible: eventCell.column !== 0
                        text: eventCell.display ?? ""
                        textFormat: Text.PlainText
                        color: global.theme.general.fore
                        leftPadding: eventCell.column === 1 ? 0 : 6
                        rightPadding: 6 + (eventCell.column === eventTable.columns - 1 ? verticalScrollBar.width : 0)
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        visible: eventCell.column === 0
                        anchors.centerIn: parent
                        width: 3; height: 16
                        radius: width * 0.5
                        color: eventCell.display ? global.theme.general.successBack3 : global.theme.general.dangerBack3
                    }

                    HoverHandler {
                        enabled: !!eventCell.toolTip

                        onHoveredChanged: {
                            if (!hovered) {
                                mainTooltip.text = ""
                            }
                        }

                        onPointChanged: {
                            mainTooltip.position = eventCell.mapToGlobal(point.position)
                            mainTooltip.text = eventCell.toolTip
                        }
                    }
                }
            }
        }
    }
}
