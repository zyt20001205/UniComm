pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQml.Models

import "../util"

import UniComm

Item {
    id: root
    anchors.fill: parent
    required property StructureModule structureModule
    readonly property var selectedStructure: root.structureModule.models[global.documentModule.focusedUrl] ?? null
    // null accepts every kind; an empty list accepts none.
    property var selectedKinds: null
    onSelectedStructureChanged: {
        if (selectedStructure) filterModel.model = selectedStructure.model
    }
    onSelectedKindsChanged: filterRefresh()

    Keys.onEscapePressed: event => {
        if (!toolBar.searchExpanded) {
            event.accepted = false
            return
        }
        toolBar.searchExpanded = false
        treeView.forceActiveFocus()
    }
    function filterRefresh() {
        filterModel.invalidate()
        treeView.selectedRow = -1
        treeView.contentY = 0
        if (searchField.query.length > 0)
            Qt.callLater(() => treeView.expandRecursively())
    }
    function kindIcon(kind: int): string {
        switch (kind) {
            case LspSymbolKind.Package:
                return "qrc:/icon/symbolPackage.svg"
            case LspSymbolKind.Method:
            case LspSymbolKind.Function:
                return "qrc:/icon/symbolMethod.svg"
            case LspSymbolKind.Variable:
                return "qrc:/icon/symbolVariable.svg"
            case LspSymbolKind.Constant:
                return "qrc:/icon/symbolConstant.svg"
            case LspSymbolKind.String:
                return "qrc:/icon/symbolString.svg"
            case LspSymbolKind.Number:
                return "qrc:/icon/symbolNumeric.svg"
            case LspSymbolKind.Boolean:
                return "qrc:/icon/symbolBoolean.svg"
            case LspSymbolKind.Array:
                return "qrc:/icon/symbolArray.svg"
            default:
                return "qrc:/icon/symbolMisc.svg"
        }
    }
    function kindColor(kind: int): string {
        const colors = global.theme.lua.structure
        switch (kind) {
            case LspSymbolKind.Package:
                return colors.package.fore
            case LspSymbolKind.Method:
                return colors.method.fore
            case LspSymbolKind.Function:
                return colors.function.fore
            case LspSymbolKind.Variable:
                return colors.variable.fore
            case LspSymbolKind.Constant:
                return colors.constant.fore
            case LspSymbolKind.String:
                return colors.string.fore
            case LspSymbolKind.Number:
                return colors.number.fore
            case LspSymbolKind.Boolean:
                return colors.boolean.fore
            case LspSymbolKind.Array:
                return colors.array.fore
            case LspSymbolKind.Object:
                return colors.object.fore
            default:
                return global.theme.general.fore
        }
    }

    Connections {
        target: root.structureModule

        function onClearSelection() {
            treeView.selectedRow = -1
        }
    }

    SortFilterProxyModel {
        id: filterModel
        recursiveFiltering: true
        onModelChanged: {
            treeView.selectedRow = -1
            treeView.contentY = 0
            if (searchField.query.length > 0)
                Qt.callLater(() => treeView.expandRecursively())
        }
        filters: FunctionFilter {
            column: 0
            component RoleData: QtObject {
                property int kind
                property string display
            }

            function filter(data: RoleData): bool {
                return (root.selectedKinds === null || root.selectedKinds.includes(data.kind))
                    && data.display.toLowerCase().includes(searchField.query)
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    ColumnLayout {
        anchors.fill: parent

        Item {
            id: toolBar
            property bool searchExpanded: false
            Layout.fillWidth: true; Layout.preferredHeight: 32
            clip: true

            RowLayout {
                id: buttons
                anchors.verticalCenter: parent.verticalCenter
                x: toolBar.searchExpanded ? 0 : toolBar.width - width
                spacing: 0

                Behavior on x {
                    NumberAnimation { duration: AnimationDuration.Move; easing.type: Easing.OutCubic }
                }

                Button {
                    id: collapseButton
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/collapse.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    onClicked: treeView.collapseRecursively()

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) mainTooltip.text = ""
                        }
                        onPointChanged: {
                            mainTooltip.position = collapseButton.mapToGlobal(point.position)
                            mainTooltip.text = qsTr("Collapse All")
                        }
                    }
                }

                Button {
                    id: expandButton
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/expand.svg"
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    onClicked: treeView.expandRecursively()

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) mainTooltip.text = ""
                        }
                        onPointChanged: {
                            mainTooltip.position = expandButton.mapToGlobal(point.position)
                            mainTooltip.text = qsTr("Expand All")
                        }
                    }
                }

                Button {
                    id: filterButton
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/filter.svg"
                    icon.color: root.selectedKinds !== null ? global.theme.general.warningBack3 : global.theme.general.fore
                    icon.width: 16; icon.height: 16
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                    onClicked: {
                        mainTooltip.text = ""
                        const position = filterButton.mapToGlobal(0, filterButton.height)
                        const localPosition = mainOverlay.mapFromGlobal(position)
                        filterMenu.popup(localPosition.x, localPosition.y)
                    }

                    HoverHandler {
                        enabled: !filterMenu.visible
                        onHoveredChanged: {
                            if (!hovered) mainTooltip.text = ""
                        }
                        onPointChanged: {
                            mainTooltip.position = filterButton.mapToGlobal(point.position)
                            mainTooltip.text = qsTr("Filter")
                        }
                    }
                }

                Button {
                    id: searchButton
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    flat: true
                    icon.source: "qrc:/icon/search.svg"
                    icon.color: searchField.query.length > 0 ? global.theme.general.warningBack3 : global.theme.general.fore
                    icon.width: 16; icon.height: 16

                    onClicked: {
                        mainTooltip.text = ""
                        toolBar.searchExpanded = !toolBar.searchExpanded
                        if (toolBar.searchExpanded) {
                            searchField.forceActiveFocus()
                            searchField.selectAll()
                        } else {
                            treeView.forceActiveFocus()
                        }
                    }

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) mainTooltip.text = ""
                        }
                        onPointChanged: {
                            mainTooltip.position = searchButton.mapToGlobal(point.position)
                            mainTooltip.text = searchField.query.length > 0 ? searchField.text : qsTr("Search")
                        }
                    }
                }
            }

            LineEdit {
                id: searchField
                anchors.left: buttons.right; anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 32
                clip: true
                visible: toolBar.searchExpanded || opacity > 0
                enabled: toolBar.searchExpanded
                opacity: toolBar.searchExpanded ? 1 : 0
                clearable: true
                tooltip: mainTooltip
                placeholderText: qsTr("Search symbols")
                readonly property string query: text.trim().toLowerCase()

                Behavior on opacity {
                    NumberAnimation { duration: AnimationDuration.Fade }
                }

                onQueryChanged: root.filterRefresh()
            }
        }

        TreeView {
            id: treeView
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true
            model: root.selectedStructure ? filterModel : null
            property int selectedRow: -1
            onModelChanged: {
                selectedRow = -1
                contentY = 0
            }

            ScrollBar.vertical: ScrollBar {
                id: treeScrollBar
                minimumSize: Math.min(1, 20 / Math.max(1, height))
                palette {
                    mid: global.theme.general.stroke
                    dark: global.theme.general.strokePressed
                }
            }

            delegate: Item {
                id: node
                implicitWidth: treeView.width; implicitHeight: 24
                required property bool isTreeNode
                required property bool expanded
                required property bool hasChildren
                required property int depth
                required property int row
                required property string display
                required property string detail
                required property int kind
                required property int startLine
                required property int startCharacter

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: global.theme.general.backHover
                    opacity: hoverHandler.hovered ? 1 : 0
                    Behavior on opacity {
                        NumberAnimation {
                            duration: AnimationDuration.Fade
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: treeView.selectedRow === node.row ? global.theme.general.backSelected : "transparent"
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.rightMargin: treeScrollBar.width
                    spacing: 0

                    Item {
                        Layout.preferredWidth: node.depth * 24; Layout.preferredHeight: 24
                    }

                    Item {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                        IconImage {
                            anchors.centerIn: parent
                            width: 16; height: 16
                            source: node.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                            color: global.theme.general.fore
                            visible: node.isTreeNode && node.hasChildren
                        }

                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            gesturePolicy: TapHandler.DragWithinBounds

                            onTapped: {
                                treeView.forceActiveFocus()
                                treeView.selectedRow = node.row
                                if (node.isTreeNode && node.hasChildren) {
                                    treeView.toggleExpanded(node.row)
                                }
                            }
                        }
                    }

                    Item {
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                        IconImage {
                            anchors.centerIn: parent
                            width: 20; height: 20
                            source: root.kindIcon(node.kind)
                            color: root.kindColor(node.kind)
                        }
                    }

                    Label {
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        text: node.display
                        elide: Text.ElideRight
                        Layout.fillWidth: true; Layout.preferredHeight: 24
                    }
                }

                HoverHandler {
                    id: hoverHandler
                    onHoveredChanged: {
                        if (!hovered) {
                            mainTooltip.text = ""
                        }
                    }
                    onPointChanged: {
                        mainTooltip.position = node.mapToGlobal(point.position)
                        mainTooltip.text = Qt.enumValueToString(LspSymbolKind.Value, node.kind)
                            + (node.detail ? ": " + node.detail : "")
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onTapped: {
                        treeView.forceActiveFocus()
                        treeView.selectedRow = node.row
                        const documentUrl = global.documentModule.focusedUrl
                        root.structureModule.setFocus(documentUrl, true)
                        root.structureModule.setIndex(documentUrl, node.startLine, node.startCharacter)
                        root.structureModule.addMarker(documentUrl, ScintillaMarker.Hint, node.startLine, 1000)
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    gesturePolicy: TapHandler.DragWithinBounds

                    onTapped: {
                        contextMenu.symbolName = node.display
                        contextMenu.detail = node.detail
                        contextMenu.popup()
                    }
                }
            }

            TapHandler {
                acceptedButtons: Qt.LeftButton

                onTapped: {
                    treeView.forceActiveFocus()
                    treeView.selectedRow = -1
                }
            }

        }
    }

    Menu {
        id: contextMenu
        parent: mainOverlay
        popupType: Popup.Item
        property string symbolName
        property string detail

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("Copy Name")
            icon.source: "qrc:/icon/copy.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.copyToClipboard(contextMenu.symbolName)
        }

        MenuItem {
            text: qsTr("Copy Detail")
            icon.source: "qrc:/icon/copy.svg"
            icon.width: 16; icon.height: 16
            enabled: contextMenu.detail.length > 0

            onTriggered: global.fileModule.copyToClipboard(contextMenu.detail)
        }
    }

    Menu {
        id: filterMenu
        parent: mainOverlay
        popupType: Popup.Item

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        ItemDelegate {
            width: filterMenu.availableWidth
            height: 30
            leftPadding: 12; rightPadding: 12; topPadding: 3; bottomPadding: 3
            spacing: 10
            text: qsTr("All")
            icon.source: "qrc:/icon/checkboxChecked.svg"
            icon.color: global.theme.general.fore
            icon.width: 16; icon.height: 16

            onClicked: root.selectedKinds = null
        }

        ItemDelegate {
            width: filterMenu.availableWidth
            height: 30
            leftPadding: 12; rightPadding: 12; topPadding: 3; bottomPadding: 3
            spacing: 10
            text: qsTr("None")
            icon.source: "qrc:/icon/checkboxUnchecked.svg"
            icon.color: global.theme.general.fore
            icon.width: 16; icon.height: 16

            onClicked: root.selectedKinds = []
        }

        Repeater {
            id: kindRepeater
            model: root.selectedStructure?.kinds ?? []

            delegate: CheckDelegate {
                id: kindDelegate
                required property int modelData
                width: filterMenu.availableWidth
                height: 30
                leftPadding: 12; rightPadding: 12; topPadding: 3; bottomPadding: 3
                text: Qt.enumValueToString(LspSymbolKind.Value, kindDelegate.modelData)
                icon.source: root.kindIcon(kindDelegate.modelData)
                icon.color: root.kindColor(kindDelegate.modelData)
                icon.width: 16; icon.height: 16
                checked: root.selectedKinds === null
                    || root.selectedKinds.includes(kindDelegate.modelData)

                onToggled: {
                    const kinds = root.selectedKinds ?? root.selectedStructure.kinds
                    root.selectedKinds = checked
                        ? kinds.concat(kindDelegate.modelData)
                        : kinds.filter(kind => kind !== kindDelegate.modelData)
                }
            }
        }
    }
}
