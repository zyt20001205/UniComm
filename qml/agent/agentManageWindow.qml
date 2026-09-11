import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: rootItem
    anchors.fill: parent

    FileDialog {
        id: hookScriptDialog
        property int event
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Lua files (*.lua)")]

        onAccepted: agentModule.hookScriptInsert(event, selectedFile)
    }

    Rectangle {
        anchors.fill: parent
        color: global.back
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal
        handle: Item {
            implicitWidth: 5

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: global.stroke
            }
        }

        Rectangle {
            color: global.back
            SplitView.preferredWidth: 180; SplitView.fillHeight: true

            ListView {
                id: navigationView
                anchors.fill: parent
                anchors.margins: 10
                clip: true
                currentIndex: 0
                model: [
                    {"title": qsTr("Models"), "icon": "qrc:/icon/model.svg"},
                    {"title": qsTr("MCP"), "icon": "qrc:/icon/mcp.svg"},
                    {"title": qsTr("Skills"), "icon": "qrc:/icon/skill.svg"},
                    {"title": qsTr("Hooks"), "icon": "qrc:/icon/hook.svg"},
                    {"title": qsTr("Context"), "icon": "qrc:/icon/database.svg"},
                    {"title": qsTr("Runtime"), "icon": "qrc:/icon/pulse.svg"}
                ]
                spacing: 2

                delegate: ItemDelegate {
                    required property int index
                    required property var modelData
                    width: navigationView.width
                    height: 36
                    text: modelData.title
                    icon.source: modelData.icon
                    icon.width: 18
                    icon.height: 18
                    spacing: 10
                    highlighted: navigationView.currentIndex === index

                    onClicked: navigationView.currentIndex = index
                }
            }
        }

        StackLayout {
            currentIndex: navigationView.currentIndex
            SplitView.fillWidth: true; SplitView.fillHeight: true

            SettingsPage {
                title: qsTr("Models")
                description: qsTr("Configure model providers, credentials, and available models.")
                actionText: qsTr("Add Provider")
                actionIcon: "qrc:/icon/add.svg"

                onTriggerAction: providerInsertDialog.openInsert()

                Repeater {
                    model: providerModel

                    delegate: Rectangle {
                        id: providerCard
                        property string providerId: model.id
                        property string providerName: model.display
                        property url providerIcon: model.decoration
                        property string providerApikey: model.apikey
                        property url providerBaseUrl: model.baseUrl
                        property bool providerCustom: model.custom
                        property string providerChatEndpoint: model.chatEndpoint
                        property string providerModelEndpoint: model.modelEndpoint
                        property var providerConfig: model.config
                        property var providerModels: model.models

                        function configGet(): var {
                            const config = {}
                            for (const key in providerConfig) config[key] = providerConfig[key]
                            return config
                        }

                        function edit(key: string, label: string, value: string, allowEmpty: bool): void {
                            providerEditDialog.openEdit(
                                providerId,
                                key,
                                label,
                                value,
                                configGet(),
                                allowEmpty
                            )
                        }

                        function modelRemove(modelId: string): void {
                            const config = configGet()
                            const models = {}
                            const currentModels = config.models || {}
                            for (const id in currentModels) {
                                if (id !== modelId) models[id] = currentModels[id]
                            }
                            config.models = models
                            agentModule.providerEdit(providerId, config)
                        }

                        radius: 6
                        color: global.backHover
                        border.color: global.stroke
                        implicitHeight: providerLayout.implicitHeight + 32
                        Layout.fillWidth: true

                        RowLayout {
                            id: providerLayout
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12

                            Rectangle {
                                radius: 6
                                color: global.backSelected
                                Layout.preferredWidth: 42
                                Layout.preferredHeight: 42
                                Layout.alignment: Qt.AlignTop

                                IconImage {
                                    anchors.centerIn: parent
                                    width: 24
                                    height: 24
                                    source: providerCard.providerIcon
                                }
                            }

                            ColumnLayout {
                                spacing: 10
                                Layout.fillWidth: true

                                RowLayout {
                                    spacing: 8
                                    Layout.fillWidth: true

                                    Label {
                                        text: providerCard.providerName
                                        font.bold: true
                                    }

                                    Button {
                                        visible: providerCard.providerCustom
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        flat: true
                                        icon.source: "qrc:/icon/edit.svg"
                                        icon.width: 16; icon.height: 16
                                        Layout.preferredWidth: 20; Layout.preferredHeight: 20

                                        onClicked: providerCard.edit(
                                            "name",
                                            qsTr("Provider name"),
                                            providerCard.providerName,
                                            false
                                        )
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }

                                    Button {
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        checkable: true
                                        flat: true
                                        icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/delete.svg"
                                        icon.width: 16; icon.height: 16
                                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                        onToggled: {
                                            if (!checked) agentModule.providerRemove(providerCard.providerId)
                                        }

                                        Timer {
                                            interval: 1000
                                            running: parent.checked
                                            onTriggered: parent.checked = false
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: qsTr("Base URL: %1").arg(providerCard.providerBaseUrl.toString())
                                        color: global.stroke
                                        elide: Text.ElideRight
                                    }

                                    Button {
                                        visible: providerCard.providerCustom
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        flat: true
                                        icon.source: "qrc:/icon/edit.svg"
                                        icon.width: 16; icon.height: 16
                                        Layout.preferredWidth: 20; Layout.preferredHeight: 20

                                        onClicked: providerCard.edit(
                                            "api",
                                            qsTr("Base URL"),
                                            providerCard.providerBaseUrl.toString(),
                                            false
                                        )
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: qsTr("Chat endpoint: %1").arg(providerCard.providerChatEndpoint)
                                        color: global.stroke
                                        elide: Text.ElideRight
                                    }

                                    Button {
                                        visible: providerCard.providerCustom
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        flat: true
                                        icon.source: "qrc:/icon/edit.svg"
                                        icon.width: 16; icon.height: 16
                                        Layout.preferredWidth: 20; Layout.preferredHeight: 20

                                        onClicked: providerCard.edit(
                                            "chatEndpoint",
                                            qsTr("Chat endpoint"),
                                            providerCard.providerChatEndpoint,
                                            false
                                        )
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: qsTr("Model endpoint: %1").arg(providerCard.providerModelEndpoint)
                                        color: global.stroke
                                        elide: Text.ElideRight
                                    }

                                    Button {
                                        visible: providerCard.providerCustom
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        flat: true
                                        icon.source: "qrc:/icon/edit.svg"
                                        icon.width: 16; icon.height: 16
                                        Layout.preferredWidth: 20; Layout.preferredHeight: 20

                                        onClicked: providerCard.edit(
                                            "modelsEndpoint",
                                            qsTr("Model endpoint"),
                                            providerCard.providerModelEndpoint,
                                            true
                                        )
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }
                                }

                                Rectangle {
                                    color: global.stroke
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                }

                                Label {
                                    text: qsTr("API Key")
                                    font.bold: true
                                }

                                RowLayout {
                                    spacing: 8
                                    Layout.fillWidth: true

                                    TextField {
                                        id: apikeyTextField
                                        text: providerCard.providerApikey
                                        placeholderText: qsTr("Enter API key")
                                        echoMode: TextInput.Password
                                        Layout.fillWidth: true

                                        onAccepted: agentModule.apikeySet(providerCard.providerId, text)
                                    }

                                    Button {
                                        text: qsTr("Save")
                                        enabled: apikeyTextField.text.length > 0

                                        onClicked: agentModule.apikeySet(providerCard.providerId, apikeyTextField.text)
                                    }
                                }

                                Rectangle {
                                    color: global.stroke
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 32 + providerCard.providerModels.rows * 38

                                    HorizontalHeaderView {
                                        id: providerModelHeader
                                        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                                        height: 32
                                        syncView: providerModelTable
                                        interactive: false
                                        columnSpacing: 0
                                        clip: true

                                        delegate: HorizontalHeaderViewDelegate {
                                            id: providerModelHeaderCell
                                            required property int index
                                            implicitHeight: 32
                                            padding: 0

                                            background: Rectangle {
                                                color: "transparent"
                                            }

                                            contentItem: Item {
                                                Label {
                                                    anchors.fill: parent
                                                    visible: providerModelHeaderCell.index < 7
                                                    text: [qsTr("Model"), qsTr("Context"), qsTr("Output"), qsTr("Input"), qsTr("Main"), qsTr("Sub"), qsTr("Vision"), ""][providerModelHeaderCell.index]
                                                    font.bold: true
                                                    leftPadding: 8; rightPadding: 8
                                                    horizontalAlignment: providerModelHeaderCell.index === 0 ? Text.AlignLeft : Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                    elide: Text.ElideRight
                                                }

                                                Button {
                                                    anchors.centerIn: parent
                                                    visible: providerModelHeaderCell.index === 7
                                                             && providerCard.providerModelEndpoint.length > 0
                                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                                    flat: true
                                                    icon.source: "qrc:/icon/arrowClockwise.svg"
                                                    icon.width: 16; icon.height: 16
                                                    width: 20; height: 20

                                                    onClicked: agentModule.providerModelsGet(providerCard.providerId)
                                                }

                                                Button {
                                                    anchors.centerIn: parent
                                                    visible: providerModelHeaderCell.index === 7
                                                             && providerCard.providerCustom
                                                             && providerCard.providerModelEndpoint.length === 0
                                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                                    flat: true
                                                    icon.source: "qrc:/icon/add.svg"
                                                    icon.width: 16; icon.height: 16
                                                    width: 20; height: 20

                                                    onClicked: providerModelInsertDialog.openInsert(
                                                        providerCard.providerId,
                                                        providerCard.configGet()
                                                    )
                                                }
                                            }
                                        }
                                    }

                                    TableView {
                                        id: providerModelTable
                                        anchors.left: parent.left; anchors.right: parent.right
                                        anchors.top: providerModelHeader.bottom; anchors.bottom: parent.bottom
                                        model: providerCard.providerModels
                                        alternatingRows: false
                                        interactive: false
                                        reuseItems: false
                                        rowSpacing: 0; columnSpacing: 0
                                        boundsBehavior: Flickable.StopAtBounds
                                        clip: true

                                        columnWidthProvider: function(column: int): real {
                                            switch (column) {
                                            case 0: return Math.max(160, width - 508)
                                            case 1: return 108
                                            case 2: return 100
                                            case 3: return 100
                                            case 4: return 60
                                            case 5: return 60
                                            case 6: return 60
                                            case 7: return 20
                                            }
                                        }

                                        rowHeightProvider: function(row: int): real {
                                            return 38
                                        }

                                        delegate: TableViewDelegate {
                                            id: providerModelCell
                                            implicitHeight: 38
                                            padding: 0

                                            background: Rectangle {
                                                color: providerModelCell.row % 2 === 0 ? global.back : "transparent"
                                            }

                                            contentItem: Loader {
                                                sourceComponent: {
                                                    switch (providerModelCell.column) {
                                                    case 0: return modelCellComponent
                                                    case 1: return numberCellComponent
                                                    case 2: return numberCellComponent
                                                    case 3: return inputCellComponent
                                                    case 4: return primaryCellComponent
                                                    case 5: return subagentCellComponent
                                                    case 6: return visionCellComponent
                                                    case 7: return actionCellComponent
                                                    }
                                                }
                                            }

                                            Component {
                                                id: modelCellComponent

                                                ColumnLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 8; anchors.rightMargin: 8
                                                    spacing: 0

                                                    Label {
                                                        text: providerModelCell.model.display
                                                        elide: Text.ElideRight
                                                        Layout.fillWidth: true
                                                    }

                                                    Label {
                                                        text: providerModelCell.model.modelId
                                                        color: global.stroke
                                                        font.pixelSize: 11
                                                        elide: Text.ElideRight
                                                        Layout.fillWidth: true
                                                    }
                                                }
                                            }

                                            Component {
                                                id: numberCellComponent

                                                Label {
                                                    text: Number(providerModelCell.model.display).toLocaleString(Qt.locale(), "f", 0)
                                                    color: global.stroke
                                                    horizontalAlignment: Text.AlignHCenter
                                                    verticalAlignment: Text.AlignVCenter
                                                }
                                            }

                                            Component {
                                                id: inputCellComponent

                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 8; anchors.rightMargin: 8
                                                    spacing: 4

                                                    Repeater {
                                                        model: providerModelCell.model.input

                                                        delegate: Image {
                                                            required property string modelData
                                                            source: {
                                                                switch (modelData) {
                                                                case "audio": return "qrc:/icon/fileTypeAudio.svg"
                                                                case "image": return "qrc:/icon/fileTypeImage.svg"
                                                                case "pdf": return "qrc:/icon/fileTypePdf.svg"
                                                                case "text": return "qrc:/icon/fileTypeTxt.svg"
                                                                case "video": return "qrc:/icon/fileTypeVideo.svg"
                                                                }
                                                            }
                                                            sourceSize.width: 16; sourceSize.height: 16
                                                            Layout.preferredWidth: 16; Layout.preferredHeight: 16

                                                            HoverHandler {
                                                                onHoveredChanged: {
                                                                    if (!hovered) mainToolTip.text = ""
                                                                }
                                                                onPointChanged: {
                                                                    mainToolTip.position = parent.mapToGlobal(point.position)
                                                                    mainToolTip.text = qsTr("%1 input").arg(modelData[0].toUpperCase() + modelData.slice(1))
                                                                }
                                                            }
                                                        }
                                                    }

                                                    Item {
                                                        Layout.fillWidth: true
                                                    }
                                                }
                                            }

                                            Component {
                                                id: primaryCellComponent

                                                RadioButton {
                                                    anchors.centerIn: parent
                                                    width: implicitWidth; height: implicitHeight
                                                    checked: providerModelCell.model.primary

                                                    onClicked: agentModule.primaryModelSet(providerCard.providerId, providerModelCell.model.modelId)

                                                    HoverHandler {
                                                        onHoveredChanged: {
                                                            if (!hovered) mainToolTip.text = ""
                                                        }
                                                        onPointChanged: {
                                                            mainToolTip.position = parent.mapToGlobal(point.position)
                                                            mainToolTip.text = qsTr("Default model for new conversations")
                                                        }
                                                    }
                                                }
                                            }

                                            Component {
                                                id: subagentCellComponent

                                                RadioButton {
                                                    anchors.centerIn: parent
                                                    width: implicitWidth; height: implicitHeight
                                                    checked: providerModelCell.model.subagent

                                                    onClicked: agentModule.subagentModelSet(providerCard.providerId, providerModelCell.model.modelId)

                                                    HoverHandler {
                                                        onHoveredChanged: {
                                                            if (!hovered) mainToolTip.text = ""
                                                        }
                                                        onPointChanged: {
                                                            mainToolTip.position = parent.mapToGlobal(point.position)
                                                            mainToolTip.text = qsTr("Default model for subagents")
                                                        }
                                                    }
                                                }
                                            }

                                            Component {
                                                id: visionCellComponent

                                                RadioButton {
                                                    anchors.centerIn: parent
                                                    width: implicitWidth; height: implicitHeight
                                                    checked: providerModelCell.model.vision

                                                    onClicked: agentModule.visionModelSet(providerCard.providerId, providerModelCell.model.modelId)

                                                    HoverHandler {
                                                        onHoveredChanged: {
                                                            if (!hovered) mainToolTip.text = ""
                                                        }
                                                        onPointChanged: {
                                                            mainToolTip.position = parent.mapToGlobal(point.position)
                                                            mainToolTip.text = qsTr("Default model for image understanding")
                                                        }
                                                    }
                                                }
                                            }

                                            Component {
                                                id: actionCellComponent

                                                Button {
                                                    anchors.centerIn: parent
                                                    visible: providerCard.providerCustom
                                                             && providerCard.providerModelEndpoint.length === 0
                                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                                    checkable: true
                                                    flat: true
                                                    icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/delete.svg"
                                                    icon.width: 16; icon.height: 16
                                                    width: 20; height: 20

                                                    onClicked: {
                                                        if (!checked) providerCard.modelRemove(providerModelCell.model.modelId)
                                                    }

                                                    Timer {
                                                        interval: 1000
                                                        running: parent.checked
                                                        onTriggered: parent.checked = false
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            SettingsPage {
                title: qsTr("MCP")
                description: qsTr("Add and manage MCP servers exposed to the agent.")
                actionText: qsTr("Add Server")
                actionIcon: "qrc:/icon/add.svg"

                onTriggerAction: mcpInsertDialog.openInsert()

                Repeater {
                    model: mcpModel

                    delegate: Rectangle {
                        id: serverCard
                        property string serverName: model.display || ""
                        property url serverUrl: model.url
                        property bool serverEnabled: model.enabled
                        property string serverVersion: model.version || ""
                        property string serverDescription: model.description || ""
                        property url serverIcon: model.decoration || "qrc:/icon/mcp.svg"

                        radius: 6
                        color: global.backHover
                        border.color: global.stroke
                        Layout.fillWidth: true
                        Layout.preferredHeight: 168

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12

                            Rectangle {
                                radius: 6
                                color: global.backSelected
                                Layout.preferredWidth: 42
                                Layout.preferredHeight: 42
                                Layout.alignment: Qt.AlignTop

                                IconImage {
                                    anchors.centerIn: parent
                                    width: 24
                                    height: 24
                                    source: serverCard.serverIcon
                                    color: model.decoration ? "transparent" : global.fore
                                }
                            }

                            ColumnLayout {
                                spacing: 4
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                RowLayout {
                                    spacing: 8
                                    Layout.fillWidth: true

                                    Label {
                                        text: serverCard.serverName
                                        font.bold: true
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        visible: serverCard.serverVersion.length > 0
                                        text: serverCard.serverVersion
                                        color: global.stroke
                                    }
                                }

                                Label {
                                    text: serverCard.serverUrl.toString()
                                    color: global.stroke
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: serverCard.serverDescription
                                    color: global.stroke
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Item {
                                    Layout.fillHeight: true
                                }
                            }

                            RowLayout {
                                spacing: 4
                                Layout.alignment: Qt.AlignTop

                                Switch {
                                    checked: serverCard.serverEnabled

                                    onClicked: agentModule.mcpEnabledSet(serverCard.serverUrl, checked)
                                }

                                Button {
                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                    checkable: true
                                    flat: true
                                    icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/delete.svg"
                                    icon.width: 16; icon.height: 16
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    onToggled: {
                                        if (!checked) agentModule.mcpRemove(serverCard.serverUrl)
                                    }

                                    Timer {
                                        interval: 1000
                                        running: parent.checked
                                        onTriggered: parent.checked = false
                                    }
                                }
                            }
                        }
                    }
                }
            }

            SettingsPage {
                title: qsTr("Skills")
                description: qsTr("Choose the skills loaded while preparing a turn.")
            }

            SettingsPage {
                title: qsTr("Hooks")
                description: qsTr("Configure scripts for agent lifecycle events.")

                Repeater {
                    model: hookModel

                    delegate: Rectangle {
                        id: hookCard
                        property int hookEvent: index
                        property string hookTitle: model.display
                        property string hookDescription: model.description
                        property bool hookEnabled: model.enabled
                        property var hookScripts: model.scripts
                        radius: 6
                        color: global.backHover
                        border.color: global.stroke
                        implicitHeight: hookLayout.implicitHeight + 32
                        Layout.fillWidth: true

                        ColumnLayout {
                            id: hookLayout
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 12

                            RowLayout {
                                spacing: 16
                                Layout.fillWidth: true

                                ColumnLayout {
                                    spacing: 4
                                    Layout.fillWidth: true

                                    Label {
                                        text: hookCard.hookTitle
                                        font.bold: true
                                    }

                                    Label {
                                        text: hookCard.hookDescription
                                        color: global.stroke
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }
                                }

                                Switch {
                                    checked: hookCard.hookEnabled

                                    onToggled: agentModule.hookEnabledSet(hookCard.hookEvent, checked)
                                }
                            }

                            Rectangle {
                                color: global.stroke
                                implicitHeight: 1
                                Layout.fillWidth: true
                            }

                            Label {
                                visible: hookCard.hookScripts.length === 0
                                text: qsTr("No scripts configured.")
                                color: global.stroke
                                Layout.fillWidth: true
                            }

                            Repeater {
                                model: hookCard.hookScripts

                                delegate: RowLayout {
                                    id: hookScript
                                    required property var modelData
                                    spacing: 8
                                    Layout.fillWidth: true

                                    IconImage {
                                        source: hookScript.modelData.decoration
                                        sourceSize.width: 18
                                        sourceSize.height: 18
                                    }

                                    ColumnLayout {
                                        spacing: 2
                                        Layout.fillWidth: true

                                        Label {
                                            text: hookScript.modelData.fileName
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Label {
                                            text: hookScript.modelData.documentUrl
                                            color: global.stroke
                                            elide: Text.ElideMiddle
                                            Layout.fillWidth: true
                                        }
                                    }

                                    Button {
                                        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                        checkable: true
                                        flat: true
                                        icon.source: checked ? "qrc:/icon/checkmark.svg" : "qrc:/icon/delete.svg"
                                        icon.width: 16; icon.height: 16
                                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                        onToggled: {
                                            if (!checked) agentModule.hookScriptRemove(hookCard.hookEvent, hookScript.modelData.documentUrl)
                                        }

                                        Timer {
                                            interval: 1000
                                            running: parent.checked
                                            onTriggered: parent.checked = false
                                        }
                                    }
                                }
                            }

                            Button {
                                text: qsTr("Add Script")
                                icon.source: "qrc:/icon/add.svg"
                                icon.width: 16
                                icon.height: 16

                                onClicked: {
                                    hookScriptDialog.event = hookCard.hookEvent
                                    hookScriptDialog.open()
                                }
                            }
                        }
                    }
                }
            }

            SettingsPage {
                title: qsTr("Context")
                description: qsTr("Manage context limits, token budgets, and compaction.")

                Rectangle {
                    radius: 6
                    color: global.backHover
                    border.color: global.stroke
                    implicitHeight: contextLayout.implicitHeight + 32
                    Layout.fillWidth: true

                    ColumnLayout {
                        id: contextLayout
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 16

                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                spacing: 4
                                Layout.fillWidth: true

                                Label {
                                    text: qsTr("Automatic compaction")
                                    font.bold: true
                                }

                                Label {
                                    text: qsTr("Compact earlier turns when context usage reaches this threshold.")
                                    color: global.stroke
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }

                            Label {
                                text: Math.round(compactThresholdSlider.value) + "%"
                                font.pixelSize: 26
                                font.bold: true
                            }
                        }

                        Slider {
                            id: compactThresholdSlider
                            from: 50
                            to: 95
                            stepSize: 5
                            snapMode: Slider.SnapAlways
                            value: agentModule.compactThresholdGet()
                            Layout.fillWidth: true

                            onMoved: agentModule.compactThresholdSet(Math.round(value))
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: "50%"
                                color: global.stroke
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "95%"
                                color: global.stroke
                            }
                        }

                        Rectangle {
                            color: global.stroke
                            implicitHeight: 1
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: 8
                            Layout.fillWidth: true

                            IconImage {
                                source: "qrc:/icon/info.svg"
                                sourceSize.width: 16
                                sourceSize.height: 16
                                color: global.stroke
                                Layout.alignment: Qt.AlignTop
                            }

                            Label {
                                text: qsTr("At %1%, earlier turns are summarized before the next request. Models without a known context limit are not compacted automatically.").arg(Math.round(compactThresholdSlider.value))
                                color: global.stroke
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            SettingsPage {
                title: qsTr("Runtime")
                description: qsTr("Configure execution limits and failure handling.")

                Rectangle {
                    radius: 6
                    color: global.backHover
                    border.color: global.stroke
                    implicitHeight: toolFailureLayout.implicitHeight + 32
                    Layout.fillWidth: true

                    ColumnLayout {
                        id: toolFailureLayout
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 16

                        RowLayout {
                            spacing: 16
                            Layout.fillWidth: true

                            ColumnLayout {
                                spacing: 4
                                Layout.fillWidth: true

                                Label {
                                    text: qsTr("Consecutive tool failure limit")
                                    font.bold: true
                                }

                                Label {
                                    text: qsTr("Stop the current turn when the same tool fails this many times in a row.")
                                    color: global.stroke
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }

                            SpinBox {
                                from: 3
                                to: 10
                                value: agentModule.toolFailureLimitGet()
                                editable: true

                                onValueModified: agentModule.toolFailureLimitSet(value)
                            }
                        }

                        Rectangle {
                            color: global.stroke
                            implicitHeight: 1
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: 8
                            Layout.fillWidth: true

                            IconImage {
                                source: "qrc:/icon/info.svg"
                                sourceSize.width: 16
                                sourceSize.height: 16
                                color: global.stroke
                                Layout.alignment: Qt.AlignTop
                            }

                            Label {
                                text: qsTr("A successful call clears the counter. A failure from another tool starts a new counter.")
                                color: global.stroke
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                Rectangle {
                    radius: 6
                    color: global.backHover
                    border.color: global.stroke
                    implicitHeight: toolPlanLayout.implicitHeight + 32
                    Layout.fillWidth: true

                    ColumnLayout {
                        id: toolPlanLayout
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 16

                        RowLayout {
                            spacing: 16
                            Layout.fillWidth: true

                            ColumnLayout {
                                spacing: 4
                                Layout.fillWidth: true

                                Label {
                                    text: qsTr("Tool plan reminder interval")
                                    font.bold: true
                                }

                                Label {
                                    text: qsTr("Append a plan_update reminder after this many tool calls.")
                                    color: global.stroke
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }

                            SpinBox {
                                from: 5
                                to: 30
                                value: agentModule.toolPlanIntervalGet()
                                editable: true

                                onValueModified: agentModule.toolPlanIntervalSet(value)
                            }
                        }

                        Rectangle {
                            color: global.stroke
                            implicitHeight: 1
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: 8
                            Layout.fillWidth: true

                            IconImage {
                                source: "qrc:/icon/info.svg"
                                sourceSize.width: 16
                                sourceSize.height: 16
                                color: global.stroke
                                Layout.alignment: Qt.AlignTop
                            }

                            Label {
                                text: qsTr("Applies only when plan_update is available. Reminders are not stored in conversation history.")
                                color: global.stroke
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                Rectangle {
                    radius: 6
                    color: global.backHover
                    border.color: global.stroke
                    implicitHeight: toolCallLayout.implicitHeight + 32
                    Layout.fillWidth: true

                    ColumnLayout {
                        id: toolCallLayout
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 16

                        RowLayout {
                            spacing: 16
                            Layout.fillWidth: true

                            ColumnLayout {
                                spacing: 4
                                Layout.fillWidth: true

                                Label {
                                    text: qsTr("Tool call limit")
                                    font.bold: true
                                }

                                Label {
                                    text: qsTr("Stop the current turn after this many tool calls.")
                                    color: global.stroke
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }

                            SpinBox {
                                from: 10
                                to: 100
                                stepSize: 5
                                value: agentModule.toolCallLimitGet()
                                editable: true

                                onValueModified: agentModule.toolCallLimitSet(value)
                            }
                        }

                        Rectangle {
                            color: global.stroke
                            implicitHeight: 1
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: 8
                            Layout.fillWidth: true

                            IconImage {
                                source: "qrc:/icon/info.svg"
                                sourceSize.width: 16
                                sourceSize.height: 16
                                color: global.stroke
                                Layout.alignment: Qt.AlignTop
                            }

                            Label {
                                text: qsTr("Plan updates and user input requests do not count toward this limit.")
                                color: global.stroke
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: providerInsertDialog
        width: 520
        x: (rootItem.width - width) / 2
        y: (rootItem.height - height) / 2
        modal: true
        title: qsTr("Add Provider")
        property bool deepseekAdded: false

        function openInsert(): void {
            deepseekAdded = agentModule.providerExists("deepseek")
            providerNameTextField.clear()
            providerBaseUrlTextField.clear()
            open()
        }

        function submitCustom(): void {
            agentModule.providerInsert("", {
                "name": providerNameTextField.text.trim(),
                "api": providerBaseUrlTextField.text.trim(),
                "chatEndpoint": "/chat/completions",
                "modelsEndpoint": "/models"
            })
            close()
        }

        contentItem: ColumnLayout {
            spacing: 12

            Label {
                text: qsTr("Preset")
                font.bold: true
            }

            Rectangle {
                radius: 4
                color: global.backHover
                border.color: global.stroke
                Layout.fillWidth: true
                Layout.preferredHeight: 56

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    IconImage {
                        source: "qrc:/icon/deepseek.svg"
                        Layout.preferredWidth: 24; Layout.preferredHeight: 24
                    }

                    ColumnLayout {
                        spacing: 0
                        Layout.fillWidth: true

                        Label {
                            text: "DeepSeek"
                            font.bold: true
                        }

                        Label {
                            text: qsTr("Built-in OpenAI-compatible provider")
                            color: global.stroke
                        }
                    }

                    Button {
                        text: providerInsertDialog.deepseekAdded ? qsTr("Added") : qsTr("Add")
                        enabled: !providerInsertDialog.deepseekAdded
                        leftPadding: 12; rightPadding: 12
                        implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding

                        onClicked: {
                            agentModule.providerInsert("deepseek", {})
                            providerInsertDialog.deepseekAdded = true
                        }
                    }
                }
            }

            Rectangle {
                color: global.stroke
                Layout.fillWidth: true
                Layout.preferredHeight: 1
            }

            Label {
                text: qsTr("OpenAI-compatible")
                font.bold: true
            }

            TextField {
                id: providerNameTextField
                placeholderText: qsTr("Provider name")
                Layout.fillWidth: true
            }

            TextField {
                id: providerBaseUrlTextField
                placeholderText: qsTr("Base URL, for example http://localhost:8000/v1")
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Cancel")

                    onClicked: providerInsertDialog.close()
                }

                Button {
                    text: qsTr("Add")
                    highlighted: true
                    enabled: providerNameTextField.text.trim().length > 0
                             && providerBaseUrlTextField.text.trim().length > 0

                    onClicked: providerInsertDialog.submitCustom()
                }
            }
        }
    }

    Dialog {
        id: providerEditDialog
        width: 480
        x: (rootItem.width - width) / 2
        y: (rootItem.height - height) / 2
        modal: true
        property string providerId
        property string field
        property var providerConfig
        property bool allowEmpty: false

        function openEdit(id: string, key: string, label: string, value: string, config: var, empty: bool): void {
            providerId = id
            field = key
            providerConfig = config
            title = qsTr("Edit %1").arg(label)
            allowEmpty = empty
            providerEditTextField.text = value
            open()
        }

        function submit(): void {
            const config = {}
            for (const key in providerConfig) config[key] = providerConfig[key]
            config[field] = providerEditTextField.text.trim()
            agentModule.providerEdit(providerId, config)
            close()
        }

        onOpened: {
            providerEditTextField.forceActiveFocus()
            providerEditTextField.selectAll()
        }

        contentItem: ColumnLayout {
            spacing: 12

            TextField {
                id: providerEditTextField
                Layout.fillWidth: true

                onAccepted: {
                    if (providerEditDialog.allowEmpty || text.trim().length > 0) providerEditDialog.submit()
                }
            }

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Cancel")

                    onClicked: providerEditDialog.close()
                }

                Button {
                    text: qsTr("Save")
                    highlighted: true
                    enabled: providerEditDialog.allowEmpty || providerEditTextField.text.trim().length > 0

                    onClicked: providerEditDialog.submit()
                }
            }
        }
    }

    Dialog {
        id: providerModelInsertDialog
        width: 480
        x: (rootItem.width - width) / 2
        y: (rootItem.height - height) / 2
        modal: true
        title: qsTr("Add Model")
        property string providerId
        property var providerConfig

        function openInsert(id: string, config: var): void {
            providerId = id
            providerConfig = config
            providerModelIdTextField.clear()
            providerModelNameTextField.clear()
            providerModelContextTextField.clear()
            providerModelOutputTextField.clear()
            open()
        }

        function submit(): void {
            const models = {}
            const currentModels = providerConfig.models || {}
            for (const id in currentModels) models[id] = currentModels[id]
            const modelId = providerModelIdTextField.text.trim()
            models[modelId] = {
                "name": providerModelNameTextField.text.trim(),
                "limit": {
                    "context": Number(providerModelContextTextField.text),
                    "output": Number(providerModelOutputTextField.text)
                }
            }
            providerConfig.models = models
            agentModule.providerEdit(providerId, providerConfig)
            close()
        }

        onOpened: providerModelIdTextField.forceActiveFocus()

        contentItem: ColumnLayout {
            spacing: 12

            TextField {
                id: providerModelIdTextField
                placeholderText: qsTr("Model ID")
                Layout.fillWidth: true
            }

            TextField {
                id: providerModelNameTextField
                placeholderText: qsTr("Display name")
                Layout.fillWidth: true
            }

            TextField {
                id: providerModelContextTextField
                placeholderText: qsTr("Context window")
                validator: IntValidator { bottom: 1 }
                Layout.fillWidth: true
            }

            TextField {
                id: providerModelOutputTextField
                placeholderText: qsTr("Maximum output tokens")
                validator: IntValidator { bottom: 1 }
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Cancel")

                    onClicked: providerModelInsertDialog.close()
                }

                Button {
                    text: qsTr("Add")
                    highlighted: true
                    enabled: providerModelIdTextField.text.trim().length > 0
                             && providerModelNameTextField.text.trim().length > 0
                             && providerModelContextTextField.acceptableInput
                             && providerModelOutputTextField.acceptableInput

                    onClicked: providerModelInsertDialog.submit()
                }
            }
        }
    }

    Dialog {
        id: mcpInsertDialog
        width: 480
        x: (rootItem.width - width) / 2
        y: (rootItem.height - height) / 2
        modal: true
        title: qsTr("Add MCP Server")

        function openInsert(): void {
            urlTextField.clear()
            errorLabel.text = ""
            open()
        }

        function submit(): void {
            const error = agentModule.mcpInsert(urlTextField.text.trim())
            errorLabel.text = error
            if (error.length === 0) close()
        }

        onOpened: urlTextField.forceActiveFocus()

        contentItem: ColumnLayout {
            spacing: 12

            TextField {
                id: urlTextField
                placeholderText: qsTr("https://example.com/mcp")
                Layout.fillWidth: true

                onAccepted: mcpInsertDialog.submit()
            }

            Label {
                id: errorLabel
                visible: text.length > 0
                color: global.dangerFore3
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            RowLayout {
                spacing: 8
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("Cancel")

                    onClicked: mcpInsertDialog.close()
                }

                Button {
                    text: qsTr("Add")
                    highlighted: true
                    enabled: urlTextField.text.trim().length > 0

                    onClicked: mcpInsertDialog.submit()
                }
            }
        }
    }

    component SettingsPage: Item {
        id: settingsPage
        default property alias content: pageContent.data
        property string title
        property string description
        property string actionText
        property url actionIcon

        signal triggerAction()

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 8

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: settingsPage.title
                    font.pixelSize: 22
                    font.bold: true
                    Layout.fillWidth: true
                }

                Button {
                    visible: settingsPage.actionText.length > 0
                    text: settingsPage.actionText
                    icon.source: settingsPage.actionIcon
                    icon.width: 16
                    icon.height: 16

                    onClicked: settingsPage.triggerAction()
                }
            }

            Label {
                text: settingsPage.description
                color: global.stroke
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            ScrollView {
                id: settingsPageScrollView
                clip: true
                contentWidth: availableWidth
                rightPadding: settingsPageScrollBar.width
                Layout.fillWidth: true
                Layout.fillHeight: true

                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: ScrollBar {
                    id: settingsPageScrollBar
                    x: settingsPageScrollView.width - width
                    y: settingsPageScrollView.topPadding
                    height: settingsPageScrollView.availableHeight
                    policy: ScrollBar.AsNeeded
                    palette {
                        mid: global.stroke
                        dark: global.strokePressed
                    }
                }

                ColumnLayout {
                    id: pageContent
                    width: settingsPageScrollView.availableWidth
                }
            }
        }
    }
}
