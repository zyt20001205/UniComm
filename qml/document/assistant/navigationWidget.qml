pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import UniComm

ToolTip {
    id: root
    parent: mainOverlay
    popupType: Popup.Item
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
    padding: 6
    font: navigationWidget.font
    required property QtObject navigationWidget

    Connections {
        target: root.navigationWidget

        function onShowNavigation(position) {
            if (!root.visible) {
                const localPosition = mainOverlay.mapFromGlobal(position)
                root.x = localPosition.x - root.leftPadding - 11
                root.y = localPosition.y
            }
            listView.currentIndex = 0
            root.open()
        }

        function onHideNavigation() { root.close() }
        function onSelectPrevious() { listView.decrementCurrentIndex() }
        function onSelectNext() { listView.incrementCurrentIndex() }
        function onAcceptSelection() { root.navigationWidget.indicatorInsert(listView.currentIndex) }
    }

    onVisibleChanged: {
        navigationWidget.visible = visible
        if (!visible && navigationWidget.model.length > 0) navigationWidget.navigationHide()
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
        model: root.navigationWidget.model
        implicitWidth: Math.max(200, root.navigationWidget.contentSize.width + 11 + 6 + verticalScrollBar.width)
        implicitHeight: Math.min(root.navigationWidget.contentSize.height, 150)
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
            height: root.navigationWidget.rowHeight
            font: root.font
            leftPadding: 11; rightPadding: 6 + verticalScrollBar.width
            topPadding: 0; bottomPadding: 0
            focusPolicy: Qt.NoFocus
            hoverEnabled: true
            highlighted: ListView.isCurrentItem

            contentItem: Label {
                font: row.font
                horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                text: row.modelData.text
                textFormat: Text.PlainText
                elide: Text.ElideRight
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
                root.navigationWidget.indicatorInsert(index)
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
                root.navigationWidget.detailReload(listView.previewIndex)
                if (root.navigationWidget.detailText.length > 0) detailTooltip.open()
                else detailTooltip.close()
            }
        }
    }

    ToolTip {
        id: detailTooltip
        x: root.width - 5
        y: {
            const rowHeight = root.navigationWidget.rowHeight
            const rowY = listView.previewIndex * rowHeight - listView.contentY
            return Math.max(0, Math.min(listView.height - rowHeight, rowY)) - 6
        }
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

        contentItem: Label {
            clip: true
            font: root.font
            text: root.navigationWidget.detailText
            textFormat: Text.RichText
        }
    }
}
