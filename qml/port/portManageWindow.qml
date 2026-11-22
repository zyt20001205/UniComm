import QtMultimedia
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts
import QtQuick.Shapes
import UniComm
import "../util"

Item {
    id: root
    required property PortManageWindow portManageWindow
    required property var roiModel
    required property var pipelineModel
    property alias pageIndex: swipeView.currentIndex
    readonly property int portType: root.portManageWindow.config.portType ?? PortType.SerialPort

    Rectangle {
        anchors.fill: parent
        color: global.theme.general.back
    }

    ColumnLayout {
        anchors.fill: parent

        SwipeView {
            id: swipeView
            clip: true
            currentIndex: 0
            interactive: false
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.margins: 20

            // port type selection
            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true

                Tumbler {
                    id: tumbler
                    currentIndex: root.portType
                    delegate: delegateComponent
                    model: [qsTr("Serial Port"), qsTr("Vision"), qsTr("Tcp Client"), qsTr("Tcp Server"), qsTr("Ssl Client"), qsTr("Ssl Server"), qsTr("WebSocket Client"), qsTr("WebSocket Server"), qsTr("Udp Socket"), qsTr("Bluetooth LE")]
                    wrap: false
                    Layout.fillWidth: true; Layout.fillHeight: true

                    onMovingChanged: {
                        if (!moving) {
                            root.portManageWindow.typeSelect(currentIndex)
                        }
                    }
                }

                Component {
                    id: delegateComponent

                    Label {
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
                        text: modelData
                        font.pointSize: 16
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: false

                    IconImage {
                        visible: source !== ""
                        source: sourceGet()
                        sourceSize: Qt.size(80, 80)
                        color: global.theme.general.fore
                        Layout.preferredWidth: 80; Layout.preferredHeight: 80
                        Layout.alignment: Qt.AlignHCenter

                        function sourceGet() {
                            switch (root.portType) {
                                case PortType.SerialPort:
                                    return "qrc:/icon/serialPort.svg"
                                case PortType.Vision:
                                    return "qrc:/icon/video.svg"
                                case PortType.TcpClient:
                                    return "qrc:/icon/tcpClient.svg"
                                case PortType.TcpServer:
                                    return "qrc:/icon/tcpServer.svg"
                                case PortType.SslClient:
                                    return ""
                                case PortType.SslServer:
                                    return ""
                                case PortType.WebSocketClient:
                                    return ""
                                case PortType.WebSocketServer:
                                    return ""
                                case PortType.UdpSocket:
                                    return "qrc:/icon/udpSocket.svg"
                                case PortType.BluetoothLe:
                                    return ""
                                default:
                                    return ""
                            }
                        }
                    }

                    Label {
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        text: textGet()
                        wrapMode: Text.WordWrap
                        font.pointSize: 12
                        Layout.alignment: Qt.AlignHCenter; Layout.fillWidth: true

                        function textGet() {
                            switch (root.portType) {
                                case PortType.SerialPort:
                                    return qsTr("A serial communication interface through which information transfers in or out sequentially one bit at a time.")
                                case PortType.Vision:
                                    return qsTr("A vision input that captures frames from a screen or camera for image processing and recognition.")
                                case PortType.TcpClient:
                                    return qsTr("A device that initiates a connection with a TCP server to send and receive reliable, ordered data over a network.")
                                case PortType.TcpServer:
                                    return qsTr("A device that listens on a network port, accepts incoming connections from TCP clients, and manages reliable, ordered data exchange.")
                                case PortType.SslClient:
                                    return qsTr("A secure client that establishes encrypted connections with SSL/TLS servers to ensure data confidentiality and integrity during transmission.")
                                case PortType.SslServer:
                                    return qsTr("A secure server that accepts encrypted SSL/TLS byte-stream connections from multiple clients.")
                                case PortType.WebSocketClient:
                                    return qsTr("A client that establishes a full-duplex WebSocket connection and exchanges text or binary messages.")
                                case PortType.WebSocketServer:
                                    return qsTr("A server that accepts multiple WebSocket clients over ws or wss and manages each peer independently.")
                                case PortType.UdpSocket:
                                    return qsTr("A device that uses the User Datagram Protocol to send independent, connectionless messages (datagrams) over an IP network.")
                                case PortType.BluetoothLe:
                                    return qsTr("A Bluetooth Low Energy central that exchanges data through GATT characteristics.")
                                default:
                                    return ""
                            }
                        }
                    }
                }
            }

            // port setting
            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true

                ScrollView {
                    id: portSettingsScrollView
                    clip: true
                    contentWidth: availableWidth
                    contentHeight: portSettingsLayout.height
                    rightPadding: portSettingsScrollBar.width
                    Layout.fillWidth: true; Layout.fillHeight: true

                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical: ScrollBar {
                        id: portSettingsScrollBar
                        minimumSize: Math.min(1, 20 / Math.max(1, height))
                        x: portSettingsScrollView.width - width
                        y: portSettingsScrollView.topPadding
                        height: portSettingsScrollView.availableHeight
                        policy: ScrollBar.AsNeeded
                        palette {
                            mid: global.theme.general.stroke
                            dark: global.theme.general.strokePressed
                        }
                    }

                    StackLayout {
                        id: portSettingsLayout
                        currentIndex: root.portType
                        width: portSettingsScrollView.availableWidth
                        height: implicitHeight
                        implicitHeight: children[currentIndex] ? children[currentIndex].implicitHeight : 0

                    // serial port
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("Serial Port")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Device Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: serialPortDeviceNameComboBox
                            currentValue: root.portManageWindow.config.deviceName
                            font.pointSize: 12
                            model: root.portManageWindow.serialPorts
                            textRole: "display"
                            valueRole: "deviceName"
                            Layout.fillWidth: true

                            onActivated: {
                                const device = root.portManageWindow.serialPorts[currentIndex]
                                root.portManageWindow.config.deviceName = device.deviceName
                                root.portManageWindow.config.portName = device.name || device.deviceName
                            }
                        }

                        Label {
                            text: qsTr("Baud Rate")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            value: root.portManageWindow.config.baudRate ?? 115200
                            onValueModified: root.portManageWindow.config.baudRate = value
                            font.pointSize: 12
                            editable: true
                            from: 1
                            to: 5000000
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Databits")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            currentValue: root.portManageWindow.config.dataBits ?? 8
                            onActivated: root.portManageWindow.config.dataBits = currentValue
                            font.pointSize: 12
                            model: [5, 6, 7, 8]
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Parity")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            currentValue: root.portManageWindow.config.parity ?? 0
                            onActivated: root.portManageWindow.config.parity = currentValue
                            font.pointSize: 12
                            model: [
                                { text: qsTr("No"), value: 0 },
                                { text: qsTr("Even"), value: 2 },
                                { text: qsTr("Odd"), value: 3 },
                                { text: qsTr("Space"), value: 4 },
                                { text: qsTr("Mark"), value: 5 }
                            ]
                            textRole: "text"
                            valueRole: "value"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Stop Bits")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            currentValue: root.portManageWindow.config.stopBits ?? 1
                            onActivated: root.portManageWindow.config.stopBits = currentValue
                            font.pointSize: 12
                            model: [
                                { text: "1", value: 1 },
                                { text: "1.5", value: 3 },
                                { text: "2", value: 2 }
                            ]
                            textRole: "text"
                            valueRole: "value"
                            Layout.fillWidth: true
                        }
                    }

                    // vision
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("Vision")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Device Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: visionDeviceNameComboBox
                            currentValue: root.portManageWindow.config.deviceName
                            font.pointSize: 12
                            model: root.portManageWindow.visionSources
                            Layout.fillWidth: true

                            onActivated: {
                                root.portManageWindow.config.deviceName = currentValue
                                root.portManageWindow.config.portName = currentValue
                            }
                        }
                    }

                    // tcp client
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("Tcp Client")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Remote Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.remoteHost ?? "" }

                            onValueCommitted: root.portManageWindow.config.remoteHost = text
                        }

                        Label {
                            text: qsTr("Remote Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            value: root.portManageWindow.config.remotePort ?? 0
                            onValueModified: root.portManageWindow.config.remotePort = value
                            font.pointSize: 12
                            editable: true
                            from: 0
                            to: 65535
                            Layout.fillWidth: true
                        }
                    }

                    // tcp server
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("Tcp Server")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Local Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: tcpServerLocalHost
                            editable: true
                            model: root.portManageWindow.localHosts
                            currentValue: root.portManageWindow.config.localHost
                            editText: root.portManageWindow.config.localHost ?? ""
                            font.pointSize: 12
                            Layout.fillWidth: true

                            onActivated: root.portManageWindow.config.localHost = currentText

                            Connections {
                                target: tcpServerLocalHost.contentItem
                                function onEditingFinished() { root.portManageWindow.config.localHost = tcpServerLocalHost.contentItem.text }
                            }
                        }

                        Label {
                            text: qsTr("Local Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            value: root.portManageWindow.config.localPort ?? 0
                            onValueModified: root.portManageWindow.config.localPort = value
                            font.pointSize: 12
                            editable: true
                            from: 0
                            to: 65535
                            Layout.fillWidth: true
                        }
                    }

                    // ssl client
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("Ssl Client")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Remote Host")
                            font.pointSize: 12
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.remoteHost ?? "" }

                            onValueCommitted: root.portManageWindow.config.remoteHost = text
                        }

                        Label {
                            text: qsTr("Remote Port")
                            font.pointSize: 12
                        }

                        SpinBox {
                            value: root.portManageWindow.config.remotePort ?? 0
                            onValueModified: root.portManageWindow.config.remotePort = value
                            font.pointSize: 12
                            editable: true
                            from: 0
                            to: 65535
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            color: global.theme.general.stroke
                            implicitHeight: 1
                            Layout.columnSpan: 2
                            Layout.fillWidth: true
                            Layout.topMargin: 8
                            Layout.bottomMargin: 8
                        }

                        Label {
                            text: qsTr("Secure")
                            font.pointSize: 12
                        }

                        Switch {
                            id: sslClientSecureSwitch
                            checked: root.portManageWindow.config.secure ?? false
                            onToggled: root.portManageWindow.config.secure = checked
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        }

                        Label {
                            visible: sslClientSecureSwitch.checked
                            text: qsTr("CA Certificate")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: sslClientSecureSwitch.checked
                            clearable: true
                            browsable: true
                            title: qsTr("Select CA Certificate")
                            nameFilters: [qsTr("PEM certificates (*.pem *.crt *.cer)"), qsTr("All files (*)")]
                            placeholderText: qsTr("Default trust store")
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.caCertificate ?? "" }

                            onValueCommitted: root.portManageWindow.config.caCertificate = text
                        }

                        Label {
                            visible: sslClientSecureSwitch.checked
                            text: qsTr("Client Certificate")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: sslClientSecureSwitch.checked
                            clearable: true
                            browsable: true
                            title: qsTr("Select Client Certificate")
                            nameFilters: [qsTr("PEM certificates (*.pem *.crt *.cer)"), qsTr("All files (*)")]
                            placeholderText: qsTr("PEM certificate URL")
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.certificate ?? "" }

                            onValueCommitted: root.portManageWindow.config.certificate = text
                        }

                        Label {
                            visible: sslClientSecureSwitch.checked
                            text: qsTr("Private Key")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: sslClientSecureSwitch.checked
                            clearable: true
                            browsable: true
                            title: qsTr("Select Private Key")
                            nameFilters: [qsTr("PEM private keys (*.pem *.key)"), qsTr("All files (*)")]
                            placeholderText: qsTr("PEM private key URL")
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.privateKey ?? "" }

                            onValueCommitted: root.portManageWindow.config.privateKey = text
                        }

                        Label {
                            visible: sslClientSecureSwitch.checked
                            text: qsTr("Private Key Password")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: sslClientSecureSwitch.checked
                            revealable: true
                            clearable: true
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.privateKeyPassword ?? "" }

                            onValueCommitted: root.portManageWindow.config.privateKeyPassword = text
                        }
                    }

                    // ssl server
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("Ssl Server")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Local Host")
                            font.pointSize: 12
                        }

                        ComboBox {
                            id: sslServerLocalHost
                            editable: true
                            model: root.portManageWindow.localHosts
                            currentValue: root.portManageWindow.config.localHost
                            editText: root.portManageWindow.config.localHost ?? ""
                            font.pointSize: 12
                            Layout.fillWidth: true

                            onActivated: root.portManageWindow.config.localHost = currentText

                            Connections {
                                target: sslServerLocalHost.contentItem
                                function onEditingFinished() { root.portManageWindow.config.localHost = sslServerLocalHost.contentItem.text }
                            }
                        }

                        Label {
                            text: qsTr("Local Port")
                            font.pointSize: 12
                        }

                        SpinBox {
                            value: root.portManageWindow.config.localPort ?? 0
                            onValueModified: root.portManageWindow.config.localPort = value
                            font.pointSize: 12
                            editable: true
                            from: 1
                            to: 65535
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            color: global.theme.general.stroke
                            implicitHeight: 1
                            Layout.columnSpan: 2
                            Layout.fillWidth: true
                            Layout.topMargin: 8
                            Layout.bottomMargin: 8
                        }

                        Label {
                            text: qsTr("Server Certificate")
                            font.pointSize: 12
                        }

                        LineEdit {
                            clearable: true
                            browsable: true
                            title: qsTr("Select Server Certificate")
                            nameFilters: [qsTr("PEM certificates (*.pem *.crt *.cer)"), qsTr("All files (*)")]
                            font.pointSize: 12
                            placeholderText: qsTr("PEM certificate URL")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.certificate ?? "" }

                            onValueCommitted: root.portManageWindow.config.certificate = text
                        }

                        Label {
                            text: qsTr("Private Key")
                            font.pointSize: 12
                        }

                        LineEdit {
                            clearable: true
                            browsable: true
                            title: qsTr("Select Private Key")
                            nameFilters: [qsTr("PEM private keys (*.pem *.key)"), qsTr("All files (*)")]
                            font.pointSize: 12
                            placeholderText: qsTr("PEM private key URL")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.privateKey ?? "" }

                            onValueCommitted: root.portManageWindow.config.privateKey = text
                        }

                        Label {
                            text: qsTr("Private Key Password")
                            font.pointSize: 12
                        }

                        LineEdit {
                            revealable: true
                            clearable: true
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.privateKeyPassword ?? "" }

                            onValueCommitted: root.portManageWindow.config.privateKeyPassword = text
                        }

                        Label {
                            text: qsTr("CA Certificate")
                            font.pointSize: 12
                        }

                        LineEdit {
                            clearable: true
                            browsable: true
                            title: qsTr("Select CA Certificate")
                            nameFilters: [qsTr("PEM certificates (*.pem *.crt *.cer)"), qsTr("All files (*)")]
                            placeholderText: qsTr("No client certificate required")
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.caCertificate ?? "" }

                            onValueCommitted: root.portManageWindow.config.caCertificate = text
                        }
                    }

                    // web socket client
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("WebSocket Client")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Remote Host")
                            font.pointSize: 12
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: "ws.postman-echo.com/raw"
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.remoteHost ?? "" }

                            onValueCommitted: root.portManageWindow.config.remoteHost = text
                        }

                        Label {
                            text: qsTr("Remote Port")
                            font.pointSize: 12
                        }

                        SpinBox {
                            value: root.portManageWindow.config.remotePort ?? 0
                            onValueModified: root.portManageWindow.config.remotePort = value
                            font.pointSize: 12
                            editable: true
                            from: 1
                            to: 65535
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            color: global.theme.general.stroke
                            implicitHeight: 1
                            Layout.columnSpan: 2
                            Layout.fillWidth: true
                            Layout.topMargin: 8
                            Layout.bottomMargin: 8
                        }

                        Label {
                            text: qsTr("Secure")
                            font.pointSize: 12
                        }

                        Switch {
                            id: webSocketClientSecureSwitch
                            checked: root.portManageWindow.config.secure ?? false
                            onToggled: {
                                root.portManageWindow.config.secure = checked
                                root.portManageWindow.config.remotePort = checked ? 443 : 80
                            }
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        }

                        Label {
                            visible: webSocketClientSecureSwitch.checked
                            text: qsTr("CA Certificate")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: webSocketClientSecureSwitch.checked
                            clearable: true
                            browsable: true
                            title: qsTr("Select CA Certificate")
                            nameFilters: [qsTr("PEM certificates (*.pem *.crt *.cer)"), qsTr("All files (*)")]
                            placeholderText: qsTr("Default trust store")
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.caCertificate ?? "" }

                            onValueCommitted: root.portManageWindow.config.caCertificate = text
                        }

                        Label {
                            visible: webSocketClientSecureSwitch.checked
                            text: qsTr("Client Certificate")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: webSocketClientSecureSwitch.checked
                            clearable: true
                            browsable: true
                            title: qsTr("Select Client Certificate")
                            nameFilters: [qsTr("PEM certificates (*.pem *.crt *.cer)"), qsTr("All files (*)")]
                            placeholderText: qsTr("PEM certificate URL")
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.certificate ?? "" }

                            onValueCommitted: root.portManageWindow.config.certificate = text
                        }

                        Label {
                            visible: webSocketClientSecureSwitch.checked
                            text: qsTr("Private Key")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: webSocketClientSecureSwitch.checked
                            clearable: true
                            browsable: true
                            title: qsTr("Select Private Key")
                            nameFilters: [qsTr("PEM private keys (*.pem *.key)"), qsTr("All files (*)")]
                            placeholderText: qsTr("PEM private key URL")
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.privateKey ?? "" }

                            onValueCommitted: root.portManageWindow.config.privateKey = text
                        }

                        Label {
                            visible: webSocketClientSecureSwitch.checked
                            text: qsTr("Private Key Password")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: webSocketClientSecureSwitch.checked
                            revealable: true
                            clearable: true
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.privateKeyPassword ?? "" }

                            onValueCommitted: root.portManageWindow.config.privateKeyPassword = text
                        }
                    }

                    // web socket server
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("WebSocket Server")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Local Host")
                            font.pointSize: 12
                        }

                        ComboBox {
                            id: webSocketServerLocalHost
                            editable: true
                            model: root.portManageWindow.localHosts
                            currentValue: root.portManageWindow.config.localHost
                            editText: root.portManageWindow.config.localHost ?? ""
                            font.pointSize: 12
                            Layout.fillWidth: true

                            onActivated: root.portManageWindow.config.localHost = currentText

                            Connections {
                                target: webSocketServerLocalHost.contentItem
                                function onEditingFinished() { root.portManageWindow.config.localHost = webSocketServerLocalHost.contentItem.text }
                            }
                        }

                        Label {
                            text: qsTr("Local Port")
                            font.pointSize: 12
                        }

                        SpinBox {
                            value: root.portManageWindow.config.localPort ?? 0
                            onValueModified: root.portManageWindow.config.localPort = value
                            font.pointSize: 12
                            editable: true
                            from: 1
                            to: 65535
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            color: global.theme.general.stroke
                            implicitHeight: 1
                            Layout.columnSpan: 2
                            Layout.fillWidth: true
                            Layout.topMargin: 8
                            Layout.bottomMargin: 8
                        }

                        Label {
                            text: qsTr("Secure")
                            font.pointSize: 12
                        }

                        Switch {
                            id: webSocketServerSecureSwitch
                            checked: root.portManageWindow.config.secure ?? false
                            onToggled: {
                                root.portManageWindow.config.secure = checked
                                root.portManageWindow.config.localPort = checked ? 443 : 80
                            }
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        }

                        Label {
                            visible: webSocketServerSecureSwitch.checked
                            text: qsTr("Server Certificate")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: webSocketServerSecureSwitch.checked
                            clearable: true
                            browsable: true
                            title: qsTr("Select Server Certificate")
                            nameFilters: [qsTr("PEM certificates (*.pem *.crt *.cer)"), qsTr("All files (*)")]
                            font.pointSize: 12
                            placeholderText: qsTr("PEM certificate URL")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.certificate ?? "" }

                            onValueCommitted: root.portManageWindow.config.certificate = text
                        }

                        Label {
                            visible: webSocketServerSecureSwitch.checked
                            text: qsTr("Private Key")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: webSocketServerSecureSwitch.checked
                            clearable: true
                            browsable: true
                            title: qsTr("Select Private Key")
                            nameFilters: [qsTr("PEM private keys (*.pem *.key)"), qsTr("All files (*)")]
                            font.pointSize: 12
                            placeholderText: qsTr("PEM private key URL")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.privateKey ?? "" }

                            onValueCommitted: root.portManageWindow.config.privateKey = text
                        }

                        Label {
                            visible: webSocketServerSecureSwitch.checked
                            text: qsTr("Private Key Password")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: webSocketServerSecureSwitch.checked
                            revealable: true
                            clearable: true
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.privateKeyPassword ?? "" }

                            onValueCommitted: root.portManageWindow.config.privateKeyPassword = text
                        }

                        Label {
                            visible: webSocketServerSecureSwitch.checked
                            text: qsTr("CA Certificate")
                            font.pointSize: 12
                        }

                        LineEdit {
                            visible: webSocketServerSecureSwitch.checked
                            clearable: true
                            browsable: true
                            title: qsTr("Select CA Certificate")
                            nameFilters: [qsTr("PEM certificates (*.pem *.crt *.cer)"), qsTr("All files (*)")]
                            placeholderText: qsTr("No client certificate required")
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.caCertificate ?? "" }

                            onValueCommitted: root.portManageWindow.config.caCertificate = text
                        }
                    }

                    // udp socket
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("Udp Socket")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Local Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: udpSocketLocalHostComboBox
                            currentValue: root.portManageWindow.config.localHost
                            onActivated: root.portManageWindow.config.localHost = currentValue
                            font.pointSize: 12
                            model: root.portManageWindow.localHosts
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Local Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            value: root.portManageWindow.config.localPort ?? 0
                            onValueModified: root.portManageWindow.config.localPort = value
                            font.pointSize: 12
                            editable: true
                            from: 0
                            to: 65535
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Remote Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.remoteHost ?? "" }

                            onValueCommitted: root.portManageWindow.config.remoteHost = text
                        }

                        Label {
                            text: qsTr("Remote Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            value: root.portManageWindow.config.remotePort ?? 0
                            onValueModified: root.portManageWindow.config.remotePort = value
                            font.pointSize: 12
                            editable: true
                            from: 0
                            to: 65535
                            Layout.fillWidth: true
                        }
                    }

                    // bluetooth le
                    GridLayout {
                        columns: 2
                        columnSpacing: 16; rowSpacing: 10

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        LineEdit {
                            clearable: true
                            font.pointSize: 12
                            placeholderText: qsTr("Bluetooth LE")
                            Layout.fillWidth: true
                            Binding on text { value: root.portManageWindow.config.portName ?? "" }

                            onValueCommitted: root.portManageWindow.config.portName = text
                        }

                        Label {
                            text: qsTr("Address")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            ComboBox {
                                id: bluetoothPeripheralComboBox
                                currentValue: root.portManageWindow.config.peripheralAddress
                                font.pointSize: 12
                                model: root.portManageWindow.bluetoothPeripherals
                                textRole: "display"
                                valueRole: "address"
                                Layout.fillWidth: true

                                onActivated: {
                                    const device = root.portManageWindow.bluetoothPeripherals[currentIndex]
                                    root.portManageWindow.config.peripheralAddress = device.address
                                    root.portManageWindow.config.adapterAddress = device.adapterAddress
                                    root.portManageWindow.config.portName = device.name || device.address
                                }
                            }

                            Button {
                                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                enabled: bluetoothPeripheralComboBox.currentIndex >= 0
                                    && bluetoothPeripheralComboBox.currentIndex < root.portManageWindow.bluetoothPeripherals.length
                                    && root.portManageWindow.bluetoothPeripherals[bluetoothPeripheralComboBox.currentIndex].peripheral !== undefined
                                icon.source: "qrc:/icon/link.svg"
                                icon.width: 24; icon.height: 24
                                Layout.preferredWidth: 32; Layout.preferredHeight: 32
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Discover Services")

                                onClicked: root.portManageWindow.bleConnect(root.portManageWindow.bluetoothPeripherals[bluetoothPeripheralComboBox.currentIndex].peripheral)
                            }
                        }

                        TreeView {
                            id: bluetoothTreeView
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            property int selectedRow: -1
                            Layout.columnSpan: 2
                            Layout.fillWidth: true; Layout.fillHeight: true
                            Layout.preferredHeight: 180

                            model: root.portManageWindow.bluetoothServiceAddress === bluetoothPeripheralComboBox.currentValue
                                ? root.portManageWindow.bluetoothServices : null
                            columnWidthProvider: function(column) {
                                return Math.max(0, bluetoothTreeView.width - bluetoothTreeScrollBar.width)
                            }
                            onWidthChanged: forceLayout()

                            delegate: Item {
                                id: bluetoothTreeDelegate
                                required property TreeView treeView
                                required property bool isTreeNode
                                required property bool expanded
                                required property bool hasChildren
                                required property int depth
                                required property int row
                                required property string uuid
                                required property string shortUuid
                                required property string name
                                required property var capabilities
                                implicitWidth: bluetoothTreeView.width
                                implicitHeight: Math.max(24, bluetoothTreeRow.implicitHeight)

                                onImplicitHeightChanged: Qt.callLater(bluetoothTreeView.forceLayout)

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 6
                                    color: global.theme.general.backHover
                                    visible: bluetoothTreeHoverHandler.hovered
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 6
                                    color: global.theme.general.backSelected
                                    visible: bluetoothTreeView.selectedRow === bluetoothTreeDelegate.row
                                }

                                RowLayout {
                                    id: bluetoothTreeRow
                                    anchors.left: parent.left; anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 0

                                    Item {
                                        Layout.preferredWidth: bluetoothTreeDelegate.depth * 24
                                        Layout.preferredHeight: 24
                                    }

                                    Item {
                                        Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                        IconImage {
                                            anchors.centerIn: parent
                                            width: 16; height: 16
                                            source: bluetoothTreeDelegate.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                                            color: global.theme.general.fore
                                            visible: bluetoothTreeDelegate.isTreeNode && bluetoothTreeDelegate.hasChildren
                                        }

                                        TapHandler {
                                            acceptedButtons: Qt.LeftButton
                                            gesturePolicy: TapHandler.DragWithinBounds

                                            onTapped: {
                                                bluetoothTreeView.selectedRow = bluetoothTreeDelegate.row
                                                if (bluetoothTreeDelegate.isTreeNode && bluetoothTreeDelegate.hasChildren) {
                                                    bluetoothTreeView.toggleExpanded(bluetoothTreeDelegate.row)
                                                }
                                            }
                                        }
                                    }

                                    RowLayout {
                                        id: bluetoothTreeContent
                                        spacing: 6
                                        Layout.minimumWidth: 0
                                        Layout.fillWidth: true

                                        Label {
                                            text: bluetoothTreeDelegate.name.length > 0
                                                ? bluetoothTreeDelegate.name + " \u00b7 " + bluetoothTreeDelegate.shortUuid
                                                : bluetoothTreeDelegate.shortUuid
                                            textFormat: Text.PlainText
                                            font.pointSize: 12
                                            font.weight: bluetoothTreeDelegate.depth === 0 ? Font.DemiBold : Font.Normal
                                            color: global.theme.general.fore
                                            wrapMode: Text.NoWrap
                                            verticalAlignment: Text.AlignVCenter
                                            Layout.minimumWidth: implicitWidth
                                            Layout.minimumHeight: 24
                                        }

                                        RowLayout {
                                            visible: bluetoothTreeDelegate.capabilities.length > 0
                                            spacing: 6
                                            Layout.minimumWidth: implicitWidth

                                            Repeater {
                                                model: bluetoothTreeDelegate.capabilities

                                                Control {
                                                    id: capabilityBadge
                                                    required property string modelData
                                                    leftPadding: 6; rightPadding: 6; topPadding: 2; bottomPadding: 2
                                                    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
                                                    implicitHeight: Math.max(24, contentItem.implicitHeight + topPadding + bottomPadding)

                                                    contentItem: Label {
                                                        text: {
                                                            switch (capabilityBadge.modelData) {
                                                                case "read": return qsTr("Read")
                                                                case "write_request": return qsTr("Write request")
                                                                case "write_command": return qsTr("Write command")
                                                                case "notify": return qsTr("Notify")
                                                                case "indicate": return qsTr("Indicate")
                                                                default: return capabilityBadge.modelData
                                                            }
                                                        }
                                                        textFormat: Text.PlainText
                                                        font.pointSize: 10
                                                        font.weight: Font.DemiBold
                                                        color: global.theme.general.fore
                                                        horizontalAlignment: Text.AlignHCenter
                                                        verticalAlignment: Text.AlignVCenter
                                                    }

                                                    background: Rectangle {
                                                        radius: 4 // Fluent borderRadiusMedium.
                                                        color: global.theme.general.backSelected
                                                        border.width: 1
                                                        border.color: global.theme.general.stroke
                                                    }
                                                }
                                            }
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }
                                    }
                                }

                                HoverHandler {
                                    id: bluetoothTreeHoverHandler

                                    onHoveredChanged: {
                                        if (!hovered) {
                                            mainTooltip.text = ""
                                        }
                                    }
                                    onPointChanged: {
                                        mainTooltip.position = parent.mapToGlobal(point.position)
                                        mainTooltip.text = bluetoothTreeDelegate.uuid
                                    }
                                }

                                TapHandler {
                                    acceptedButtons: Qt.LeftButton
                                    gesturePolicy: TapHandler.DragWithinBounds

                                    onTapped: bluetoothTreeView.selectedRow = bluetoothTreeDelegate.row
                                }
                            }

                            ScrollBar.vertical: ScrollBar {
                                id: bluetoothTreeScrollBar
                                palette {
                                    mid: global.theme.general.stroke
                                    dark: global.theme.general.strokePressed
                                }
                            }

                            Connections {
                                target: portManageWindow

                                function onBluetoothServicesChanged() {
                                    bluetoothTreeView.selectedRow = -1
                                    if (bluetoothTreeView.model) Qt.callLater(bluetoothTreeView.expandRecursively)
                                }
                            }
                        }
                    }
                }

                }

                Label {
                    id: portNameValidator
                    Layout.fillWidth: true; Layout.fillHeight: false
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    color: "#c50f1f"
                    text: ""
                    font.pointSize: 16
                }

                Timer {
                    id: portNameValidatorTimer
                    interval: 3000
                    onTriggered: portNameValidator.text = ""
                }
            }

            // port format
            StackLayout {
                currentIndex: root.portType === PortType.Vision ? 1 : 0
                Layout.fillWidth: true; Layout.fillHeight: true

                // format
                GridLayout {
                    columns: 2
                    columnSpacing: 20; rowSpacing: 20
                    Layout.fillWidth: true; Layout.fillHeight: true

                    Label {
                        text: qsTr("Log Format")
                        font.pointSize: 12
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        currentValue: root.portManageWindow.config.logFormat
                        onActivated: root.portManageWindow.config.logFormat = currentValue
                        model: [
                            { text: qsTr("raw"), value: "raw" },
                            { text: qsTr("hex"), value: "hex" },
                            { text: qsTr("ascii"), value: "ascii" },
                            { text: qsTr("utf-8"), value: "utf-8" }
                        ]
                        textRole: "text"
                        valueRole: "value"
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("Tx Suffix")
                        font.pointSize: 12
                        Layout.fillWidth: true
                    }

                    ComboBox {
                        currentValue: root.portManageWindow.config.txSuffix
                        onActivated: root.portManageWindow.config.txSuffix = currentValue
                        model: [
                            { text: qsTr("null"), value: "null" },
                            { text: qsTr("crlf"), value: "crlf" },
                            { text: qsTr("modbus crc"), value: "modbus crc" },
                            { text: qsTr("modbus lrc"), value: "modbus lrc" }
                        ]
                        textRole: "text"
                        valueRole: "value"
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("Buffer Size")
                        font.pointSize: 12
                        Layout.fillWidth: true
                    }

                    SpinBox {
                        value: root.portManageWindow.config.bufferSize ?? 65536
                        onValueModified: root.portManageWindow.config.bufferSize = value
                        font.pointSize: 12
                        editable: true
                        from: 1
                        to: 67108864 // 64MB
                        Layout.fillWidth: true
                    }
                }

                // image
                SplitView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    orientation: Qt.Horizontal
                    handle: Item {
                        implicitWidth: 5

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            width: 1
                            color: global.theme.general.stroke
                        }
                    }

                    ColumnLayout {
                        SplitView.fillWidth: true; SplitView.fillHeight: true

                        Flickable {
                            clip: true
                            contentWidth: videoOutput.width
                            contentHeight: videoOutput.height
                            Layout.fillWidth: true; Layout.fillHeight: true

                            ScrollBar.vertical: ScrollBar {
                                minimumSize: Math.min(1, 20 / Math.max(1, height))
                                policy: ScrollBar.AsNeeded
                                palette {
                                    mid: global.theme.general.stroke
                                    dark: global.theme.general.strokePressed
                                }
                            }
                            ScrollBar.horizontal: ScrollBar {
                                minimumSize: Math.min(1, 20 / Math.max(1, width))
                                policy: ScrollBar.AsNeeded
                                palette {
                                    mid: global.theme.general.stroke
                                    dark: global.theme.general.strokePressed
                                }
                            }

                            VideoOutput {
                                id: videoOutput
                                property var indicatorList: []
                            }

                            Rectangle {
                                id: roiSelection
                                anchors.fill: videoOutput
                                color: global.theme.general.stroke
                                opacity: 0.3
                                visible: step !== 0
                                property int step: 0
                                property var roiList: []

                                TapHandler {
                                    acceptedButtons: Qt.LeftButton
                                    enabled: roiSelection.visible

                                    onTapped: (eventPoint) => {
                                        const position = eventPoint.position;
                                        roiSelection.roiRecord(position)
                                    }
                                }

                                Shortcut {
                                    enabled: roiSelection.visible
                                    sequence: "Esc"

                                    onActivated: roiSelection.roiStop()
                                }

                                HoverHandler {
                                    onHoveredChanged: {
                                        if (!hovered) {
                                            mainTooltip.text = ""
                                        }
                                    }
                                    onPointChanged: {
                                        mainTooltip.position = parent.mapToGlobal(point.position)
                                        mainTooltip.text = qsTr("%1, %2\nSelect %3/%4  Esc: Exit")
                                            .arg(Math.round(point.position.x))
                                            .arg(Math.round(point.position.y))
                                            .arg(roiSelection.roiList.length + 1)
                                            .arg(roiSelection.step)
                                    }
                                }

                                function roiRecord(position) {
                                    roiList.push(position)
                                    if (roiList.length === step) {
                                        if (step === 2) {
                                            let ix = Math.round(Math.min(roiList[0].x, roiList[1].x))
                                            let iy = Math.round(Math.min(roiList[0].y, roiList[1].y))
                                            let iw = Math.round(Math.abs(roiList[1].x - roiList[0].x))
                                            let ih = Math.round(Math.abs(roiList[1].y - roiList[0].y))
                                            root.portManageWindow.roiInsert([ix, iy, iw, ih])
                                        } else if (step === 4) {
                                            let ix0 = Math.round(roiList[0].x)
                                            let iy0 = Math.round(roiList[0].y)
                                            let ix1 = Math.round(roiList[1].x)
                                            let iy1 = Math.round(roiList[1].y)
                                            let ix2 = Math.round(roiList[2].x)
                                            let iy2 = Math.round(roiList[2].y)
                                            let ix3 = Math.round(roiList[3].x)
                                            let iy3 = Math.round(roiList[3].y)
                                            root.portManageWindow.roiInsert([ix0, iy0, ix1, iy1, ix2, iy2, ix3, iy3])
                                        }
                                        roiSelection.roiStop()
                                    }
                                }

                                function roiStart(step) {
                                    roiSelection.step = step
                                }

                                function roiStop() {
                                    roiSelection.step = 0
                                    roiSelection.roiList = []
                                    mainTooltip.text = ""
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Button {
                                flat: true
                                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                icon.source: "qrc:/icon/rectangle.svg"
                                icon.width: 32; icon.height: 32
                                Layout.preferredWidth: 48; Layout.preferredHeight: 48

                                onClicked: roiSelection.roiStart(2)

                                HoverHandler {
                                    onHoveredChanged: {
                                        if (!hovered) {
                                            mainTooltip.text = ""
                                        }
                                    }
                                    onPointChanged: {
                                        mainTooltip.position = parent.mapToGlobal(point.position)
                                        mainTooltip.text = qsTr("Rectangular")
                                    }
                                }
                            }

                            Button {
                                flat: true
                                leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                icon.source: "qrc:/icon/quadrilateral.svg"
                                icon.width: 32; icon.height: 32
                                Layout.preferredWidth: 48; Layout.preferredHeight: 48

                                onClicked: roiSelection.roiStart(4)

                                HoverHandler {
                                    onHoveredChanged: {
                                        if (!hovered) {
                                            mainTooltip.text = ""
                                        }
                                    }
                                    onPointChanged: {
                                        mainTooltip.position = parent.mapToGlobal(point.position)
                                        mainTooltip.text = qsTr("Quadrilateral")
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                            }
                        }
                    }

                    SplitView {
                        SplitView.preferredWidth: 300; SplitView.fillHeight: true
                        orientation: Qt.Vertical
                        handle: Item {
                            implicitHeight: 5

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.right: parent.right
                                height: 1
                                color: global.theme.general.stroke
                            }
                        }

                        // roi
                        ColumnLayout {
                            id: roiLayout
                            SplitView.fillWidth: true
                            SplitView.minimumHeight: roiHeader.implicitHeight
                            SplitView.preferredHeight: expanded ? 150 : roiHeader.implicitHeight
                            SplitView.maximumHeight: expanded ? 1000000 : roiHeader.implicitHeight
                            property bool expanded: false

                            RowLayout {
                                id: roiHeader
                                Layout.fillWidth: true

                                Button {
                                    flat: true
                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                    icon.source: roiLayout.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                                    icon.width: 16; icon.height: 16
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    onClicked: roiLayout.expanded = !roiLayout.expanded
                                }

                                Label {
                                    leftPadding: 6
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 20
                                    text: qsTr("ROI")
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Item {
                                visible: roiLayout.expanded
                                Layout.fillWidth: true; Layout.fillHeight: true

                                Item {
                                    anchors.fill: parent
                                    visible: root.roiModel.empty

                                    RowLayout {
                                        anchors.centerIn: parent

                                        Label {
                                            text: qsTr("Create ROI first.")
                                            font.pixelSize: 16
                                            Layout.alignment: Qt.AlignVCenter
                                        }

                                        IconImage {
                                            source: "qrc:/icon/rectangle.svg"
                                            color: global.theme.general.fore
                                            Layout.alignment: Qt.AlignVCenter
                                        }
                                    }
                                }

                                Item {
                                    anchors.fill: parent
                                    visible: !root.roiModel.empty

                                    VerticalHeaderView {
                                        id: roiVerticalHeaderView
                                        anchors.left: parent.left
                                        width: 24; height: parent.height
                                        syncView: roiTableView
                                        clip: true
                                        interactive: false
                                        movableRows: true
                                        delegate: VerticalHeaderViewDelegate {
                                            id: roiVerticalHeaderViewDelegate
                                            implicitWidth: roiVerticalHeaderView.width; implicitHeight: 24
                                            padding: 0

                                            contentItem: Rectangle {
                                                width: 24; height: 24
                                                color: global.theme.general.back

                                                IconImage {
                                                    anchors.centerIn: parent
                                                    width: 16; height: 16
                                                    color: global.theme.general.fore
                                                    source: "qrc:/icon/drag.svg"
                                                }
                                            }

                                            HoverHandler {
                                                onHoveredChanged: cursorShape = Qt.OpenHandCursor
                                            }
                                        }
                                        property var moves: []

                                        Rectangle {
                                            anchors.fill: parent
                                            color: global.theme.general.stroke
                                        }

                                        Timer {
                                            id: roiTimer
                                            interval: 10
                                            onTriggered: {
                                                let index = -1
                                                let distance = -1
                                                let currentDistance;
                                                for (let i = 0; i < roiVerticalHeaderView.moves.length; ++i) {
                                                    let move = roiVerticalHeaderView.moves[i]
                                                    currentDistance = Math.abs(move.oldVisualIndex - move.newVisualIndex)
                                                    if (currentDistance > distance) {
                                                        distance = currentDistance
                                                        index = i
                                                    }
                                                }
                                                let move = roiVerticalHeaderView.moves[index]
                                                root.portManageWindow.roiSwap(move.oldVisualIndex, move.newVisualIndex)
                                                roiVerticalHeaderView.clearRowReordering()
                                                roiTableView.clearRowReordering()
                                                roiVerticalHeaderView.moves = []
                                            }
                                        }

                                        onRowMoved: (logicalIndex, oldVisualIndex, newVisualIndex) => {
                                            moves.push({oldVisualIndex, newVisualIndex})
                                            roiTimer.restart()
                                        }
                                    }

                                    TableView {
                                        id: roiTableView
                                        anchors.left: roiVerticalHeaderView.right; anchors.right: parent.right
                                        height: parent.height
                                        alternatingRows: false
                                        clip: true
                                        editTriggers: TableView.NoEditTriggers
                                        rowSpacing: 1
                                        model: root.roiModel
                                        contentWidth: width
                                        property int hoveredRow: -1
                                        property int selectedRow: -1
                                        onSelectedRowChanged: previewLoader.start()

                                        ScrollBar.vertical: ScrollBar {
                                            minimumSize: Math.min(1, 20 / Math.max(1, height))
                                            policy: ScrollBar.AsNeeded
                                            palette {
                                                mid: global.theme.general.stroke
                                                dark: global.theme.general.strokePressed
                                            }
                                        }

                                        Rectangle {
                                            anchors.fill: parent
                                            color: global.theme.general.stroke
                                        }

                                        delegate: Item {
                                            implicitWidth: parent.width; implicitHeight: 24
                                            required property int row

                                            Rectangle {
                                                anchors.fill: parent
                                                color: global.theme.general.back
                                            }

                                            Rectangle {
                                                anchors.fill: parent
                                                radius: 6
                                                color: global.theme.general.backHover
                                                opacity: roiTableView.hoveredRow === row ? 1 : 0
                                                Behavior on opacity {
                                                    NumberAnimation {
                                                        duration: 150
                                                    }
                                                }
                                            }

                                            Rectangle {
                                                anchors.fill: parent
                                                radius: 6
                                                color: roiTableView.selectedRow === row ? global.theme.general.backSelected : "transparent"
                                            }

                                            Label {
                                                id: label
                                                anchors.fill: parent
                                                leftPadding: 6
                                                horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                                text: model.display || ""
                                                elide: Text.ElideRight
                                            }

                                            HoverHandler {
                                                id: hoverHandler
                                            }

                                            TapHandler {
                                                acceptedButtons: Qt.LeftButton
                                                gesturePolicy: TapHandler.DragWithinBounds

                                                onTapped: roiTableView.selectedRow = row
                                            }

                                            TapHandler {
                                                acceptedButtons: Qt.RightButton
                                                gesturePolicy: TapHandler.DragWithinBounds

                                                onSingleTapped: {
                                                    roiMenu.roiIndex = model.row
                                                    roiMenu.popup()
                                                }
                                            }
                                        }

                                        TapHandler {
                                            acceptedButtons: Qt.LeftButton
                                            gesturePolicy: TapHandler.DragWithinBounds

                                            onTapped: roiTableView.selectedRow = -1
                                        }
                                    }

                                    Menu {
                                        id: roiMenu
                                        property int roiIndex

                                        MenuItem {
                                            text: qsTr("Delete")
                                            icon.source: "qrc:/icon/delete.svg"
                                            icon.width: 16; icon.height: 16

                                            onTriggered: root.portManageWindow.roiRemove(roiMenu.roiIndex)
                                        }
                                    }
                                }
                            }
                        }

                        // pipeline
                        ColumnLayout {
                            id: pipelineLayout
                            SplitView.fillWidth: true
                            SplitView.minimumHeight: pipelineHeader.implicitHeight
                            SplitView.preferredHeight: expanded ? 200 : pipelineHeader.implicitHeight
                            SplitView.maximumHeight: expanded ? 1000000 : pipelineHeader.implicitHeight
                            property bool expanded: false

                            RowLayout {
                                id: pipelineHeader
                                Layout.fillWidth: true

                                Button {
                                    flat: true
                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                    icon.source: pipelineLayout.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                                    icon.width: 16; icon.height: 16
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    onClicked: pipelineLayout.expanded = !pipelineLayout.expanded
                                }

                                Label {
                                    leftPadding: 6
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 20
                                    text: qsTr("Image Pipeline")
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Button {
                                    flat: true
                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                    icon.source: "qrc:/icon/add.svg"
                                    icon.width: 24; icon.height: 24
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    onClicked: pipelineSlotMenu.popup()

                                    HoverHandler {
                                        onHoveredChanged: {
                                            if (!hovered) {
                                                mainTooltip.text = ""
                                            }
                                        }
                                        onPointChanged: {
                                            mainTooltip.position = parent.mapToGlobal(point.position)
                                            mainTooltip.text = qsTr("Add Pipeline")
                                        }
                                    }
                                }
                            }

                            Item {
                                visible: pipelineLayout.expanded
                                Layout.fillWidth: true; Layout.fillHeight: true

                                Item {
                                    anchors.fill: parent
                                    visible: root.pipelineModel.empty

                                    RowLayout {
                                        anchors.centerIn: parent

                                        Label {
                                            text: qsTr("Raw output.")
                                            font.pixelSize: 16
                                            Layout.alignment: Qt.AlignVCenter
                                        }
                                    }
                                }

                                Item {
                                    anchors.fill: parent
                                    visible: !root.pipelineModel.empty

                                    VerticalHeaderView {
                                        id: pipelineVerticalHeaderView
                                        anchors.left: parent.left
                                        width: 24; height: parent.height
                                        syncView: pipelineTableView
                                        clip: true
                                        interactive: false
                                        movableRows: true
                                        delegate: VerticalHeaderViewDelegate {
                                            id: pipelineVerticalHeaderViewDelegate
                                            implicitWidth: pipelineVerticalHeaderView.width; implicitHeight: 24
                                            padding: 0

                                            contentItem: Rectangle {
                                                width: 24; height: 24
                                                color: global.theme.general.back

                                                IconImage {
                                                    anchors.centerIn: parent
                                                    width: 16; height: 16
                                                    color: global.theme.general.fore
                                                    source: "qrc:/icon/drag.svg"
                                                }
                                            }

                                            HoverHandler {
                                                onHoveredChanged: cursorShape = Qt.OpenHandCursor
                                            }
                                        }
                                        property var moves: []

                                        Rectangle {
                                            anchors.fill: parent
                                            color: global.theme.general.stroke
                                        }

                                        Timer {
                                            id: pipelineTimer
                                            interval: 10
                                            onTriggered: {
                                                let index = -1
                                                let distance = -1
                                                let currentDistance;
                                                for (let i = 0; i < pipelineVerticalHeaderView.moves.length; ++i) {
                                                    let move = pipelineVerticalHeaderView.moves[i]
                                                    currentDistance = Math.abs(move.oldVisualIndex - move.newVisualIndex)
                                                    if (currentDistance > distance) {
                                                        distance = currentDistance
                                                        index = i
                                                    }
                                                }
                                                let move = pipelineVerticalHeaderView.moves[index]
                                                root.portManageWindow.pipelineSwap(move.oldVisualIndex, move.newVisualIndex)
                                                pipelineVerticalHeaderView.clearRowReordering()
                                                pipelineTableView.clearRowReordering()
                                                pipelineVerticalHeaderView.moves = []
                                            }
                                        }

                                        onRowMoved: (logicalIndex, oldVisualIndex, newVisualIndex) => {
                                            moves.push({oldVisualIndex, newVisualIndex})
                                            pipelineTimer.restart()
                                        }
                                    }

                                    TableView {
                                        id: pipelineTableView
                                        anchors.left: pipelineVerticalHeaderView.right; anchors.right: parent.right
                                        height: parent.height
                                        alternatingRows: false
                                        clip: true
                                        editTriggers: TableView.NoEditTriggers
                                        rowSpacing: 1
                                        model: root.pipelineModel
                                        contentWidth: width

                                        ScrollBar.vertical: ScrollBar {
                                            minimumSize: Math.min(1, 20 / Math.max(1, height))
                                            policy: ScrollBar.AsNeeded
                                            palette {
                                                mid: global.theme.general.stroke
                                                dark: global.theme.general.strokePressed
                                            }
                                        }

                                        Rectangle {
                                            anchors.fill: parent
                                            color: global.theme.general.stroke
                                        }

                                        delegate: DelegateChooser {
                                            role: "display"

                                            DelegateChoice {
                                                roleValue: qsTr("Scale")
                                                delegate: scaleDelegate
                                            }

                                            DelegateChoice {
                                                roleValue: qsTr("Threshold")
                                                delegate: thresholdDelegate
                                            }
                                        }

                                        Component {
                                            id: scaleDelegate

                                            Item {
                                                id: scaleItem
                                                implicitWidth: parent.width; implicitHeight: 80
                                                property var session: model.whatsThis

                                                Rectangle {
                                                    anchors.fill: parent
                                                    color: global.theme.general.back
                                                }

                                                ColumnLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 10

                                                    RowLayout {

                                                        Label {
                                                            leftPadding: 6
                                                            horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                                            text: model.display || ""
                                                            elide: Text.ElideRight
                                                            Layout.fillWidth: true
                                                        }

                                                        ComboBox {
                                                            id: scaleComboBox
                                                            currentIndex: session.interpolation
                                                            model: [
                                                                qsTr("Nearest"),
                                                                qsTr("Linear"),
                                                                qsTr("Cubic"),
                                                                qsTr("Area"),
                                                                qsTr("Lanczos4"),
                                                                qsTr("Linear Exact"),
                                                                qsTr("Nearest Exact")
                                                            ]
                                                            property bool initialized: false

                                                            Component.onCompleted: {
                                                                initialized = true
                                                            }

                                                            onCurrentTextChanged: {
                                                                if (!scaleComboBox.initialized) return
                                                                scaleItem.session.interpolation = scaleComboBox.currentIndex
                                                                const index = root.pipelineModel.index(row, 0);
                                                                root.pipelineModel.setData(index, scaleItem.session, Qt.WhatsThisRole)
                                                            }
                                                        }
                                                    }

                                                    DoubleSpinBox {
                                                        value: session.ratio
                                                        from: 0.1
                                                        to: 10
                                                        stepSize: 0.1
                                                        editable: true
                                                        Layout.fillWidth: true

                                                        textFromValue: function (value, locale) {
                                                            return "x" + value.toFixed(2)
                                                        }

                                                        valueFromText: function (text, locale) {
                                                            return Number(text.replace("x", ""))
                                                        }

                                                        onValueModified: {
                                                            scaleItem.session.ratio = value
                                                            const index = root.pipelineModel.index(row, 0);
                                                            root.pipelineModel.setData(index, scaleItem.session, Qt.WhatsThisRole)
                                                        }
                                                    }
                                                }

                                                TapHandler {
                                                    acceptedButtons: Qt.RightButton
                                                    gesturePolicy: TapHandler.DragWithinBounds

                                                    onSingleTapped: {
                                                        pipelineMenu.pipelineIndex = model.row
                                                        pipelineMenu.popup()
                                                    }
                                                }
                                            }
                                        }

                                        Component {
                                            id: thresholdDelegate

                                            Item {
                                                id: thresholdItem
                                                implicitWidth: parent.width; implicitHeight: 80
                                                property var session: model.whatsThis

                                                Rectangle {
                                                    anchors.fill: parent
                                                    color: global.theme.general.back
                                                }

                                                ColumnLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 10

                                                    RowLayout {

                                                        Label {
                                                            leftPadding: 6
                                                            horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                                            text: model.display || ""
                                                            elide: Text.ElideRight
                                                            Layout.fillWidth: true
                                                        }

                                                        ComboBox {
                                                            id: thresholdComboBox
                                                            currentIndex: session.mode
                                                            model: [
                                                                qsTr("Binary"),
                                                                qsTr("Binary Inv"),
                                                                qsTr("Trunc"),
                                                                qsTr("To Zero"),
                                                                qsTr("To Zero Inv")
                                                            ]
                                                            property bool initialized: false

                                                            Component.onCompleted: {
                                                                initialized = true
                                                            }

                                                            onCurrentTextChanged: {
                                                                if (!thresholdComboBox.initialized) return
                                                                thresholdItem.session.mode = thresholdComboBox.currentIndex
                                                                const index = root.pipelineModel.index(row, 0);
                                                                root.pipelineModel.setData(index, thresholdItem.session, Qt.WhatsThisRole)
                                                            }
                                                        }
                                                    }

                                                    RangeSlider {
                                                        id: thresholdSlider
                                                        first.value: session.thresh
                                                        second.value: session.maxval
                                                        from: 0
                                                        to: 255
                                                        Layout.fillWidth: true
                                                        ToolTip.text: first.value.toFixed(0) + " ~ " + second.value.toFixed(0)
                                                        ToolTip.visible: hovered

                                                        first.onMoved: {
                                                            thresholdItem.session.thresh = Math.round(thresholdSlider.first.value)
                                                            const index = root.pipelineModel.index(row, 0);
                                                            root.pipelineModel.setData(index, thresholdItem.session, Qt.WhatsThisRole)
                                                        }

                                                        second.onMoved: {
                                                            thresholdItem.session.maxval = Math.round(thresholdSlider.second.value)
                                                            const index = root.pipelineModel.index(row, 0);
                                                            root.pipelineModel.setData(index, thresholdItem.session, Qt.WhatsThisRole)
                                                        }
                                                    }
                                                }

                                                TapHandler {
                                                    acceptedButtons: Qt.RightButton
                                                    gesturePolicy: TapHandler.DragWithinBounds

                                                    onSingleTapped: {
                                                        pipelineMenu.pipelineIndex = model.row
                                                        pipelineMenu.popup()
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    Menu {
                                        id: pipelineMenu
                                        property int pipelineIndex

                                        MenuItem {
                                            text: qsTr("Delete")
                                            icon.source: "qrc:/icon/delete.svg"
                                            icon.width: 16; icon.height: 16

                                            onTriggered: root.portManageWindow.pipelineRemove(pipelineMenu.pipelineIndex)
                                        }
                                    }
                                }
                            }
                        }

                        // recognition
                        ColumnLayout {
                            id: recognitionLayout
                            SplitView.fillWidth: true
                            SplitView.minimumHeight: recognitionHeader.implicitHeight
                            SplitView.preferredHeight: expanded ? 100 : recognitionHeader.implicitHeight
                            SplitView.maximumHeight: expanded ? 1000000 : recognitionHeader.implicitHeight
                            property bool expanded: false

                            RowLayout {
                                id: recognitionHeader
                                Layout.fillWidth: true

                                Button {
                                    flat: true
                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                    icon.source: recognitionLayout.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                                    icon.width: 16; icon.height: 16
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    onClicked: recognitionLayout.expanded = !recognitionLayout.expanded
                                }

                                Label {
                                    leftPadding: 6
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 20
                                    text: qsTr("Recognition")
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                ComboBox {
                                    id: recognitionComboBox
                                    currentIndex: root.portManageWindow.config.recognition?.mode ?? RecognitionMethod.OCR
                                    model: [
                                        qsTr("OCR"),
                                        qsTr("Shi-Tomasi Corner"),
                                        qsTr("Harris Corner"),
                                        qsTr("Template Match")
                                    ]
                                    onActivated: root.portManageWindow.config.recognition.mode = currentIndex
                                }
                            }

                            StackLayout {
                                visible: recognitionLayout.expanded
                                Layout.fillWidth: true
                                currentIndex: recognitionComboBox.currentIndex

                                // OCR
                                ColumnLayout {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                }

                                // Shi-Tomasi Corner
                                ColumnLayout {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                }

                                // Harris Corner
                                ColumnLayout {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                }

                                // Template Match
                                ColumnLayout {
                                    Layout.fillWidth: true; Layout.fillHeight: true

                                    LineEdit {
                                        clearable: true
                                        browsable: true
                                        nameFilters: [qsTr("Image files (*.png)")]
                                        Layout.fillWidth: true
                                        Binding on text { value: root.portManageWindow.config.recognition?.template ?? "" }

                                        onValueCommitted: root.portManageWindow.config.recognition.template = text
                                    }
                                }
                            }
                        }

                        // preview
                        ColumnLayout {
                            id: previewLayout
                            SplitView.fillWidth: true; SplitView.fillHeight: true
                            SplitView.minimumHeight: previewHeader.implicitHeight
                            SplitView.preferredHeight: expanded ? 300 : previewHeader.implicitHeight
                            SplitView.maximumHeight: expanded ? 1000000 : previewHeader.implicitHeight
                            property bool expanded: false

                            RowLayout {
                                id: previewHeader
                                Layout.fillWidth: true
                                property int previousHeight

                                Button {
                                    flat: true
                                    leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                    icon.source: previewLayout.expanded ? "qrc:/icon/arrowExpand.svg" : "qrc:/icon/arrowCollapse.svg"
                                    icon.width: 16; icon.height: 16
                                    Layout.preferredWidth: 24; Layout.preferredHeight: 24

                                    onClicked: previewLayout.expanded = !previewLayout.expanded
                                }

                                Label {
                                    leftPadding: 6
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 20
                                    text: qsTr("Image Preview")
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Flickable {
                                visible: previewLayout.expanded
                                clip: true
                                contentWidth: previewImage.width
                                contentHeight: previewImage.height
                                Layout.fillWidth: true; Layout.fillHeight: true

                                ScrollBar.vertical: ScrollBar {
                                    minimumSize: Math.min(1, 20 / Math.max(1, height))
                                    policy: ScrollBar.AsNeeded
                                    palette {
                                        mid: global.theme.general.stroke
                                        dark: global.theme.general.strokePressed
                                    }
                                }
                                ScrollBar.horizontal: ScrollBar {
                                    minimumSize: Math.min(1, 20 / Math.max(1, width))
                                    policy: ScrollBar.AsNeeded
                                    palette {
                                        mid: global.theme.general.stroke
                                        dark: global.theme.general.strokePressed
                                    }
                                }

                                Image {
                                    id: previewImage
                                    source: "qrc:/icon/null.svg"
                                    property var recognition: ({})

                                    Repeater {
                                        model: previewImage.recognition.items || []

                                        delegate: Item {
                                            id: recognitionDelegate
                                            required property var modelData
                                            readonly property bool point: modelData.width === 0 && modelData.height === 0
                                            x: modelData.x
                                            y: modelData.y
                                            width: point ? 1 : modelData.width
                                            height: point ? 1 : modelData.height

                                            Rectangle {
                                                anchors.fill: parent
                                                visible: !recognitionDelegate.point
                                                color: "transparent"
                                                border.width: 2
                                                border.color: global.theme.general.dangerFore3

                                                Label {
                                                    width: parent.width
                                                    leftPadding: 2; rightPadding: 2
                                                    color: global.theme.general.fore
                                                    font.pixelSize: 11
                                                    text: recognitionDelegate.modelData.text + "\n" + Number(recognitionDelegate.modelData.confidence).toFixed(1) + "%"
                                                    elide: Text.ElideRight
                                                    background: Rectangle {
                                                        color: global.theme.general.back
                                                        opacity: 0.8
                                                    }
                                                }
                                            }

                                            Rectangle {
                                                x: -8
                                                y: -1
                                                width: 16
                                                height: 2
                                                visible: recognitionDelegate.point
                                                color: global.theme.general.dangerFore3
                                            }

                                            Rectangle {
                                                x: -1
                                                y: -8
                                                width: 2
                                                height: 16
                                                visible: recognitionDelegate.point
                                                color: global.theme.general.dangerFore3
                                            }
                                        }
                                    }
                                }

                                Item {
                                    id: previewLoader
                                    property bool running: false
                                    property int framerate: 0
                                    property int frames: 0
                                    property double timestamp: 0

                                    function start() {
                                        if (running) return
                                        running = true
                                        frames = 0
                                        timestamp = Date.now()
                                        next()
                                    }

                                    function stop() {
                                        previewImage.source = "qrc:/icon/null.svg"
                                        previewImage.recognition = ({})
                                        running = false
                                        framerate = 0
                                    }

                                    function next() {
                                        if (!running) return
                                        if (!root.Window.window.visible || root.roiModel.empty || roiTableView.selectedRow === -1) {
                                            stop()
                                            return
                                        }
                                        const result = root.portManageWindow.previewLoad(roiTableView.selectedRow)
                                        if (result.source) {
                                            previewImage.source = result.source
                                            previewImage.recognition = result.recognition
                                        }
                                        ++frames
                                        const now = Date.now()
                                        if (now - timestamp >= 1000) {
                                            framerate = Math.round(frames * 1000 / (now - timestamp))
                                            frames = 0
                                            timestamp = now
                                        }
                                        Qt.callLater(next)
                                    }
                                }
                            }

                            RowLayout {
                                visible: previewLayout.expanded
                                Layout.fillWidth: true

                                Label {
                                    leftPadding: 6
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 20
                                    text: previewImage.recognition.text || ""
                                    elide: Text.ElideRight
                                }

                                Item {
                                    Layout.fillWidth: true
                                }

                                Label {
                                    rightPadding: 6
                                    horizontalAlignment: Text.AlignLeft; verticalAlignment: Text.AlignVCenter
                                    font.pixelSize: 20
                                    text: previewLoader.framerate + " fps"
                                    elide: Text.ElideLeft
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: false
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: 20

            Button {
                text: qsTr("Prev")
                enabled: swipeView.currentIndex !== 0
                flat: true

                onClicked: {
                    swipeView.currentIndex = swipeView.currentIndex - 1
                    root.portManageWindow.width = 600
                    root.portManageWindow.height = 500
                }
            }

            PageIndicator {
                id: pageIndicator
                count: swipeView.count
                currentIndex: swipeView.currentIndex
                Layout.fillWidth: false
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                background: Rectangle {
                    color: "transparent"
                }
            }

            Button {
                text: swipeView.currentIndex !== 2 ? qsTr("Next") : qsTr("Confirm")
                highlighted: swipeView.currentIndex === 2
                flat: true

                function validationError() {
                    const config = root.portManageWindow.config
                    const type = root.portType
                    if (!config.portName || (type === PortType.BluetoothLe && !config.portName.trim()))
                        return qsTr("Invalid Port Name")
                    switch (type) {
                    case PortType.SerialPort:
                        if (!serialPortDeviceNameComboBox.currentText) return qsTr("Invalid Device Name")
                        break
                    case PortType.Vision:
                        if (!visionDeviceNameComboBox.currentText) return qsTr("Invalid Device Name")
                        break
                    case PortType.TcpClient:
                    case PortType.SslClient:
                    case PortType.WebSocketClient:
                        if (!config.remoteHost) return qsTr("Empty Remote Host")
                        break
                    case PortType.TcpServer:
                    case PortType.SslServer:
                    case PortType.WebSocketServer:
                    case PortType.UdpSocket:
                        if (!config.localHost || (type === PortType.UdpSocket && !udpSocketLocalHostComboBox.currentText))
                            return qsTr("Invalid Local Host")
                        if (type === PortType.UdpSocket && !config.remoteHost) return qsTr("Invalid Remote Host")
                        if (type === PortType.SslServer || (type === PortType.WebSocketServer && config.secure)) {
                            if (!config.certificate?.trim()) return qsTr("Server Certificate Required")
                            if (!config.privateKey?.trim()) return qsTr("Private Key Required")
                        }
                        break
                    case PortType.BluetoothLe:
                        if (!config.peripheralAddress) return qsTr("Invalid Address")
                        break
                    }
                    return ""
                }

                onClicked: {
                    if (swipeView.currentIndex === 0) {
                        swipeView.currentIndex = swipeView.currentIndex + 1
                    } else if (swipeView.currentIndex === 1) {
                        const error = validationError()
                        if (error) {
                            portNameValidator.text = error
                            portNameValidatorTimer.start()
                            return
                        }
                        if (root.portType === PortType.Vision) {
                            root.portManageWindow.width = 1600
                            root.portManageWindow.height = 900
                            root.portManageWindow.visionCapture(videoOutput.videoSink)
                        }
                        swipeView.currentIndex = swipeView.currentIndex + 1
                    } else {
                        root.portManageWindow.save()
                    }
                }
            }
        }
    }

    // pipeline
    Menu {
        id: pipelineSlotMenu

        MenuItem {
            text: qsTr("Scale")
            icon.source: "qrc:/icon/resize.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                let session = {}
                session.type = ImageOperation.Scale
                session.ratio = 1.0
                session.interpolation = 1
                root.portManageWindow.pipelineInsert(session)
            }
        }

        MenuItem {
            text: qsTr("Threshold")
            icon.source: "qrc:/icon/contrast.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                let session = {}
                session.type = ImageOperation.Threshold
                session.thresh = 0
                session.maxval = 255
                session.mode = 0
                root.portManageWindow.pipelineInsert(session)
            }
        }
    }

    // indicator
    Component {
        id: indicatorRectComponent

        Rectangle {
            id: indicatorRect
            color: roiTableView.selectedRow === index ? Qt.alpha(global.theme.general.brandBackSelected, 0.3) : "transparent"
            border.color: global.theme.general.brandStroke
            border.width: 1
            property int index

            Label {
                x: parent.width / 2 - implicitWidth / 2
                y: parent.height / 2 - implicitHeight / 2
                text: index.toString()
                font.bold: true
                font.pixelSize: 20
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.SizeAllCursor
                preventStealing: true
                property point lastPos: Qt.point(0, 0)

                onPressed: (mouse) => {
                    roiTableView.selectedRow = index
                    lastPos = mapToItem(videoOutput, mouse.x, mouse.y)
                }

                onPositionChanged: (mouse) => {
                    const pos = mapToItem(videoOutput, mouse.x, mouse.y)
                    indicatorRect.x += pos.x - lastPos.x
                    indicatorRect.y += pos.y - lastPos.y
                    lastPos = pos
                }

                onReleased: indicatorRect.roiSet()
            }

            Rectangle {
                id: topLeftHandle
                visible: roiTableView.selectedRow === index
                x: -width / 2
                y: -height / 2
                width: 10
                height: 10
                radius: width / 2
                color: global.theme.general.brandBackSelected
                border.color: global.theme.general.brandStroke
                border.width: 1
                z: 1

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeFDiagCursor
                    preventStealing: true
                    property point lastPos: Qt.point(0, 0)

                    onPressed: (mouse) => {
                        roiTableView.selectedRow = index
                        lastPos = mapToItem(videoOutput, mouse.x, mouse.y)
                    }

                    onPositionChanged: (mouse) => {
                        const pos = mapToItem(videoOutput, mouse.x, mouse.y)
                        const dx = pos.x - lastPos.x
                        const dy = pos.y - lastPos.y
                        indicatorRect.x += dx
                        indicatorRect.y += dy
                        indicatorRect.width -= dx
                        indicatorRect.height -= dy
                        lastPos = pos
                    }

                    onReleased: indicatorRect.roiSet()
                }
            }

            Rectangle {
                id: bottomRightHandle
                visible: roiTableView.selectedRow === index
                x: parent.width - width / 2
                y: parent.height - height / 2
                width: 10
                height: 10
                radius: width / 2
                color: global.theme.general.brandBackSelected
                border.color: global.theme.general.brandStroke
                border.width: 1
                z: 1

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeFDiagCursor
                    preventStealing: true
                    property point lastPos: Qt.point(0, 0)

                    onPressed: (mouse) => {
                        roiTableView.selectedRow = index
                        lastPos = mapToItem(videoOutput, mouse.x, mouse.y)
                    }

                    onPositionChanged: (mouse) => {
                        const pos = mapToItem(videoOutput, mouse.x, mouse.y)
                        indicatorRect.width += pos.x - lastPos.x
                        indicatorRect.height += pos.y - lastPos.y
                        lastPos = pos
                    }

                    onReleased: indicatorRect.roiSet()
                }
            }

            function roiSet() {
                const ix = Math.round(indicatorRect.x)
                const iy = Math.round(indicatorRect.y)
                const iw = Math.round(indicatorRect.width)
                const ih = Math.round(indicatorRect.height)
                const modelIndex = root.roiModel.index(index, 0)
                root.roiModel.setData(modelIndex, [ix, iy, iw, ih], Qt.WhatsThisRole)
            }
        }
    }

    Component {
        id: indicatorQuadComponent

        Shape {
            id: indicatorQuadShape
            property int x0
            property int y0
            property int x1
            property int y1
            property int x2
            property int y2
            property int x3
            property int y3
            property int index

            ShapePath {
                fillColor: roiTableView.selectedRow === index ? Qt.alpha(global.theme.general.brandBackSelected, 0.3) : "transparent"
                strokeColor: global.theme.general.brandStroke
                strokeWidth: 1
                startX: x0; startY: y0

                PathLine {
                    x: x1; y: y1
                }
                PathLine {
                    x: x2; y: y2
                }
                PathLine {
                    x: x3; y: y3
                }
                PathLine {
                    x: x0; y: y0
                }
            }

            Label {
                x: indicatorQuadShape.boundingRect.x + indicatorQuadShape.boundingRect.width / 2 - implicitWidth / 2
                y: indicatorQuadShape.boundingRect.y + indicatorQuadShape.boundingRect.height / 2 - implicitHeight / 2
                text: index.toString()
                font.bold: true
                font.pixelSize: 20
            }

            Item {
                x: indicatorQuadShape.boundingRect.x
                y: indicatorQuadShape.boundingRect.y
                width: indicatorQuadShape.boundingRect.width
                height: indicatorQuadShape.boundingRect.height

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeAllCursor
                    preventStealing: true
                    property point startPos: Qt.point(0, 0)
                    property int startX0
                    property int startY0
                    property int startX1
                    property int startY1
                    property int startX2
                    property int startY2
                    property int startX3
                    property int startY3

                    onPressed: (mouse) => {
                        roiTableView.selectedRow = index
                        startPos = mapToItem(videoOutput, mouse.x, mouse.y)
                        startX0 = indicatorQuadShape.x0
                        startY0 = indicatorQuadShape.y0
                        startX1 = indicatorQuadShape.x1
                        startY1 = indicatorQuadShape.y1
                        startX2 = indicatorQuadShape.x2
                        startY2 = indicatorQuadShape.y2
                        startX3 = indicatorQuadShape.x3
                        startY3 = indicatorQuadShape.y3
                    }

                    onPositionChanged: (mouse) => {
                        const pos = mapToItem(videoOutput, mouse.x, mouse.y)
                        const dx = pos.x - startPos.x
                        const dy = pos.y - startPos.y
                        indicatorQuadShape.x0 = Math.round(startX0 + dx)
                        indicatorQuadShape.y0 = Math.round(startY0 + dy)
                        indicatorQuadShape.x1 = Math.round(startX1 + dx)
                        indicatorQuadShape.y1 = Math.round(startY1 + dy)
                        indicatorQuadShape.x2 = Math.round(startX2 + dx)
                        indicatorQuadShape.y2 = Math.round(startY2 + dy)
                        indicatorQuadShape.x3 = Math.round(startX3 + dx)
                        indicatorQuadShape.y3 = Math.round(startY3 + dy)
                    }

                    onReleased: indicatorQuadShape.roiSet()
                }
            }

            Rectangle {
                visible: roiTableView.selectedRow === index
                x: indicatorQuadShape.x0 - width / 2
                y: indicatorQuadShape.y0 - height / 2
                width: 10
                height: 10
                radius: width / 2
                color: global.theme.general.brandBackSelected
                border.color: global.theme.general.brandStroke
                border.width: 1
                z: 1

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeFDiagCursor
                    preventStealing: true
                    property point startPos: Qt.point(0, 0)
                    property int startX
                    property int startY

                    onPressed: (mouse) => {
                        roiTableView.selectedRow = index
                        startPos = mapToItem(videoOutput, mouse.x, mouse.y)
                        startX = indicatorQuadShape.x0
                        startY = indicatorQuadShape.y0
                    }

                    onPositionChanged: (mouse) => {
                        const pos = mapToItem(videoOutput, mouse.x, mouse.y)
                        indicatorQuadShape.x0 = Math.round(startX + pos.x - startPos.x)
                        indicatorQuadShape.y0 = Math.round(startY + pos.y - startPos.y)
                    }

                    onReleased: indicatorQuadShape.roiSet()
                }
            }

            Rectangle {
                visible: roiTableView.selectedRow === index
                x: indicatorQuadShape.x1 - width / 2
                y: indicatorQuadShape.y1 - height / 2
                width: 10
                height: 10
                radius: width / 2
                color: global.theme.general.brandBackSelected
                border.color: global.theme.general.brandStroke
                border.width: 1
                z: 1

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeFDiagCursor
                    preventStealing: true
                    property point startPos: Qt.point(0, 0)
                    property int startX
                    property int startY

                    onPressed: (mouse) => {
                        roiTableView.selectedRow = index
                        startPos = mapToItem(videoOutput, mouse.x, mouse.y)
                        startX = indicatorQuadShape.x1
                        startY = indicatorQuadShape.y1
                    }

                    onPositionChanged: (mouse) => {
                        const pos = mapToItem(videoOutput, mouse.x, mouse.y)
                        indicatorQuadShape.x1 = Math.round(startX + pos.x - startPos.x)
                        indicatorQuadShape.y1 = Math.round(startY + pos.y - startPos.y)
                    }

                    onReleased: indicatorQuadShape.roiSet()
                }
            }

            Rectangle {
                visible: roiTableView.selectedRow === index
                x: indicatorQuadShape.x2 - width / 2
                y: indicatorQuadShape.y2 - height / 2
                width: 10
                height: 10
                radius: width / 2
                color: global.theme.general.brandBackSelected
                border.color: global.theme.general.brandStroke
                border.width: 1
                z: 1

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeFDiagCursor
                    preventStealing: true
                    property point startPos: Qt.point(0, 0)
                    property int startX
                    property int startY

                    onPressed: (mouse) => {
                        roiTableView.selectedRow = index
                        startPos = mapToItem(videoOutput, mouse.x, mouse.y)
                        startX = indicatorQuadShape.x2
                        startY = indicatorQuadShape.y2
                    }

                    onPositionChanged: (mouse) => {
                        const pos = mapToItem(videoOutput, mouse.x, mouse.y)
                        indicatorQuadShape.x2 = Math.round(startX + pos.x - startPos.x)
                        indicatorQuadShape.y2 = Math.round(startY + pos.y - startPos.y)
                    }

                    onReleased: indicatorQuadShape.roiSet()
                }
            }

            Rectangle {
                visible: roiTableView.selectedRow === index
                x: indicatorQuadShape.x3 - width / 2
                y: indicatorQuadShape.y3 - height / 2
                width: 10
                height: 10
                radius: width / 2
                color: global.theme.general.brandBackSelected
                border.color: global.theme.general.brandStroke
                border.width: 1
                z: 1

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeFDiagCursor
                    preventStealing: true
                    property point startPos: Qt.point(0, 0)
                    property int startX
                    property int startY

                    onPressed: (mouse) => {
                        roiTableView.selectedRow = index
                        startPos = mapToItem(videoOutput, mouse.x, mouse.y)
                        startX = indicatorQuadShape.x3
                        startY = indicatorQuadShape.y3
                    }

                    onPositionChanged: (mouse) => {
                        const pos = mapToItem(videoOutput, mouse.x, mouse.y)
                        indicatorQuadShape.x3 = Math.round(startX + pos.x - startPos.x)
                        indicatorQuadShape.y3 = Math.round(startY + pos.y - startPos.y)
                    }

                    onReleased: indicatorQuadShape.roiSet()
                }
            }

            function roiSet() {
                const modelIndex = root.roiModel.index(index, 0)
                root.roiModel.setData(modelIndex, [
                    indicatorQuadShape.x0, indicatorQuadShape.y0,
                    indicatorQuadShape.x1, indicatorQuadShape.y1,
                    indicatorQuadShape.x2, indicatorQuadShape.y2,
                    indicatorQuadShape.x3, indicatorQuadShape.y3
                ], Qt.WhatsThisRole)
            }
        }
    }

    function indicatorReload() {
        for (let i = 0; i < videoOutput.indicatorList.length; ++i) {
            videoOutput.indicatorList[i].destroy()
        }
        videoOutput.indicatorList = []
        for (let row = 0; row < root.roiModel.rowCount(); ++row) {
            const index = root.roiModel.index(row, 0)
            const roi = root.roiModel.data(index, Qt.WhatsThisRole)
            var indicator
            if (roi.length === 4) {
                // rect roi
                indicator = indicatorRectComponent.createObject(videoOutput, {
                    x: roi[0],
                    y: roi[1],
                    width: roi[2],
                    height: roi[3],
                    index: row
                });
            } else if (roi.length === 8) {
                // quad roi
                indicator = indicatorQuadComponent.createObject(videoOutput, {
                    x0: roi[0],
                    y0: roi[1],
                    x1: roi[2],
                    y1: roi[3],
                    x2: roi[4],
                    y2: roi[5],
                    x3: roi[6],
                    y3: roi[7],
                    index: row
                });
            }
            videoOutput.indicatorList.push(indicator)
        }
    }
}
