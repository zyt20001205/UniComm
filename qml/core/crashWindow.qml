import QtQuick
import QtQuick.Controls

ScrollView {
    id: crashScrollView
    rightPadding: effectiveScrollBarWidth
    bottomPadding: effectiveScrollBarHeight

    ScrollBar.vertical: ScrollBar {
        parent: crashScrollView
        x: parent.mirrored ? 0 : parent.width - width
        y: parent.topPadding
        height: parent.availableHeight
        policy: ScrollBar.AsNeeded
        minimumSize: Math.min(1, 20 / Math.max(1, height))
    }

    ScrollBar.horizontal: ScrollBar {
        parent: crashScrollView
        x: parent.leftPadding
        y: parent.height - height
        width: parent.availableWidth
        policy: ScrollBar.AsNeeded
        minimumSize: Math.min(1, 20 / Math.max(1, width))
    }

    TextArea {
        text: crashReport
        readOnly: true
        selectByMouse: true
        wrapMode: TextEdit.NoWrap
        font.family: "Consolas"
        background: null
    }
}
