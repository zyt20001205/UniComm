import QtMultimedia
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Shapes

Item {
    id: rootItem
    property int portType: 0
    property bool bluetoothBusy: false
    property var patterns: {
        "ipv4": /^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/,
        "ipv6": /[0-9a-fA-F]{1,4}(:[0-9a-fA-F]{1,4}){3}/
    }

    Rectangle {
        anchors.fill: parent
        color: global.back
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
                    currentIndex: rootItem.portType
                    delegate: delegateComponent
                    model: [qsTr("Serial Port"), qsTr("Visa"), qsTr("Tcp Client"), qsTr("Tcp Server"), qsTr("Ssl Client"), qsTr("Ssl Server"), qsTr("WebSocket Client"), qsTr("WebSocket Server"), qsTr("Udp Socket"), qsTr("Video Stream"), qsTr("Bluetooth LE")]
                    wrap: false
                    Layout.fillWidth: true; Layout.fillHeight: true

                    onMovingChanged: {
                        if (!moving) {
                            rootItem.portType = currentIndex
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
                        color: global.fore
                        Layout.preferredWidth: 80; Layout.preferredHeight: 80
                        Layout.alignment: Qt.AlignHCenter

                        function sourceGet() {
                            switch (rootItem.portType) {
                                case 0:
                                    return "qrc:/icon/serialPort.svg"
                                case 1:
                                    return ""
                                case 2:
                                    return "qrc:/icon/tcpClient.svg"
                                case 3:
                                    return "qrc:/icon/tcpServer.svg"
                                case 4:
                                    return ""
                                case 5:
                                    return ""
                                case 6:
                                    return ""
                                case 7:
                                    return ""
                                case 8:
                                    return "qrc:/icon/udpSocket.svg"
                                case 9:
                                    return "qrc:/icon/video.svg"
                                case 10:
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
                            switch (rootItem.portType) {
                                case 0:
                                    return qsTr("A serial communication interface through which information transfers in or out sequentially one bit at a time.")
                                case 1:
                                    return qsTr("A widely used application programming interface (API) in the test and measurement (T&M) industry for communicating with instruments from a computer.")
                                case 2:
                                    return qsTr("A device that initiates a connection with a TCP server to send and receive reliable, ordered data over a network.")
                                case 3:
                                    return qsTr("A device that listens on a network port, accepts incoming connections from TCP clients, and manages reliable, ordered data exchange.")
                                case 4:
                                    return qsTr("A secure client that establishes encrypted connections with SSL/TLS servers to ensure data confidentiality and integrity during transmission.")
                                case 5:
                                    return qsTr("A secure server that accepts encrypted SSL/TLS byte-stream connections from multiple clients.")
                                case 6:
                                    return qsTr("A client that establishes a full-duplex WebSocket connection and exchanges text or binary messages.")
                                case 7:
                                    return qsTr("A server that accepts multiple WebSocket clients over ws or wss and manages each peer independently.")
                                case 8:
                                    return qsTr("A device that uses the User Datagram Protocol to send independent, connectionless messages (datagrams) over an IP network.")
                                case 9:
                                    return qsTr("Video stream for image processing and OCR text recognition.")
                                case 10:
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

                StackLayout {
                    currentIndex: rootItem.portType
                    Layout.fillWidth: true; Layout.fillHeight: true

                    // serial port
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: serialPortNameComboBox
                            font.pointSize: 12
                            model: serialPortStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Baud Rate")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            id: serialPortBaudRateSpinBox
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
                            id: serialPortDataBitsComboBox
                            font.pointSize: 12
                            model: ListModel {
                                ListElement {
                                    text: "5"; value: 5
                                }
                                ListElement {
                                    text: "6"; value: 6
                                }
                                ListElement {
                                    text: "7"; value: 7
                                }
                                ListElement {
                                    text: "8"; value: 8
                                }
                            }
                            textRole: "text"
                            valueRole: "value"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Parity")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: serialPortParityComboBox
                            font.pointSize: 12
                            model: ListModel {
                                ListElement {
                                    text: qsTr("No"); value: 0
                                }
                                ListElement {
                                    text: qsTr("Even"); value: 2
                                }
                                ListElement {
                                    text: qsTr("Odd"); value: 3
                                }
                                ListElement {
                                    text: qsTr("Space"); value: 4
                                }
                                ListElement {
                                    text: qsTr("Mark"); value: 5
                                }
                            }
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
                            id: serialPortStopBitsComboBox
                            font.pointSize: 12
                            model: ListModel {
                                ListElement {
                                    text: "1"; value: 1
                                }
                                ListElement {
                                    text: "1.5"; value: 3
                                }
                                ListElement {
                                    text: "2"; value: 2
                                }
                            }
                            textRole: "text"
                            valueRole: "value"
                            Layout.fillWidth: true
                        }
                    }

                    // visa
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: visaNameComboBox
                            font.pointSize: 12
                            model: visaStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
                            Layout.fillWidth: true
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

                        TextField {
                            id: tcpClientNameTextField
                            font.pointSize: 12
                            placeholderText: qsTr("Tcp Client")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Remote Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: tcpClientRemoteHostTextField
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Remote Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            id: tcpClientRemotePortSpinBox
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

                        TextField {
                            id: tcpServerNameTextField
                            font.pointSize: 12
                            placeholderText: qsTr("Tcp Server")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Local Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: tcpServerLocalHostComboBox
                            font.pointSize: 12
                            model: localHostStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Local Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            id: tcpServerLocalPortSpinBox
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
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: sslClientNameTextField
                            font.pointSize: 12
                            placeholderText: qsTr("Ssl Client")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Remote Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: sslClientRemoteHostTextField
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Remote Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            id: sslClientRemotePortSpinBox
                            font.pointSize: 12
                            editable: true
                            from: 0
                            to: 65535
                            Layout.fillWidth: true
                        }
                    }

                    // ssl server
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: sslServerNameTextField
                            font.pointSize: 12
                            placeholderText: qsTr("Ssl Server")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Local Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: sslServerLocalHostComboBox
                            font.pointSize: 12
                            model: localHostStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Local Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            id: sslServerLocalPortSpinBox
                            font.pointSize: 12
                            editable: true
                            from: 0
                            to: 65535
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Certificate")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: sslServerCertificateTextField
                            font.pointSize: 12
                            placeholderText: qsTr("PEM certificate path")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Private Key")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: sslServerPrivateKeyTextField
                            font.pointSize: 12
                            placeholderText: qsTr("PEM private key path")
                            Layout.fillWidth: true
                        }
                    }

                    // web socket client
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: webSocketClientNameTextField
                            font.pointSize: 12
                            placeholderText: qsTr("WebSocket Client")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("URL")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: webSocketClientUrlTextField
                            font.pointSize: 12
                            placeholderText: "ws://127.0.0.1:8080"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Message Type")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: webSocketClientMessageTypeComboBox
                            font.pointSize: 12
                            model: ListModel {
                                ListElement {
                                    text: qsTr("Binary"); value: "binary"
                                }
                                ListElement {
                                    text: qsTr("Text"); value: "text"
                                }
                            }
                            textRole: "text"
                            valueRole: "value"
                            Layout.fillWidth: true
                        }
                    }

                    // web socket server
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: webSocketServerNameTextField
                            font.pointSize: 12
                            placeholderText: qsTr("WebSocket Server")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Local Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: webSocketServerLocalHostComboBox
                            font.pointSize: 12
                            model: localHostStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Local Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            id: webSocketServerLocalPortSpinBox
                            font.pointSize: 12
                            editable: true
                            from: 0
                            to: 65535
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Secure")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        Switch {
                            id: webSocketServerSecureSwitch
                            Layout.alignment: Qt.AlignLeft
                        }

                        Label {
                            text: qsTr("Certificate")
                            enabled: webSocketServerSecureSwitch.checked
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: webSocketServerCertificateTextField
                            enabled: webSocketServerSecureSwitch.checked
                            font.pointSize: 12
                            placeholderText: qsTr("PEM certificate path")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Private Key")
                            enabled: webSocketServerSecureSwitch.checked
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: webSocketServerPrivateKeyTextField
                            enabled: webSocketServerSecureSwitch.checked
                            font.pointSize: 12
                            placeholderText: qsTr("PEM private key path")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Message Type")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: webSocketServerMessageTypeComboBox
                            font.pointSize: 12
                            model: ListModel {
                                ListElement {
                                    text: qsTr("Binary"); value: "binary"
                                }
                                ListElement {
                                    text: qsTr("Text"); value: "text"
                                }
                            }
                            textRole: "text"
                            valueRole: "value"
                            Layout.fillWidth: true
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

                        TextField {
                            id: udpSocketNameTextField
                            font.pointSize: 12
                            placeholderText: qsTr("Udp Socket")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Local Host")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: udpSocketLocalHostComboBox
                            font.pointSize: 12
                            model: localHostStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Local Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            id: udpSocketLocalPortSpinBox
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

                        TextField {
                            id: udpSocketRemoteHostTextField
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Remote Port")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        SpinBox {
                            id: udpSocketRemotePortSpinBox
                            font.pointSize: 12
                            editable: true
                            from: 0
                            to: 65535
                            Layout.fillWidth: true
                        }
                    }

                    // video stream
                    GridLayout {
                        columns: 2
                        columnSpacing: 20; rowSpacing: 20

                        Label {
                            text: qsTr("Port Name")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: videoStreamNameComboBox
                            font.pointSize: 12
                            model: videoStreamStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
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

                        TextField {
                            id: bluetoothNameTextField
                            font.pointSize: 12
                            placeholderText: qsTr("Bluetooth LE")
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Adapter")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            ComboBox {
                                id: bluetoothAdapterComboBox
                                enabled: !rootItem.bluetoothBusy
                                font.pointSize: 12
                                model: bluetoothAdapterStandardItemModel
                                textRole: "display"
                                valueRole: "whatsThis"
                                Layout.fillWidth: true
                            }

                            Button {
                                text: qsTr("Scan")
                                enabled: !rootItem.bluetoothBusy && bluetoothAdapterComboBox.currentIndex >= 0
                                onClicked: portSetting.bluetoothScan(bluetoothAdapterComboBox.currentValue)
                            }
                        }

                        Label {
                            text: qsTr("Peripheral")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            ComboBox {
                                id: bluetoothPeripheralComboBox
                                enabled: !rootItem.bluetoothBusy
                                font.pointSize: 12
                                model: bluetoothPeripheralStandardItemModel
                                textRole: "display"
                                valueRole: "whatsThis"
                                Layout.fillWidth: true
                            }

                            Button {
                                text: qsTr("Discover")
                                enabled: !rootItem.bluetoothBusy && bluetoothPeripheralComboBox.currentIndex >= 0
                                onClicked: portSetting.bluetoothDiscover(bluetoothAdapterComboBox.currentValue, bluetoothPeripheralComboBox.currentValue)
                            }
                        }

                        Label {
                            text: qsTr("Service")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: bluetoothServiceComboBox
                            enabled: !rootItem.bluetoothBusy
                            font.pointSize: 12
                            model: bluetoothServiceStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
                            Layout.fillWidth: true
                            onActivated: portSetting.bluetoothServiceSelect(currentValue)
                        }

                        Label {
                            text: qsTr("Tx Characteristic")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: bluetoothTxCharacteristicComboBox
                            enabled: !rootItem.bluetoothBusy
                            font.pointSize: 12
                            model: bluetoothTxCharacteristicStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Write Type")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: bluetoothWriteTypeComboBox
                            enabled: !rootItem.bluetoothBusy
                            font.pointSize: 12
                            model: ListModel {
                                ListElement {
                                    text: qsTr("Request"); value: "request"
                                }
                                ListElement {
                                    text: qsTr("Command"); value: "command"
                                }
                            }
                            textRole: "text"
                            valueRole: "value"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Rx Characteristic")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: bluetoothRxCharacteristicComboBox
                            enabled: !rootItem.bluetoothBusy
                            font.pointSize: 12
                            model: bluetoothRxCharacteristicStandardItemModel
                            textRole: "display"
                            valueRole: "whatsThis"
                            Layout.fillWidth: true
                        }

                        Label {
                            text: qsTr("Subscribe Type")
                            font.pointSize: 12
                            Layout.fillWidth: true
                        }

                        ComboBox {
                            id: bluetoothSubscribeTypeComboBox
                            enabled: !rootItem.bluetoothBusy
                            font.pointSize: 12
                            model: ListModel {
                                ListElement {
                                    text: qsTr("Notify"); value: "notify"
                                }
                                ListElement {
                                    text: qsTr("Indicate"); value: "indicate"
                                }
                            }
                            textRole: "text"
                            valueRole: "value"
                            Layout.fillWidth: true
                        }

                        Label {
                            id: bluetoothStatusLabel
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            Layout.columnSpan: 2; Layout.fillWidth: true
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
                currentIndex: {
                    if ([0, 1, 2, 3, 4, 5, 6, 7, 8, 10].includes(rootItem.portType)) {
                        return 0
                    } else {
                        return 1
                    }
                }
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
                        id: logFormatComboBox
                        model: ListModel {
                            ListElement {
                                text: qsTr("raw"); value: "raw"
                            }
                            ListElement {
                                text: qsTr("hex"); value: "hex"
                            }
                            ListElement {
                                text: qsTr("ascii"); value: "ascii"
                            }
                            ListElement {
                                text: qsTr("utf-8"); value: "utf-8"
                            }
                        }
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
                        id: txSuffixComboBox
                        model: ListModel {
                            ListElement {
                                text: qsTr("null"); value: "null"
                            }
                            ListElement {
                                text: qsTr("crlf"); value: "crlf"
                            }
                            ListElement {
                                text: qsTr("modbus crc"); value: "modbus crc"
                            }
                            ListElement {
                                text: qsTr("modbus lrc"); value: "modbus lrc"
                            }
                        }
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
                        id: bufferSizeSpinBox
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
                            color: global.stroke
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
                                policy: ScrollBar.AsNeeded
                                palette {
                                    mid: global.stroke
                                    dark: global.strokePressed
                                }
                            }
                            ScrollBar.horizontal: ScrollBar {
                                policy: ScrollBar.AsNeeded
                                palette {
                                    mid: global.stroke
                                    dark: global.strokePressed
                                }
                            }

                            VideoOutput {
                                id: videoOutput
                                property var indicatorList: []
                            }

                            Rectangle {
                                id: roiSelection
                                anchors.fill: videoOutput
                                color: global.stroke
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
                                            mainToolTip.text = ""
                                        }
                                    }
                                    onPointChanged: {
                                        mainToolTip.position = parent.mapToGlobal(point.position)
                                        mainToolTip.text = qsTr("%1, %2\nSelect %3/%4  Esc: Exit")
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
                                            portSetting.roiInsert([ix, iy, iw, ih])
                                        } else if (step === 4) {
                                            let ix0 = Math.round(roiList[0].x)
                                            let iy0 = Math.round(roiList[0].y)
                                            let ix1 = Math.round(roiList[1].x)
                                            let iy1 = Math.round(roiList[1].y)
                                            let ix2 = Math.round(roiList[2].x)
                                            let iy2 = Math.round(roiList[2].y)
                                            let ix3 = Math.round(roiList[3].x)
                                            let iy3 = Math.round(roiList[3].y)
                                            portSetting.roiInsert([ix0, iy0, ix1, iy1, ix2, iy2, ix3, iy3])
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
                                    mainToolTip.text = ""
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
                                            mainToolTip.text = ""
                                        }
                                    }
                                    onPointChanged: {
                                        mainToolTip.position = parent.mapToGlobal(point.position)
                                        mainToolTip.text = qsTr("Rectangular")
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
                                            mainToolTip.text = ""
                                        }
                                    }
                                    onPointChanged: {
                                        mainToolTip.position = parent.mapToGlobal(point.position)
                                        mainToolTip.text = qsTr("Quadrilateral")
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
                                color: global.stroke
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
                                    visible: roiModel.empty

                                    RowLayout {
                                        anchors.centerIn: parent

                                        Label {
                                            text: qsTr("Create ROI first.")
                                            font.pixelSize: 16
                                            Layout.alignment: Qt.AlignVCenter
                                        }

                                        IconImage {
                                            source: "qrc:/icon/rectangle.svg"
                                            color: global.fore
                                            Layout.alignment: Qt.AlignVCenter
                                        }
                                    }
                                }

                                Item {
                                    anchors.fill: parent
                                    visible: !roiModel.empty

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
                                                color: global.back

                                                IconImage {
                                                    anchors.centerIn: parent
                                                    width: 16; height: 16
                                                    color: global.fore
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
                                            color: global.stroke
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
                                                portSetting.roiSwap(move.oldVisualIndex, move.newVisualIndex)
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
                                        model: roiModel
                                        contentWidth: width
                                        property int hoveredRow: -1
                                        property int selectedRow: -1
                                        onSelectedRowChanged: previewLoader.start()

                                        ScrollBar.vertical: ScrollBar {
                                            policy: ScrollBar.AsNeeded
                                            palette {
                                                mid: global.stroke
                                                dark: global.strokePressed
                                            }
                                        }

                                        Rectangle {
                                            anchors.fill: parent
                                            color: global.stroke
                                        }

                                        delegate: Item {
                                            implicitWidth: parent.width; implicitHeight: 24
                                            required property int row

                                            Rectangle {
                                                anchors.fill: parent
                                                color: global.back
                                            }

                                            Rectangle {
                                                anchors.fill: parent
                                                radius: 6
                                                color: global.backHover
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
                                                color: roiTableView.selectedRow === row ? global.backSelected : "transparent"
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

                                            onTriggered: portSetting.roiRemove(roiMenu.roiIndex)
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
                                                mainToolTip.text = ""
                                            }
                                        }
                                        onPointChanged: {
                                            mainToolTip.position = parent.mapToGlobal(point.position)
                                            mainToolTip.text = qsTr("Add Pipeline")
                                        }
                                    }
                                }
                            }

                            Item {
                                visible: pipelineLayout.expanded
                                Layout.fillWidth: true; Layout.fillHeight: true

                                Item {
                                    anchors.fill: parent
                                    visible: pipelineModel.empty

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
                                    visible: !pipelineModel.empty

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
                                                color: global.back

                                                IconImage {
                                                    anchors.centerIn: parent
                                                    width: 16; height: 16
                                                    color: global.fore
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
                                            color: global.stroke
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
                                                portSetting.pipelineSwap(move.oldVisualIndex, move.newVisualIndex)
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
                                        model: pipelineModel
                                        contentWidth: width

                                        ScrollBar.vertical: ScrollBar {
                                            policy: ScrollBar.AsNeeded
                                            palette {
                                                mid: global.stroke
                                                dark: global.strokePressed
                                            }
                                        }

                                        Rectangle {
                                            anchors.fill: parent
                                            color: global.stroke
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
                                                    color: global.back
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
                                                            model: ListModel {
                                                                ListElement {
                                                                    text: qsTr("Nearest")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("Linear")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("Cubic")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("Area")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("Lanczos4")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("Linear Exact")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("Nearest Exact")
                                                                }
                                                            }
                                                            textRole: "text"
                                                            property bool initialized: false

                                                            Component.onCompleted: {
                                                                initialized = true
                                                            }

                                                            onCurrentTextChanged: {
                                                                if (!scaleComboBox.initialized) return
                                                                scaleItem.session.interpolation = scaleComboBox.currentIndex
                                                                const index = pipelineModel.index(row, 0);
                                                                pipelineModel.setData(index, scaleItem.session, Qt.WhatsThisRole)
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
                                                            const index = pipelineModel.index(row, 0);
                                                            pipelineModel.setData(index, scaleItem.session, Qt.WhatsThisRole)
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
                                                    color: global.back
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
                                                            model: ListModel {
                                                                ListElement {
                                                                    text: qsTr("Binary")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("Binary Inv")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("Trunc")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("To Zero")
                                                                }
                                                                ListElement {
                                                                    text: qsTr("To Zero Inv")
                                                                }
                                                            }
                                                            textRole: "text"
                                                            property bool initialized: false

                                                            Component.onCompleted: {
                                                                initialized = true
                                                            }

                                                            onCurrentTextChanged: {
                                                                if (!thresholdComboBox.initialized) return
                                                                thresholdItem.session.mode = thresholdComboBox.currentIndex
                                                                const index = pipelineModel.index(row, 0);
                                                                pipelineModel.setData(index, thresholdItem.session, Qt.WhatsThisRole)
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
                                                            const index = pipelineModel.index(row, 0);
                                                            pipelineModel.setData(index, thresholdItem.session, Qt.WhatsThisRole)
                                                        }

                                                        second.onMoved: {
                                                            thresholdItem.session.maxval = Math.round(thresholdSlider.second.value)
                                                            const index = pipelineModel.index(row, 0);
                                                            pipelineModel.setData(index, thresholdItem.session, Qt.WhatsThisRole)
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

                                            onTriggered: portSetting.pipelineRemove(pipelineMenu.pipelineIndex)
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
                                    model: ListModel {
                                        ListElement {
                                            text: qsTr("OCR")
                                        }
                                        ListElement {
                                            text: qsTr("Corner ShiTomasi")
                                        }
                                        ListElement {
                                            text: qsTr("Corner Harris")
                                        }
                                        ListElement {
                                            text: qsTr("Template Match")
                                        }
                                    }
                                    textRole: "text"
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

                                // Corner ShiTomasi
                                ColumnLayout {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                }

                                // Corner Harris
                                ColumnLayout {
                                    Layout.fillWidth: true; Layout.fillHeight: true
                                }

                                // Template Match
                                ColumnLayout {
                                    Layout.fillWidth: true; Layout.fillHeight: true

                                    RowLayout {
                                        Layout.fillWidth: true

                                        TextField {
                                            id: templateTextField
                                            Layout.fillWidth: true
                                        }

                                        Button {
                                            leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
                                            icon.source: "qrc:/icon/folder.svg"
                                            icon.width: 16; icon.height: 16
                                            Layout.preferredWidth: 32; Layout.preferredHeight: 32

                                            onClicked: {
                                                const url = templateTextField.text
                                                fileDialog.selectedFile = url
                                                fileDialog.currentFolder = url.substring(0, url.lastIndexOf('/'))
                                                fileDialog.open()
                                            }
                                        }

                                        FileDialog {
                                            id: fileDialog
                                            fileMode: FileDialog.OpenFile
                                            nameFilters: ["Image files (*.png)"]
                                            onAccepted: templateTextField.text = selectedFile
                                        }
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
                                    policy: ScrollBar.AsNeeded
                                    palette {
                                        mid: global.stroke
                                        dark: global.strokePressed
                                    }
                                }
                                ScrollBar.horizontal: ScrollBar {
                                    policy: ScrollBar.AsNeeded
                                    palette {
                                        mid: global.stroke
                                        dark: global.strokePressed
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
                                                border.color: global.dangerFore3

                                                Label {
                                                    width: parent.width
                                                    leftPadding: 2; rightPadding: 2
                                                    color: global.fore
                                                    font.pixelSize: 11
                                                    text: recognitionDelegate.modelData.text + "\n" + Number(recognitionDelegate.modelData.confidence).toFixed(1) + "%"
                                                    elide: Text.ElideRight
                                                    background: Rectangle {
                                                        color: global.back
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
                                                color: global.dangerFore3
                                            }

                                            Rectangle {
                                                x: -1
                                                y: -8
                                                width: 2
                                                height: 16
                                                visible: recognitionDelegate.point
                                                color: global.dangerFore3
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
                                        if (!rootItem.Window.window.visible || roiModel.empty || roiTableView.selectedRow === -1) {
                                            stop()
                                            return
                                        }
                                        portSetting.previewLoad(roiTableView.selectedRow)
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
                    portSetting.dialogResize(600, 500)
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

                onClicked: {
                    if (swipeView.currentIndex === 0) {
                        swipeView.currentIndex = swipeView.currentIndex + 1
                    } else if (swipeView.currentIndex === 1) {
                        switch (rootItem.portType) {
                            case 0: {
                                if (!serialPortNameComboBox.currentText) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                            }
                                break
                            case 1: {
                                if (!visaNameComboBox.currentText) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                            }
                                break
                            case 2: {
                                if (!tcpClientNameTextField.text) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                if (!tcpClientRemoteHostTextField.text) {
                                    portNameValidator.text = qsTr("Empty Remote Host")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                // else if (!(rootItem.patterns["ipv4"].test(tcpClientRemoteHostTextField.text)
                                //     || rootItem.patterns["ipv6"].test(tcpClientRemoteHostTextField.text))) {
                                //     portNameValidator.text = qsTr("Invalid Remote Host")
                                //     portNameValidatorTimer.start()
                                //     return
                                // }
                            }
                                break
                            case 3: {
                                if (!tcpServerNameTextField.text) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                if (!tcpServerLocalHostComboBox.currentText) {
                                    portNameValidator.text = qsTr("Invalid Local Host")
                                    portNameValidatorTimer.start()
                                    return
                                }
                            }
                                break
                            case 4: {
                                if (!sslClientNameTextField.text) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                if (!sslClientRemoteHostTextField.text) {
                                    portNameValidator.text = qsTr("Empty Remote Host")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                // else if (!(rootItem.patterns["ipv4"].test(sslClientRemoteHostTextField.text)
                                //     || rootItem.patterns["ipv6"].test(sslClientRemoteHostTextField.text))) {
                                //     portNameValidator.text = qsTr("Invalid Remote Host")
                                //     portNameValidatorTimer.start()
                                //     return
                                // }
                            }
                                break
                            case 5: {
                                if (!sslServerNameTextField.text) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                if (!sslServerLocalHostComboBox.currentText) {
                                    portNameValidator.text = qsTr("Invalid Local Host")
                                    portNameValidatorTimer.start()
                                    return
                                }
                            }
                                break
                            case 6: {
                                if (!webSocketClientNameTextField.text) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                if (!webSocketClientUrlTextField.text) {
                                    portNameValidator.text = qsTr("Empty URL")
                                    portNameValidatorTimer.start()
                                    return
                                }
                            }
                                break
                            case 7: {
                                if (!webSocketServerNameTextField.text) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                if (!webSocketServerLocalHostComboBox.currentText) {
                                    portNameValidator.text = qsTr("Invalid Local Host")
                                    portNameValidatorTimer.start()
                                    return
                                }
                            }
                                break
                            case 8: {
                                if (!udpSocketNameTextField.text) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                if (!udpSocketLocalHostComboBox.currentText) {
                                    portNameValidator.text = qsTr("Invalid Local Host")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                if (!udpSocketRemoteHostTextField.text) {
                                    portNameValidator.text = qsTr("Invalid Remote Host")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                // else if (!(rootItem.patterns["ipv4"].test(udpSocketRemoteHostTextField.text)
                                //     || rootItem.patterns["ipv6"].test(udpSocketRemoteHostTextField.text))) {
                                //     portNameValidator.text = qsTr("Invalid Remote Host")
                                //     portNameValidatorTimer.start()
                                //     return
                                // }
                            }
                                break
                            case 9: {
                                portSetting.videoCapture()
                            }
                                break
                            case 10: {
                                if (!bluetoothNameTextField.text) {
                                    portNameValidator.text = qsTr("Invalid Port Name")
                                    portNameValidatorTimer.start()
                                    return
                                }
                                if (bluetoothAdapterComboBox.currentIndex < 0
                                    || bluetoothPeripheralComboBox.currentIndex < 0
                                    || bluetoothServiceComboBox.currentIndex < 0
                                    || bluetoothTxCharacteristicComboBox.currentIndex < 0
                                    || bluetoothRxCharacteristicComboBox.currentIndex < 0) {
                                    portNameValidator.text = qsTr("Incomplete Bluetooth Configuration")
                                    portNameValidatorTimer.start()
                                    return
                                }
                            }
                                break
                        }
                        swipeView.currentIndex = swipeView.currentIndex + 1
                    } else {
                        portSetting.portSettingExport()
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
                session.type = 0
                session.ratio = 1.0
                session.interpolation = 1
                portSetting.pipelineInsert(session)
            }
        }

        MenuItem {
            text: qsTr("Threshold")
            icon.source: "qrc:/icon/contrast.svg"
            icon.width: 16; icon.height: 16

            onTriggered: {
                let session = {}
                session.type = 1
                session.thresh = 0
                session.maxval = 255
                session.mode = 0
                portSetting.pipelineInsert(session)
            }
        }
    }

    // indicator
    Component {
        id: indicatorRectComponent

        Rectangle {
            id: indicatorRect
            color: roiTableView.selectedRow === index ? Qt.alpha(global.brandBackSelected, 0.3) : "transparent"
            border.color: global.brandStroke
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
                color: global.brandBackSelected
                border.color: global.brandStroke
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
                color: global.brandBackSelected
                border.color: global.brandStroke
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
                const modelIndex = roiModel.index(index, 0)
                roiModel.setData(modelIndex, [ix, iy, iw, ih], Qt.WhatsThisRole)
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
                fillColor: roiTableView.selectedRow === index ? Qt.alpha(global.brandBackSelected, 0.3) : "transparent"
                strokeColor: global.brandStroke
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
                color: global.brandBackSelected
                border.color: global.brandStroke
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
                color: global.brandBackSelected
                border.color: global.brandStroke
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
                color: global.brandBackSelected
                border.color: global.brandStroke
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
                color: global.brandBackSelected
                border.color: global.brandStroke
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
                const modelIndex = roiModel.index(index, 0)
                roiModel.setData(modelIndex, [
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
        for (let row = 0; row < roiModel.rowCount(); ++row) {
            const index = roiModel.index(row, 0)
            const roi = roiModel.data(index, Qt.WhatsThisRole)
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

    Component.onCompleted: {
        const objects = {
            "swipeView": swipeView,
            "tumbler": tumbler,
            // serial port
            "serialPortNameComboBox": serialPortNameComboBox,
            "serialPortBaudRateSpinBox": serialPortBaudRateSpinBox,
            "serialPortDataBitsComboBox": serialPortDataBitsComboBox,
            "serialPortParityComboBox": serialPortParityComboBox,
            "serialPortStopBitsComboBox": serialPortStopBitsComboBox,
            // visa
            "visaNameComboBox": visaNameComboBox,
            // tcp client
            "tcpClientNameTextField": tcpClientNameTextField,
            "tcpClientRemoteHostTextField": tcpClientRemoteHostTextField,
            "tcpClientRemotePortSpinBox": tcpClientRemotePortSpinBox,
            // tcp server
            "tcpServerNameTextField": tcpServerNameTextField,
            "tcpServerLocalHostComboBox": tcpServerLocalHostComboBox,
            "tcpServerLocalPortSpinBox": tcpServerLocalPortSpinBox,
            // ssl client
            "sslClientNameTextField": sslClientNameTextField,
            "sslClientRemoteHostTextField": sslClientRemoteHostTextField,
            "sslClientRemotePortSpinBox": sslClientRemotePortSpinBox,
            // ssl server
            "sslServerNameTextField": sslServerNameTextField,
            "sslServerLocalHostComboBox": sslServerLocalHostComboBox,
            "sslServerLocalPortSpinBox": sslServerLocalPortSpinBox,
            "sslServerCertificateTextField": sslServerCertificateTextField,
            "sslServerPrivateKeyTextField": sslServerPrivateKeyTextField,
            // web socket client
            "webSocketClientNameTextField": webSocketClientNameTextField,
            "webSocketClientUrlTextField": webSocketClientUrlTextField,
            "webSocketClientMessageTypeComboBox": webSocketClientMessageTypeComboBox,
            // web socket server
            "webSocketServerNameTextField": webSocketServerNameTextField,
            "webSocketServerLocalHostComboBox": webSocketServerLocalHostComboBox,
            "webSocketServerLocalPortSpinBox": webSocketServerLocalPortSpinBox,
            "webSocketServerSecureSwitch": webSocketServerSecureSwitch,
            "webSocketServerCertificateTextField": webSocketServerCertificateTextField,
            "webSocketServerPrivateKeyTextField": webSocketServerPrivateKeyTextField,
            "webSocketServerMessageTypeComboBox": webSocketServerMessageTypeComboBox,
            // udp socket
            "udpSocketNameTextField": udpSocketNameTextField,
            "udpSocketLocalHostComboBox": udpSocketLocalHostComboBox,
            "udpSocketLocalPortSpinBox": udpSocketLocalPortSpinBox,
            "udpSocketRemoteHostTextField": udpSocketRemoteHostTextField,
            "udpSocketRemotePortSpinBox": udpSocketRemotePortSpinBox,
            // video stream
            "videoStreamNameComboBox": videoStreamNameComboBox,
            // bluetooth le
            "bluetoothNameTextField": bluetoothNameTextField,
            "bluetoothAdapterComboBox": bluetoothAdapterComboBox,
            "bluetoothPeripheralComboBox": bluetoothPeripheralComboBox,
            "bluetoothServiceComboBox": bluetoothServiceComboBox,
            "bluetoothTxCharacteristicComboBox": bluetoothTxCharacteristicComboBox,
            "bluetoothRxCharacteristicComboBox": bluetoothRxCharacteristicComboBox,
            "bluetoothWriteTypeComboBox": bluetoothWriteTypeComboBox,
            "bluetoothSubscribeTypeComboBox": bluetoothSubscribeTypeComboBox,
            "bluetoothStatusLabel": bluetoothStatusLabel,
            // format
            "logFormatComboBox": logFormatComboBox,
            "txSuffixComboBox": txSuffixComboBox,
            "bufferSizeSpinBox": bufferSizeSpinBox,
            // image
            "videoSink": videoOutput.videoSink,
            "previewImage": previewImage,
            "recognitionComboBox": recognitionComboBox,
            "templateTextField": templateTextField
        };
        portSetting.propertyGet(objects)
    }
}
