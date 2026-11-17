import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: rootItem
    property int conflicts

    Rectangle {
        anchors.fill: parent
        anchors.margins: 6
        color: global.theme.general.stroke
        opacity: 0.3
        radius: 6
    }

    RowLayout {
        id: resolveBar
        anchors.fill: parent
        anchors.margins: 6

        Label {
            id: resolveStatLabel
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            text: conflicts === 0 ? qsTr("Add to index")
                : conflicts + qsTr(" conflict(s) left")
            Layout.fillWidth: true; Layout.preferredHeight: 24
        }

        Button {
            id: resolveFinishButton
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            flat: true
            visible: conflicts === 0
            icon.source: "qrc:/icon/arrowRight.svg"
            icon.width: 16; icon.height: 16
            Layout.preferredWidth: 24; Layout.preferredHeight: 24

            onClicked: resolveWidget.resolveFinish()
        }
    }

    Component.onCompleted: {
        const objects = {
            "resolveStatLabel": resolveStatLabel,
            "resolveFinishButton": resolveFinishButton
        };
        resolveWidget.propertyGet(objects)
    }
}
