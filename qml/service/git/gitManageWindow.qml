pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        ListView {
            id: navigationView
            Layout.preferredWidth: 180; Layout.fillHeight: true
            clip: true
            currentIndex: 0
            focusPolicy: Qt.ClickFocus
            keyNavigationEnabled: true
            model: [
                {title: qsTr("Remotes"), icon: "qrc:/icon/cloud.svg"},
                {title: qsTr("Proxy"), icon: "qrc:/icon/plugConnected.svg"}
            ]

            ScrollBar.vertical: ScrollBar {
                id: scrollBar
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: ItemDelegate {
                required property int index
                required property var modelData
                width: navigationView.width; height: 28
                leftPadding: 11; rightPadding: 6 + scrollBar.width
                topPadding: 0; bottomPadding: 0
                text: modelData.title
                icon.source: modelData.icon
                icon.width: 16; icon.height: 16
                focusPolicy: Qt.ClickFocus
                highlighted: ListView.isCurrentItem

                onClicked: navigationView.currentIndex = index
            }
        }

        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: navigationView.currentIndex

            Item {}
            Item {}
        }
    }
}
