pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import "../../util"

import UniComm

Item {
    id: root
    required property SymbolWidget symbolWidget

    SymbolKindStyle { id: symbolStyle }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
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
                model: root.symbolWidget.model

                delegate: RowLayout {
                    id: breadcrumbItem
                    Layout.alignment: Qt.AlignVCenter
                    required property var modelData

                    IconImage {
                        color: global.theme.general.fore
                        source: "qrc:/icon/arrowCollapse.svg"
                        sourceSize.width: 16
                        sourceSize.height: 16
                        Layout.preferredHeight: 16
                    }

                    Button {
                        flat: true
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        icon.source: symbolStyle.kindIcon(breadcrumbItem.modelData.kind)
                        icon.color: symbolStyle.kindColor(breadcrumbItem.modelData.kind)
                        icon.width: 16; icon.height: 16
                        text: breadcrumbItem.modelData.text
                        Layout.preferredWidth: implicitContentWidth + 8; Layout.preferredHeight: 20

                        onClicked: root.symbolWidget.indicatorFill(breadcrumbItem.modelData.position)

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
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
