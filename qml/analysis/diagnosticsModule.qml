pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQml.Models

import UniComm

Item {
    id: root
    anchors.fill: parent
    required property DiagnosticsModule diagnosticsModule
    property int severityLimit: DiagnosticsModule.Level.Hint
    onSeverityLimitChanged: filterModel.invalidate()
    readonly property var selectedDiagnostic: root.diagnosticsModule.diagnostics[tabBar.currentIndex] ?? null
    onSelectedDiagnosticChanged: {
        if (selectedDiagnostic) filterModel.model = selectedDiagnostic.model
    }

    SortFilterProxyModel {
        id: filterModel
        onModelChanged: {
            tableView.contentX = 0
            tableView.contentY = 0
        }
        sorters: RoleSorter {
            id: severitySorter
            roleName: "severity"
            column: DiagnosticsModule.Col.Severity
            sortOrder: Qt.AscendingOrder
        }
        filters: FunctionFilter {
            column: DiagnosticsModule.Col.Severity
            enabled: root.severityLimit !== DiagnosticsModule.Level.Hint
            component RoleData: QtObject {
                property int severity
            }

            function filter(data: RoleData): bool {
                return data.severity <= root.severityLimit
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    Item {
        anchors.fill: parent
        visible: root.diagnosticsModule.diagnostics.length === 0

        RowLayout {
            anchors.centerIn: parent

            Label {
                text: qsTr("No errors found.")
                font.pixelSize: 16
                Layout.alignment: Qt.AlignVCenter
            }

            IconImage {
                source: "qrc:/icon/checkmarkCircle.svg"
                color: global.theme.general.successBack3
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: root.diagnosticsModule.diagnostics.length !== 0

        RowLayout {
            Layout.fillWidth: true; Layout.preferredHeight: 32

            TabBar {
                id: tabBar
                contentHeight: 24
                Layout.fillWidth: true; Layout.minimumWidth: 0
                Layout.preferredHeight: 24
                clip: true
                currentIndex: {
                    const diagnostics = root.diagnosticsModule.diagnostics
                    const focusedUrl = global.documentModule.focusedUrl
                    // Wait for the Repeater to finish rebuilding its tabs.
                    if (count !== diagnostics.length)
                        return -1
                    for (let i = 0; i < diagnostics.length; ++i) {
                        if (diagnostics[i].documentUrl === focusedUrl)
                            return i
                    }
                    return diagnostics.length ? 0 : -1
                }

                Repeater {
                    model: root.diagnosticsModule.diagnostics

                    delegate: TabButton {
                        required property var modelData
                        text: modelData.name
                        width: contentItem.implicitWidth + 24; height: 24
                    }
                }
            }

            RowLayout {
                id: toolbar
                Layout.alignment: Qt.AlignVCenter

                RowLayout {
                    Layout.rightMargin: 8

                    Repeater {
                        model: [
                            {field: "errors", icon: "error", color: global.theme.general.dangerBack3},
                            {field: "warnings", icon: "warning", color: global.theme.general.warningBack3},
                            {field: "infos", icon: "info", color: global.theme.general.brandBack},
                            {field: "hints", icon: "hint", color: global.theme.general.fore}
                        ]

                        delegate: RowLayout {
                            id: statistic
                            required property var modelData
                            readonly property int count: root.selectedDiagnostic?.[modelData.field] ?? 0
                            visible: count > 0

                            IconImage {
                                source: "qrc:/icon/" + statistic.modelData.icon + ".svg"
                                color: statistic.modelData.color
                                Layout.preferredWidth: 16; Layout.preferredHeight: 16
                            }

                            Label {
                                text: statistic.count
                            }
                        }
                    }
                }

                Button {
                    id: sortButton
                    flat: true
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    icon.source: severitySorter.sortOrder === Qt.AscendingOrder
                        ? "qrc:/icon/arrowSortUpLines.svg" : "qrc:/icon/arrowSortDownLines.svg"
                    icon.width: 16; icon.height: 16

                    onClicked: {
                        mainTooltip.text = ""
                        severitySorter.sortOrder = severitySorter.sortOrder === Qt.AscendingOrder
                            ? Qt.DescendingOrder : Qt.AscendingOrder
                    }

                    HoverHandler {
                        onHoveredChanged: {
                            if (!hovered) mainTooltip.text = ""
                        }
                        onPointChanged: {
                            mainTooltip.position = sortButton.mapToGlobal(point.position)
                            mainTooltip.text = severitySorter.sortOrder === Qt.AscendingOrder
                                ? qsTr("Ascending") : qsTr("Descending")
                        }
                    }
                }

                Button {
                    id: filterButton
                    flat: true
                    Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                    icon.source: "qrc:/icon/filter.svg"
                    icon.color: root.severityLimit !== DiagnosticsModule.Level.Hint
                        ? global.theme.general.warningBack3 : global.theme.general.fore
                    icon.width: 16; icon.height: 16

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
            }
        }

        Item {
            Layout.fillWidth: true; Layout.fillHeight: true

            HorizontalHeaderView {
                id: horizontalHeaderView
                anchors.left: tableView.left; anchors.right: tableView.right; anchors.top: parent.top
                height: 32
                syncView: tableView
                clip: true
                interactive: false
                resizableColumns: true
                columnSpacing: 0
                editTriggers: TableView.NoEditTriggers

                delegate: Item {
                    id: headerCell
                    required property string display
                    implicitWidth: 120; implicitHeight: 32

                    Rectangle {
                        anchors.fill: parent
                        color: global.theme.general.back
                    }

                    Label {
                        anchors.fill: parent
                        text: headerCell.display
                        leftPadding: 6; rightPadding: 6
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
            }

            TableView {
                id: tableView
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: horizontalHeaderView.bottom; anchors.bottom: parent.bottom
                model: root.selectedDiagnostic ? filterModel : null
                rowSpacing: 1; columnSpacing: 0
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                editTriggers: TableView.NoEditTriggers

                readonly property var defaultColumnWidths: ({
                    [DiagnosticsModule.Col.Severity]: 24,
                    [DiagnosticsModule.Col.Source]: 120,
                    [DiagnosticsModule.Col.Code]: 100,
                    [DiagnosticsModule.Col.Data]: 120
                })

                columnWidthProvider: function (column: int): real {
                    if (column !== DiagnosticsModule.Col.Message) {
                        const explicitWidth = explicitColumnWidth(column)
                        return explicitWidth >= 0 ? explicitWidth : defaultColumnWidths[column]
                    }

                    let remainingWidth = Math.floor(width)
                    for (let i = DiagnosticsModule.Col.Severity; i < DiagnosticsModule.Col.Message; ++i) {
                        const explicitWidth = explicitColumnWidth(i)
                        remainingWidth -= explicitWidth >= 0 ? explicitWidth : defaultColumnWidths[i]
                    }
                    return Math.max(160, remainingWidth)
                }

                onWidthChanged: forceLayout()

                Rectangle {
                    parent: tableView.contentItem
                    anchors.fill: parent
                    color: global.theme.general.stroke
                }

                ScrollBar.vertical: ScrollBar {
                    id: verticalScrollBar
                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                    palette {
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
                    }
                }

                ScrollBar.horizontal: ScrollBar {
                    minimumSize: Math.min(1, 20 / Math.max(1, width))
                    palette {
                        mid: global.theme.general.stroke
                        dark: global.theme.general.strokePressed
                    }
                }

                delegate: Item {
                    id: cell
                    required property int column
                    required property var display
                    required property int severity
                    required property url documentUrl
                    required property int startLine
                    required property int startCharacter
                    required property int endLine
                    required property int endCharacter
                    required property string message
                    implicitWidth: 120; implicitHeight: 24

                    Rectangle {
                        anchors.fill: parent
                        color: global.theme.general.back
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 6
                        color: global.theme.general.backHover
                        opacity: hoverHandler.hovered ? 1 : 0
                        Behavior on opacity {
                            NumberAnimation {
                                duration: 150
                            }
                        }
                    }

                    IconImage {
                        anchors.centerIn: parent
                        width: 16; height: 16
                        visible: cell.column === DiagnosticsModule.Col.Severity
                        source: {
                            switch (cell.severity) {
                                case DiagnosticsModule.Level.Error:
                                    return "qrc:/icon/error.svg"
                                case DiagnosticsModule.Level.Warning:
                                    return "qrc:/icon/warning.svg"
                                case DiagnosticsModule.Level.Information:
                                    return "qrc:/icon/info.svg"
                                case DiagnosticsModule.Level.Hint:
                                    return "qrc:/icon/hint.svg"
                                default:
                                    return ""
                            }
                        }
                        color: {
                            switch (cell.severity) {
                                case DiagnosticsModule.Level.Error:
                                    return global.theme.general.dangerBack3
                                case DiagnosticsModule.Level.Warning:
                                    return global.theme.general.warningBack3
                                case DiagnosticsModule.Level.Information:
                                    return global.theme.general.brandBack
                                default:
                                    return global.theme.general.fore
                            }
                        }
                    }

                    Label {
                        anchors.fill: parent
                        visible: cell.column !== DiagnosticsModule.Col.Severity
                        text: String(cell.display ?? "")
                        textFormat: cell.column === DiagnosticsModule.Col.Message ? Text.RichText : Text.PlainText
                        leftPadding: 6; rightPadding: 6 + (cell.column === DiagnosticsModule.Col.Message ? verticalScrollBar.width : 0)
                        horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    HoverHandler {
                        id: hoverHandler
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        onTapped: {
                            root.diagnosticsModule.setIndex(cell.documentUrl, cell.startLine, cell.startCharacter)
                            root.diagnosticsModule.fillIndicator(cell.documentUrl, ScintillaIndicator.Highlight, cell.startLine, cell.startCharacter, cell.endLine, cell.endCharacter, 1000)
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: {
                            contextMenu.documentUrl = cell.documentUrl
                            contextMenu.startLine = cell.startLine
                            contextMenu.startCharacter = cell.startCharacter
                            contextMenu.endLine = cell.endLine
                            contextMenu.endCharacter = cell.endCharacter
                            contextMenu.diagnostic = cell.message
                            contextMenu.popup()
                        }
                    }
                }
            }

            Item {
                anchors.fill: tableView
                visible: tableView.rows === 0

                RowLayout {
                    anchors.centerIn: parent

                    Label {
                        text: qsTr("No diagnostics match the filter.")
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignVCenter
                    }

                    IconImage {
                        source: "qrc:/icon/filter.svg"
                        color: global.theme.general.fore
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }
        }
    }

    Menu {
        id: contextMenu
        parent: mainOverlay
        popupType: Popup.Item
        property string diagnostic
        property url documentUrl
        property int startLine
        property int startCharacter
        property int endLine
        property int endCharacter

        onOpened: {
            global.mainWindow.overlayFlagSet(false, true)
            mainOverlay.widgetCount += 1
        }
        onClosed: mainOverlay.widgetCount -= 1

        MenuItem {
            text: qsTr("Copy")
            icon.source: "qrc:/icon/copy.svg"
            icon.width: 16; icon.height: 16

            onTriggered: global.fileModule.copyToClipboard(contextMenu.diagnostic)
        }

        MenuItem {
            text: qsTr("View")
            icon.source: "qrc:/icon/eye.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                root.diagnosticsModule.setIndex(contextMenu.documentUrl, contextMenu.startLine, contextMenu.startCharacter)
                root.diagnosticsModule.fillIndicator(contextMenu.documentUrl, ScintillaIndicator.Highlight, contextMenu.startLine, contextMenu.startCharacter, contextMenu.endLine, contextMenu.endCharacter, 1000)
            }
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

        ButtonGroup {
            id: filterGroup
        }

        MenuItem {
            text: qsTr("Only")
            icon.source: "qrc:/icon/error.svg"
            icon.color: global.theme.general.dangerBack3
            icon.width: 16; icon.height: 16
            checkable: true
            checked: root.severityLimit === DiagnosticsModule.Level.Error
            ButtonGroup.group: filterGroup
            onTriggered: root.severityLimit = DiagnosticsModule.Level.Error
        }

        MenuItem {
            text: qsTr("Above")
            icon.source: "qrc:/icon/warning.svg"
            icon.color: global.theme.general.warningBack3
            icon.width: 16; icon.height: 16
            checkable: true
            checked: root.severityLimit === DiagnosticsModule.Level.Warning
            ButtonGroup.group: filterGroup
            onTriggered: root.severityLimit = DiagnosticsModule.Level.Warning
        }

        MenuItem {
            text: qsTr("Above")
            icon.source: "qrc:/icon/info.svg"
            icon.color: global.theme.general.brandBack
            icon.width: 16; icon.height: 16
            checkable: true
            checked: root.severityLimit === DiagnosticsModule.Level.Information
            ButtonGroup.group: filterGroup
            onTriggered: root.severityLimit = DiagnosticsModule.Level.Information
        }

        MenuItem {
            text: qsTr("All")
            icon.source: "qrc:/icon/filter.svg"
            icon.color: global.theme.general.fore
            icon.width: 16; icon.height: 16
            checkable: true
            checked: root.severityLimit === DiagnosticsModule.Level.Hint
            ButtonGroup.group: filterGroup
            onTriggered: root.severityLimit = DiagnosticsModule.Level.Hint
        }
    }
}
