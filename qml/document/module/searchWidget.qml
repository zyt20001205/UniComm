import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: rootItem
    required property QtObject searchWidget

    Rectangle {
        anchors.fill: parent
        color: global.theme.back
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6

        RowLayout {
            id: searchBar

            Item {
                Layout.preferredWidth: 600; Layout.preferredHeight: 24

                TextField {
                    id: searchTextField
                    anchors.fill: parent
                    rightPadding: 120
                    KeyNavigation.tab: replaceTextField

                    onTextChanged: rootItem.searchWidget.searchRequest()
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
                            rootItem.searchWidget.searchFlagsSet(matchCaseButton.checked, wholeWordButton.checked, wordStartButton.checked, regExpButton.checked)
                            rootItem.searchWidget.searchRequest()
                        }

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainTooltip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainTooltip.position = parent.mapToGlobal(point.position)
                                mainTooltip.text = parent.checked ? qsTr("Disable Match Case") : qsTr("Enable Match Case")
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
                            rootItem.searchWidget.searchFlagsSet(matchCaseButton.checked, wholeWordButton.checked, wordStartButton.checked, regExpButton.checked)
                            rootItem.searchWidget.searchRequest()
                        }

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainTooltip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainTooltip.position = parent.mapToGlobal(point.position)
                                mainTooltip.text = parent.checked ? qsTr("Disable Whole Word") : qsTr("Enable Whole Word")
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
                            rootItem.searchWidget.searchFlagsSet(matchCaseButton.checked, wholeWordButton.checked, wordStartButton.checked, regExpButton.checked)
                            rootItem.searchWidget.searchRequest()
                        }

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainTooltip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainTooltip.position = parent.mapToGlobal(point.position)
                                mainTooltip.text = parent.checked ? qsTr("Disable Word Start") : qsTr("Enable Word Start")
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
                            rootItem.searchWidget.searchFlagsSet(matchCaseButton.checked, wholeWordButton.checked, wordStartButton.checked, regExpButton.checked)
                            rootItem.searchWidget.searchRequest()
                        }

                        HoverHandler {
                            onHoveredChanged: {
                                if (!hovered) {
                                    mainTooltip.text = ""
                                }
                            }
                            onPointChanged: {
                                mainTooltip.position = parent.mapToGlobal(point.position)
                                mainTooltip.text = parent.checked ? qsTr("Disable Regular Expression") : qsTr("Enable Regular Expression")
                            }
                        }
                    }
                }
            }

            Button {
                id: searchPrevButton
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                enabled: false
                flat: true
                icon.source: "qrc:/icon/arrowUp.svg"
                icon.width: 16; icon.height: 16

                onClicked: rootItem.searchWidget.searchPrev()

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Search Previous")
                    }
                }
            }

            Button {
                id: searchNextButton
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                enabled: false
                flat: true
                icon.source: "qrc:/icon/arrowDown.svg"
                icon.width: 16; icon.height: 16

                onClicked: rootItem.searchWidget.searchNext()

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Search Next")
                    }
                }
            }

            Label {
                id: searchStatLabel
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                text: "0/0"
                Layout.preferredWidth: 80; Layout.preferredHeight: 24
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {
            id: replaceBar

            Item {
                Layout.preferredWidth: 600; Layout.preferredHeight: 24

                TextField {
                    id: replaceTextField
                    anchors.fill: parent
                    rightPadding: 24
                    
                    KeyNavigation.tab: matchCaseButton
                }

                RowLayout {
                    anchors.right: parent.right
                    spacing: 0

                    Button {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                        flat: true
                        // focusPolicy: Qt.NoFocus
                        icon.source: "qrc:/icon/dismiss.svg"
                        icon.width: 12; icon.height: 12

                        onClicked: replaceTextField.clear()
                    }
                }
            }

            Button {
                id: replaceTextButton
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                enabled: false
                flat: true
                icon.source: "qrc:/icon/replace.svg"
                icon.width: 16; icon.height: 16

                onClicked: rootItem.searchWidget.textReplace()

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Replace")
                    }
                }
            }

            Button {
                id: replaceAllButton
                Layout.preferredWidth: 24; Layout.preferredHeight: 24
                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                enabled: false
                flat: true
                icon.source: "qrc:/icon/replaceAll.svg"
                icon.width: 16; icon.height: 16

                onClicked: rootItem.searchWidget.allReplace()

                HoverHandler {
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = parent.mapToGlobal(point.position)
                        mainTooltip.text = qsTr("Replace All")
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    Component.onCompleted: {
        const objects = {
            "searchBar": searchBar,
            "searchTextField": searchTextField,
            "searchPrevButton": searchPrevButton,
            "searchNextButton": searchNextButton,
            "searchStatLabel": searchStatLabel,
            "replaceBar": replaceBar,
            "replaceTextField": replaceTextField,
            "replaceTextButton": replaceTextButton,
            "replaceAllButton": replaceAllButton
        };
        rootItem.searchWidget.propertyGet(objects)
    }
}
