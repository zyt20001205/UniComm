import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property QtObject resolveWidget

    Rectangle {
        anchors.fill: parent
        anchors.margins: 6
        color: global.theme.general.stroke
        opacity: 0.3
        radius: 6
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6

        Label {
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            text: root.resolveWidget.conflicts === 0 ? qsTr("Add to index")
                : root.resolveWidget.conflicts + qsTr(" conflict(s) left")
            Layout.fillWidth: true; Layout.preferredHeight: 24
        }

        Button {
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            flat: true
            visible: root.resolveWidget.conflicts === 0
            icon.source: "qrc:/icon/arrowRight.svg"
            icon.width: 16; icon.height: 16
            Layout.preferredWidth: 24; Layout.preferredHeight: 24

            onClicked: root.resolveWidget.resolveFinish()
        }
    }
}
