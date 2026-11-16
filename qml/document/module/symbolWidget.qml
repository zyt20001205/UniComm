import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

Item {
    id: rootItem

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
            id: symbolBreadcrumb

            Component {
                id: breadcrumbComponent

                RowLayout {
                    id: breadcrumbItem
                    Layout.alignment: Qt.AlignVCenter
                    property string text
                    property string source
                    property string detail
                    property var position

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
                        icon.source: breadcrumbItem.source
                        icon.width: 16; icon.height: 16
                        text: breadcrumbItem.text
                        Layout.preferredWidth: breadcrumbButtonTextMetrics.width + 28; Layout.preferredHeight: 20

                        onClicked: symbolWidget.indicatorFill(breadcrumbItem.position)

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainToolTip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainToolTip.position = parent.mapToGlobal(point.position)
                                mainToolTip.text = breadcrumbItem.detail
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

    function symbolLoad(symbolList) {
        symbolBreadcrumb.children = [];
        for (const symbol of symbolList) {
            const item = breadcrumbComponent.createObject(symbolBreadcrumb, {
                "text": symbol.text,
                "source": symbol.source,
                "detail": symbol.detail,
                "position": symbol.position
            });
        }
    }
}