pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import UniComm

ToolTip {
    id: root
    parent: mainOverlay
    popupType: Popup.Item
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
    padding: 6
    font: completionWidget.font
    required property QtObject completionWidget

    Connections {
        target: root.completionWidget

        function onShowCompletion(position) {
            if (!root.visible) {
                const localPosition = mainOverlay.mapFromGlobal(position)
                root.x = localPosition.x - root.leftPadding - 11 - 24
                root.y = localPosition.y
            }
            listView.currentIndex = 0
            root.open()
        }

        function onHideCompletion() { root.close() }
        function onSelectPrevious() { listView.decrementCurrentIndex() }
        function onSelectNext() { listView.incrementCurrentIndex() }
        function onAcceptSelection() { root.completionWidget.textReplace(listView.currentIndex) }
    }

    onVisibleChanged: {
        completionWidget.visible = visible
        if (!visible && completionWidget.model.length > 0) completionWidget.completionHide()
    }

    onOpened: {
        global.mainWindow.overlayFlagSet(false, false)
        mainOverlay.widgetCount += 1
    }
    onClosed: {
        mainOverlay.widgetCount -= 1
        detailTimer.stop()
        detailTooltip.close()
    }
    onAboutToShow: detailTimer.restart()

    contentItem: ListView {
        id: listView
        model: root.completionWidget.model
        implicitWidth: Math.max(200, root.completionWidget.contentSize.width + 24 + 11 + 6 + verticalScrollBar.width)
        implicitHeight: Math.min(root.completionWidget.contentSize.height, 150)
        clip: true
        focus: false
        keyNavigationEnabled: false
        keyNavigationWraps: false
        flickableDirection: Flickable.VerticalFlick
        property int hoveredIndex: -1
        readonly property int previewIndex: hoveredIndex !== -1 ? hoveredIndex : currentIndex

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
            width: listView.width
            height: root.completionWidget.rowHeight
            font: root.font
            leftPadding: 11; rightPadding: 6 + verticalScrollBar.width
            topPadding: 0; bottomPadding: 0
            focusPolicy: Qt.NoFocus
            hoverEnabled: true
            highlighted: ListView.isCurrentItem

            contentItem: RowLayout {
                spacing: 0

                Item {
                    Layout.preferredWidth: 24
                    Layout.fillHeight: true

                    IconImage {
                        anchors.centerIn: parent
                        width: 16; height: 16
                        source: CompletionKindStyle.icon(row.modelData.kind)
                        color: CompletionKindStyle.color(row.modelData.kind)
                    }
                }

                Label {
                    font: row.font
                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                    text: row.modelData.displayText
                    textFormat: Text.RichText
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            onHoveredChanged: {
                if (hovered) {
                    listView.hoveredIndex = index
                } else if (listView.hoveredIndex === index) {
                    listView.hoveredIndex = -1
                }
            }
            onClicked: listView.currentIndex = index
            onDoubleClicked: {
                listView.currentIndex = index
                root.completionWidget.textReplace(index)
            }
        }

        onModelChanged: {
            hoveredIndex = -1
            positionViewAtBeginning()
            if (root.visible) detailTimer.restart()
        }
        onHoveredIndexChanged: {
            if (root.visible) detailTimer.restart()
        }
        onCurrentIndexChanged: {
            if (root.visible) detailTimer.restart()
        }
        onContentYChanged: {
            if (root.visible) detailTimer.restart()
        }

        Timer {
            id: detailTimer
            interval: 150

            onTriggered: {
                if (!root.visible) return
                if (listView.previewIndex < 0 || listView.previewIndex >= listView.count) return
                const candidate = listView.model[listView.previewIndex]
                detailView.model = candidate.labels
                detailView.labelWidth = candidate.detailWidth
                detailTooltip.open()
            }
        }
    }

    ToolTip {
        id: detailTooltip
        x: root.width - 5
        y: {
            const rowHeight = root.completionWidget.rowHeight
            const rowY = listView.previewIndex * rowHeight - listView.contentY
            return Math.max(0, Math.min(listView.height - rowHeight, rowY)) - 6
        }
        padding: 6
        width: implicitWidth
        height: implicitHeight

        Behavior on width {
            enabled: detailTooltip.opened
            NumberAnimation { duration: AnimationDuration.Follow; easing.type: Easing.OutCubic }
        }
        Behavior on height {
            enabled: detailTooltip.opened
            NumberAnimation { duration: AnimationDuration.Follow; easing.type: Easing.OutCubic }
        }

        Behavior on y {
            enabled: detailTooltip.visible
            NumberAnimation { duration: AnimationDuration.Follow; easing.type: Easing.OutCubic }
        }

        contentItem: ListView {
            id: detailView
            property real labelWidth: 0
            implicitWidth: labelWidth + detailScrollBar.width
            implicitHeight: Math.min(count * root.completionWidget.rowHeight, 150)
            clip: true
            focus: false
            keyNavigationEnabled: false
            currentIndex: -1
            flickableDirection: Flickable.VerticalFlick

            ScrollBar.vertical: ScrollBar {
                id: detailScrollBar
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: Label {
                required property string modelData
                width: detailView.width
                height: root.completionWidget.rowHeight
                rightPadding: detailScrollBar.width
                font: root.font
                horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                text: modelData
                textFormat: Text.PlainText
                elide: Text.ElideRight
            }

            onModelChanged: positionViewAtBeginning()
        }
    }
}
