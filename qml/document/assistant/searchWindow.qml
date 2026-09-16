import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        RowLayout {
            id: searchBar

            Item {
                Layout.preferredWidth: 400; Layout.preferredHeight: 24

                TextField {
                    id: searchTextField
                    anchors.fill: parent
                    rightPadding: 120

                    onTextChanged: searchWindow.searchRequest()
                }

                RowLayout {
                    anchors.right: parent.right
                    spacing: 0

                    Button {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        flat: true
                        focusPolicy: Qt.NoFocus
                        icon.source: "qrc:/icon/dismiss.svg"
                        icon.width: 12; icon.height: 12

                        onClicked: searchTextField.clear()
                    }

                    Button {
                        id: matchCaseButton
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        checkable: true
                        flat: true
                        icon.source: "qrc:/icon/matchCase.svg"
                        icon.width: 16; icon.height: 16

                        onClicked: {
                            searchWindow.searchFlagsSet(matchCaseButton.checked, wholeWordButton.checked, wordStartButton.checked, regExpButton.checked)
                            searchWindow.searchRequest()
                        }

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainToolTip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainToolTip.position = parent.mapToGlobal(point.position)
                                mainToolTip.text = parent.checked ? qsTr("Disable Match Case") : qsTr("Enable Match Case")
                            }
                        }
                    }

                    Button {
                        id: wholeWordButton
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        checkable: true
                        flat: true
                        icon.source: "qrc:/icon/wholeWord.svg"
                        icon.width: 16; icon.height: 16

                        onClicked: {
                            searchWindow.searchFlagsSet(matchCaseButton.checked, wholeWordButton.checked, wordStartButton.checked, regExpButton.checked)
                            searchWindow.searchRequest()
                        }

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainToolTip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainToolTip.position = parent.mapToGlobal(point.position)
                                mainToolTip.text = parent.checked ? qsTr("Disable Whole Word") : qsTr("Enable Whole Word")
                            }
                        }
                    }

                    Button {
                        id: wordStartButton
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        checkable: true
                        flat: true
                        icon.source: "qrc:/icon/wordStart.svg"
                        icon.width: 16; icon.height: 16

                        onClicked: {
                            searchWindow.searchFlagsSet(matchCaseButton.checked, wholeWordButton.checked, wordStartButton.checked, regExpButton.checked)
                            searchWindow.searchRequest()
                        }

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainToolTip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainToolTip.position = parent.mapToGlobal(point.position)
                                mainToolTip.text = parent.checked ? qsTr("Disable Word Start") : qsTr("Enable Word Start")
                            }
                        }
                    }

                    Button {
                        id: regExpButton
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        checkable: true
                        flat: true
                        icon.source: "qrc:/icon/regExp.svg"
                        icon.width: 16; icon.height: 16

                        onClicked: {
                            searchWindow.searchFlagsSet(matchCaseButton.checked, wholeWordButton.checked, wordStartButton.checked, regExpButton.checked)
                            searchWindow.searchRequest()
                        }

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainToolTip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainToolTip.position = parent.mapToGlobal(point.position)
                                mainToolTip.text = parent.checked ? qsTr("Disable Regular Expression") : qsTr("Enable Regular Expression")
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                id: searchStatLabel
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                text: searchModel.empty ? "0/0" : (tableView.selectedRow + 1) + "/" + matchCount
                Layout.preferredHeight: 24
                property int matchCount
            }
        }

        Item {
            Layout.fillWidth: true; Layout.fillHeight: true

            Label {
                visible: searchModel.empty
                anchors.fill: parent
                text: qsTr("Nothing Found")
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }

            TableView {
                id: tableView
                visible: !searchModel.empty
                anchors.fill: parent
                alternatingRows: false
                clip: true
                editTriggers: TableView.NoEditTriggers
                model: searchModel
                contentWidth: width
                property int hoveredRow: -1
                property int selectedRow: -1

                ScrollBar.vertical: ScrollBar {
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                columnWidthProvider: function (column) {
                    if (column !== columns - 1) return implicitColumnWidth(column)
                    let usedWidth = 0
                    for (let i = 0; i < columns - 1; ++i) usedWidth += implicitColumnWidth(i)
                    return Math.max(implicitColumnWidth(column), width - usedWidth)
                }

                delegate: Item {
                    implicitWidth: Math.max(textMetrics.width + 16, 60)
                    implicitHeight: 24
                    required property int column
                    required property int row

                    Rectangle {
                        anchors.fill: parent
                        color: global.back
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        topLeftRadius: column === 0 ? radius : 0
                        bottomLeftRadius: column === 0 ? radius : 0
                        topRightRadius: column === tableView.columns - 1 ? radius : 0
                        bottomRightRadius: column === tableView.columns - 1 ? radius : 0
                        color: global.backHover
                        opacity: tableView.hoveredRow === row ? 1 : 0
                        Behavior on opacity {
                            NumberAnimation {
                                duration: 150
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        topLeftRadius: column === 0 ? radius : 0
                        bottomLeftRadius: column === 0 ? radius : 0
                        topRightRadius: column === tableView.columns - 1 ? radius : 0
                        bottomRightRadius: column === tableView.columns - 1 ? radius : 0
                        color: tableView.selectedRow === row ? global.backSelected : "transparent"
                    }

                    TextMetrics {
                        id: textMetrics
                        font: label.font
                        text: model.display || ""
                    }

                    Label {
                        id: label
                        anchors.fill: parent
                        leftPadding: 6
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: model.display || ""
                        textFormat: column === 2 ? Text.RichText : Text.PlainText
                        elide: Text.ElideRight
                    }

                    HoverHandler {
                        onHoveredChanged: {
                            if (hovered) {
                                tableView.hoveredRow = row
                            } else if (tableView.hoveredRow === row) {
                                tableView.hoveredRow = -1
                            }
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        gesturePolicy: TapHandler.DragWithinBounds

                        onSingleTapped: tableView.selectedRow = row

                        onDoubleTapped: searchWindow.searchNavigate(model.documentUrl, model.line)
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton

                    onTapped: tableView.selectedRow = -1
                }
            }
        }
    }

    Component.onCompleted: {
        const objects = {
            "searchBar": searchBar,
            "searchTextField": searchTextField,
            "searchStatLabel": searchStatLabel
        };
        searchWindow.propertyGet(objects)
    }
}
