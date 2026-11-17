import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Item {
    id: rootItem
    required property QtObject symbolWidget

    Rectangle {
        anchors.fill: parent
        color: global.theme.back
    }

    RowLayout {
        anchors.fill: parent

        Button {
            flat: true
            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
            icon.source: "qrc:/icon/document.svg"
            icon.width: 16; icon.height: 16
            Layout.preferredWidth: 20; Layout.preferredHeight: 20
        }

        RowLayout {
            Repeater {
                model: rootItem.symbolWidget.model

                delegate: RowLayout {
                    id: breadcrumbItem
                    Layout.alignment: Qt.AlignVCenter
                    required property var modelData

                    IconImage {
                        color: global.theme.fore
                        source: "qrc:/icon/arrowCollapse.svg"
                        sourceSize.width: 16
                        sourceSize.height: 16
                        Layout.preferredHeight: 16
                    }

                    Button {
                        id: breadcrumbButton
                        flat: true
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        icon.source: breadcrumbItem.modelData.source
                        icon.color: breadcrumbItem.modelData.iconColor
                        icon.width: 16; icon.height: 16
                        text: breadcrumbItem.modelData.text
                        Layout.preferredWidth: breadcrumbButtonTextMetrics.width + 28; Layout.preferredHeight: 20

                        onClicked: rootItem.symbolWidget.indicatorFill(breadcrumbItem.modelData.position)

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainTooltip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainTooltip.position = parent.mapToGlobal(point.position)
                                mainTooltip.text = breadcrumbItem.modelData.detail
                            }
                        }

                        TextMetrics {
                            id: breadcrumbButtonTextMetrics
                            text: breadcrumbButton.text
                            font: breadcrumbButton.font
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
