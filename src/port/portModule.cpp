#include "port/portModule.h"

#include <QJsonArray>
#include <QQuickWidget>
#include <QRegularExpression>
#include <QUrl>

#include "globals.h"
#include "core/globalManager.h"
#include "core/undoModule.h"
#include "port/basePort.h"
#include "port/bluetoothLe.h"
#include "port/module/deviceDiscovery.h"
#include "port/portManageWindow.h"
#include "port/serialPort.h"
#include "port/tcpClient.h"
#include "port/tcpServer.h"
#include "port/sslClient.h"
#include "port/sslServer.h"
#include "port/udpSocket.h"
#include "port/vision.h"
#include "port/webSocketClient.h"
#include "port/webSocketServer.h"

// public
PortModule::PortModule()
    : DockWidget("Port"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_portManageWindow(std::make_unique<PortManageWindow>()) {
    setWidget(m_widget);
    connect(m_portManageWindow.get(), &PortManageWindow::insertPort, this, &PortModule::portInsert);
    connect(m_portManageWindow.get(), &PortManageWindow::editPort, this, &PortModule::portEdit);
    for (const auto &value: g_workspaceConfig["portConfig"].toArray()) {
        auto portConfig = value.toObject();
        portDefaults(portConfig);
        if (!portCheck(portConfig).isEmpty()) continue;
        _portInsert(m_ports.size(), portConfig);
    }
}

PortModule::~PortModule() {
    for (const auto &port: m_portHash) {
        auto *thread = port->thread();
        thread->quit();
        thread->wait();
    }
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void PortModule::propertySet() {
    m_widget->setInitialProperties({{"portModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/port/portModule.qml"));

    m_portManageWindow->propertySet();
}

void PortModule::portConfigSave() {
    QJsonArray portConfigArray{};
    for (const auto &port: m_ports) {
        const QString portName = port.toHash().value("name").toString();
        QJsonObject portConfig = m_portHash[portName]->config();
        portConfigArray.append(portConfig);
    }
    g_workspaceConfig["portConfig"] = portConfigArray;
}

QSet<QString> PortModule::portList() const {
    QSet<QString> keys{};
    for (const auto &portName: m_portHash.keys()) {
        keys.insert(portName);
    }
    return keys;
}

QJsonObject PortModule::portConfigGet(const PortType::Value portType) {
    QJsonObject portConfig{};
    switch (portType) {
        case PortType::SerialPort: {
            QJsonArray serialPorts{};
            for (const auto &port: DeviceDiscovery::serialPorts()) serialPorts.append(port.toHash().value("deviceName").toString());
            portConfig = {
                {"type", "object"},
                {"required", QJsonArray{"portName", "deviceName"}},
                {
                    "defaults", QJsonObject{
                        {"baudRate", 115200},
                        {"dataBits", 8},
                        {"parity", 0},
                        {"stopBits", 1},
                        {"logFormat", "utf-8"},
                        {"txSuffix", "null"},
                        {"bufferSize", 65536}
                    }
                },
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {"deviceName", QJsonObject{{"type", "string"}, {"enum", serialPorts}}},
                        {"baudRate", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 5000000}}},
                        {"dataBits", QJsonObject{{"enum", QJsonArray{5, 6, 7, 8}}}},
                        {"parity", QJsonObject{{"enum", QJsonArray{0, 2, 3, 4, 5}}, {"description", "0=no, 2=even, 3=odd, 4=space, 5=mark"}}},
                        {"stopBits", QJsonObject{{"enum", QJsonArray{1, 2, 3}}, {"description", "1=one, 2=two, 3=one and a half"}}},
                        {"logFormat", QJsonObject{{"enum", QJsonArray{"raw", "hex", "ascii", "utf-8"}}}},
                        {"txSuffix", QJsonObject{{"enum", QJsonArray{"null", "crlf", "modbus crc", "modbus lrc"}}}},
                        {"bufferSize", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 67108864}}}
                    }
                }
            };
            break;
        }
        case PortType::Vision: {
            auto devices = DeviceDiscovery::screens();
            devices.append(DeviceDiscovery::cameras());
            devices.removeDuplicates();
            portConfig = {
                {"type", "object"},
                {"required", QJsonArray{"portName", "deviceName"}},
                {"defaults", QJsonObject{
                    {"rois", QJsonArray{}},
                    {"pipeline", QJsonArray{}},
                    {"recognition", QJsonObject{{"mode", RecognitionMethod::OCR}}}
                }},
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {
                            "deviceName", QJsonObject{
                                {"type", "string"},
                                {"enum", QJsonArray::fromStringList(devices)},
                                {"description", "The exact name of an available screen or camera."}
                            }
                        }
                    }
                }
            };
            break;
        }
        case PortType::TcpClient: {
            portConfig = {
                {"type", "object"},
                {
                    "required", QJsonArray{
                        "portName",
                        "remoteHost",
                        "remotePort"
                    }
                },
                {
                    "defaults", QJsonObject{
                        {"logFormat", "utf-8"},
                        {"txSuffix", "null"},
                        {"bufferSize", 65536}
                    }
                },
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {"remoteHost", QJsonObject{{"type", "string"}}},
                        {"remotePort", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 65535}}},
                        {"logFormat", QJsonObject{{"enum", QJsonArray{"raw", "hex", "ascii", "utf-8"}}}},
                        {"txSuffix", QJsonObject{{"enum", QJsonArray{"null", "crlf", "modbus crc", "modbus lrc"}}}},
                        {"bufferSize", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 67108864}}}
                    }
                }
            };
            break;
        }
        case PortType::TcpServer: {
            portConfig = {
                {"type", "object"},
                {
                    "required", QJsonArray{
                        "portName",
                        "localHost",
                        "localPort"
                    }
                },
                {
                    "defaults", QJsonObject{
                        {"logFormat", "utf-8"},
                        {"txSuffix", "null"},
                        {"bufferSize", 65536}
                    }
                },
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {"localHost", QJsonObject{{"type", "string"}}},
                        {"localPort", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 65535}}},
                        {"logFormat", QJsonObject{{"enum", QJsonArray{"raw", "hex", "ascii", "utf-8"}}}},
                        {"txSuffix", QJsonObject{{"enum", QJsonArray{"null", "crlf", "modbus crc", "modbus lrc"}}}},
                        {"bufferSize", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 67108864}}}
                    }
                }
            };
            break;
        }
        case PortType::SslClient: {
            portConfig = {
                {"type", "object"},
                {
                    "required", QJsonArray{
                        "portName",
                        "remoteHost",
                        "remotePort"
                    }
                },
                {
                    "defaults", QJsonObject{
                        {"secure", false},
                        {"caCertificate", ""},
                        {"certificate", ""},
                        {"privateKey", ""},
                        {"privateKeyPassword", ""},
                        {"logFormat", "utf-8"},
                        {"txSuffix", "null"},
                        {"bufferSize", 65536}
                    }
                },
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {"remoteHost", QJsonObject{{"type", "string"}}},
                        {"remotePort", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 65535}}},
                        {"secure", QJsonObject{{"type", "boolean"}, {"description", "Verify the server certificate and enable the optional certificate settings. False skips verification and ignores those settings; SSL always uses TLS."}}},
                        {"caCertificate", QJsonObject{{"type", "string"}, {"description", "Optional file:// URL of a PEM CA bundle added to the default trust store when secure is true."}}},
                        {"certificate", QJsonObject{{"type", "string"}, {"description", "Optional file:// URL of a PEM client certificate chain, leaf first. Requires privateKey and secure=true."}}},
                        {"privateKey", QJsonObject{{"type", "string"}, {"description", "Optional file:// URL of the matching PEM private key. Requires certificate and secure=true."}}},
                        {"privateKeyPassword", QJsonObject{{"type", "string"}, {"description", "Optional private key passphrase, saved in plaintext. Used only when secure is true."}}},
                        {"logFormat", QJsonObject{{"enum", QJsonArray{"raw", "hex", "ascii", "utf-8"}}}},
                        {"txSuffix", QJsonObject{{"enum", QJsonArray{"null", "crlf", "modbus crc", "modbus lrc"}}}},
                        {"bufferSize", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 67108864}}}
                    }
                }
            };
            break;
        }
        case PortType::SslServer: {
            portConfig = {
                {"type", "object"},
                {
                    "required", QJsonArray{
                        "portName",
                        "localHost",
                        "localPort",
                        "certificate",
                        "privateKey"
                    }
                },
                {
                    "defaults", QJsonObject{
                        {"caCertificate", ""},
                        {"privateKeyPassword", ""},
                        {"logFormat", "utf-8"},
                        {"txSuffix", "null"},
                        {"bufferSize", 65536}
                    }
                },
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {"localHost", QJsonObject{{"type", "string"}}},
                        {"localPort", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 65535}}},
                        {"certificate", QJsonObject{{"type", "string"}, {"description", "Required file:// URL of a PEM server certificate chain, leaf first. SSL Server always uses TLS."}}},
                        {"privateKey", QJsonObject{{"type", "string"}, {"description", "Required file:// URL of the matching PEM private key."}}},
                        {"privateKeyPassword", QJsonObject{{"type", "string"}, {"description", "Optional private key passphrase, saved in plaintext."}}},
                        {"caCertificate", QJsonObject{{"type", "string"}, {"description", "Optional file:// URL of a PEM CA bundle. Empty does not request client certificates; nonempty requires valid client certificates trusted by this bundle only, without the default trust store."}}},
                        {"logFormat", QJsonObject{{"enum", QJsonArray{"raw", "hex", "ascii", "utf-8"}}}},
                        {"txSuffix", QJsonObject{{"enum", QJsonArray{"null", "crlf", "modbus crc", "modbus lrc"}}}},
                        {"bufferSize", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 67108864}}}
                    }
                }
            };
            break;
        }
        case PortType::WebSocketClient: {
            portConfig = {
                {"type", "object"},
                {"required", QJsonArray{"portName", "remoteHost", "remotePort"}},
                {
                    "defaults", QJsonObject{
                        {"secure", false},
                        {"caCertificate", ""},
                        {"certificate", ""},
                        {"privateKey", ""},
                        {"privateKeyPassword", ""},
                        {"logFormat", "utf-8"},
                        {"txSuffix", "null"},
                        {"bufferSize", 65536}
                    }
                },
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {"remoteHost", QJsonObject{{"type", "string"}, {"description", "Hostname or IP address, optionally followed by path and query, without ws:// or wss://; for example ws.postman-echo.com/raw."}}},
                        {"remotePort", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 65535}}},
                        {"secure", QJsonObject{{"type", "boolean"}, {"description", "Use WSS and verify the server certificate. False uses plaintext WS and ignores certificate settings."}}},
                        {"caCertificate", QJsonObject{{"type", "string"}, {"description", "Optional file:// URL of a PEM CA bundle added to the default trust store when secure is true."}}},
                        {"certificate", QJsonObject{{"type", "string"}, {"description", "Optional file:// URL of a PEM client certificate chain, leaf first. Requires privateKey and secure=true."}}},
                        {"privateKey", QJsonObject{{"type", "string"}, {"description", "Optional file:// URL of the matching PEM private key. Requires certificate and secure=true."}}},
                        {"privateKeyPassword", QJsonObject{{"type", "string"}, {"description", "Optional private key passphrase, saved in plaintext. Used only when secure is true."}}},
                        {"logFormat", QJsonObject{{"enum", QJsonArray{"raw", "hex", "ascii", "utf-8"}}}},
                        {"txSuffix", QJsonObject{{"enum", QJsonArray{"null", "crlf", "modbus crc", "modbus lrc"}}}},
                        {"bufferSize", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 67108864}}}
                    }
                }
            };
            break;
        }
        case PortType::WebSocketServer: {
            portConfig = {
                {"type", "object"},
                {"required", QJsonArray{"portName", "localHost", "localPort"}},
                {
                    "defaults", QJsonObject{
                        {"secure", false},
                        {"certificate", ""},
                        {"privateKey", ""},
                        {"caCertificate", ""},
                        {"privateKeyPassword", ""},
                        {"logFormat", "utf-8"},
                        {"txSuffix", "null"},
                        {"bufferSize", 65536}
                    }
                },
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {"localHost", QJsonObject{{"type", "string"}}},
                        {"localPort", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 65535}}},
                        {"secure", QJsonObject{{"type", "boolean"}, {"description", "Use WSS with the supplied server identity. False uses plaintext WS and ignores certificate settings."}}},
                        {"certificate", QJsonObject{{"type", "string"}, {"description", "Required when secure=true: file:// URL of a PEM server certificate chain, leaf first."}}},
                        {"privateKey", QJsonObject{{"type", "string"}, {"description", "Required when secure=true: file:// URL of the matching PEM private key."}}},
                        {"privateKeyPassword", QJsonObject{{"type", "string"}, {"description", "Optional private key passphrase, saved in plaintext. Used only when secure is true."}}},
                        {"caCertificate", QJsonObject{{"type", "string"}, {"description", "Optional file:// URL of a PEM CA bundle. Empty does not request client certificates; nonempty requires valid client certificates trusted by this bundle only, without the default trust store. Used only when secure is true."}}},
                        {"logFormat", QJsonObject{{"enum", QJsonArray{"raw", "hex", "ascii", "utf-8"}}}},
                        {"txSuffix", QJsonObject{{"enum", QJsonArray{"null", "crlf", "modbus crc", "modbus lrc"}}}},
                        {"bufferSize", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 67108864}}}
                    }
                }
            };
            break;
        }
        case PortType::UdpSocket: {
            portConfig = {
                {"type", "object"},
                {"required", QJsonArray{"portName", "localHost", "localPort", "remoteHost", "remotePort"}},
                {
                    "defaults", QJsonObject{
                        {"logFormat", "utf-8"},
                        {"txSuffix", "null"},
                        {"bufferSize", 65536}
                    }
                },
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {"localHost", QJsonObject{{"type", "string"}}},
                        {"localPort", QJsonObject{{"type", "integer"}, {"minimum", 0}, {"maximum", 65535}}},
                        {"remoteHost", QJsonObject{{"type", "string"}}},
                        {"remotePort", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 65535}}},
                        {"logFormat", QJsonObject{{"enum", QJsonArray{"raw", "hex", "ascii", "utf-8"}}}},
                        {"txSuffix", QJsonObject{{"enum", QJsonArray{"null", "crlf", "modbus crc", "modbus lrc"}}}},
                        {"bufferSize", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 67108864}}}
                    }
                }
            };
            break;
        }
        case PortType::BluetoothLe: {
            portConfig = {
                {"type", "object"},
                {"required", QJsonArray{"portName", "adapterAddress", "peripheralAddress"}},
                {
                    "defaults", QJsonObject{
                        {"logFormat", "utf-8"},
                        {"txSuffix", "null"},
                        {"bufferSize", 65536}
                    }
                },
                {
                    "properties", QJsonObject{
                        {"portName", QJsonObject{{"type", "string"}}},
                        {"adapterAddress", QJsonObject{{"type", "string"}}},
                        {"peripheralAddress", QJsonObject{{"type", "string"}}},
                        {"logFormat", QJsonObject{{"enum", QJsonArray{"raw", "hex", "ascii", "utf-8"}}}},
                        {"txSuffix", QJsonObject{{"enum", QJsonArray{"null", "crlf", "modbus crc", "modbus lrc"}}}},
                        {"bufferSize", QJsonObject{{"type", "integer"}, {"minimum", 1}, {"maximum", 67108864}}}
                    }
                }
            };
            break;
        }
        default: break;
    }
    return portConfig;
}

QString PortModule::portCheck(const QJsonObject &portConfig, const QString &oldPortName) const {
    const auto portType = static_cast<PortType::Value>(portConfig.value("portType").toInt(-1));
    const auto portName = portConfig.value("portName").toString();
    if (portName.trimmed().isEmpty()) return "Port check failed: portName is empty.";
    if (portName != oldPortName && m_portHash.contains(portName)) return QString("Port check failed: '%1' already exists.").arg(portName);

    switch (portType) {
        case PortType::SerialPort: {
            if (portConfig.value("deviceName").toString().trimmed().isEmpty()) return "Port check failed: deviceName is empty.";

            const auto baudRate = portConfig.value("baudRate").toInt();
            if (baudRate < 1 || baudRate > 5000000) return "Port check failed: baudRate must be between 1 and 5000000.";

            if (!QJsonArray{5, 6, 7, 8}.contains(portConfig.value("dataBits"))) return "Port check failed: invalid dataBits.";
            if (!QJsonArray{0, 2, 3, 4, 5}.contains(portConfig.value("parity"))) return "Port check failed: invalid parity.";
            if (!QJsonArray{1, 2, 3}.contains(portConfig.value("stopBits"))) return "Port check failed: invalid stopBits.";
            break;
        }
        case PortType::Vision: {
            auto devices = DeviceDiscovery::screens();
            devices.append(DeviceDiscovery::cameras());
            if (!devices.contains(portConfig.value("deviceName").toString())) return "Port check failed: vision source is unavailable.";
            return {};
        }
        case PortType::TcpClient: {
            if (portConfig.value("remoteHost").toString().trimmed().isEmpty()) return "Port check failed: remoteHost is empty.";

            const auto remotePort = portConfig.value("remotePort").toInt();
            if (remotePort < 1 || remotePort > 65535) return "Port check failed: remotePort must be between 1 and 65535.";
            break;
        }
        case PortType::TcpServer: {
            if (portConfig.value("localHost").toString().trimmed().isEmpty()) return "Port check failed: localHost is empty.";

            const auto localPort = portConfig.value("localPort").toInt();
            if (localPort < 1 || localPort > 65535) return "Port check failed: localPort must be between 1 and 65535.";
            break;
        }
        case PortType::SslClient: {
            if (portConfig.value("remoteHost").toString().trimmed().isEmpty()) return "Port check failed: remoteHost is empty.";

            const auto remotePort = portConfig.value("remotePort").toInt();
            if (remotePort < 1 || remotePort > 65535) return "Port check failed: remotePort must be between 1 and 65535.";
            break;
        }
        case PortType::SslServer: {
            if (portConfig.value("localHost").toString().trimmed().isEmpty()) return "Port check failed: localHost is empty.";

            const auto localPort = portConfig.value("localPort").toInt();
            if (localPort < 1 || localPort > 65535) return "Port check failed: localPort must be between 1 and 65535.";
            if (portConfig.value("certificate").toString().trimmed().isEmpty()) return "Port check failed: certificate is empty.";
            if (portConfig.value("privateKey").toString().trimmed().isEmpty()) return "Port check failed: privateKey is empty.";
            break;
        }
        case PortType::WebSocketClient: {
            const auto remoteHost = portConfig.value("remoteHost").toString();
            const QUrl url((portConfig.value("secure").toBool() ? "wss://" : "ws://") + remoteHost);
            if (!url.isValid() || url.host().isEmpty() || remoteHost.contains(QRegularExpression("^[A-Za-z][A-Za-z0-9+.-]*://")) || url.hasFragment() || !url.userInfo().isEmpty())
                return "Port check failed: remoteHost must be a host with optional path and query, without scheme, credentials or fragment.";
            const auto remotePort = portConfig.value("remotePort").toInt();
            if (remotePort < 1 || remotePort > 65535) return "Port check failed: remotePort must be between 1 and 65535.";
            break;
        }
        case PortType::WebSocketServer: {
            if (portConfig.value("localHost").toString().trimmed().isEmpty()) return "Port check failed: localHost is empty.";

            const auto localPort = portConfig.value("localPort").toInt();
            if (localPort < 1 || localPort > 65535) return "Port check failed: localPort must be between 1 and 65535.";
            if (portConfig.value("secure").toBool()) {
                if (portConfig.value("certificate").toString().trimmed().isEmpty()) return "Port check failed: certificate is empty.";
                if (portConfig.value("privateKey").toString().trimmed().isEmpty()) return "Port check failed: privateKey is empty.";
            }
            break;
        }
        case PortType::UdpSocket: {
            if (portConfig.value("localHost").toString().trimmed().isEmpty()) return "Port check failed: localHost is empty.";

            const auto localPort = portConfig.value("localPort").toInt(-1);
            if (localPort < 0 || localPort > 65535) return "Port check failed: localPort must be between 0 and 65535.";
            if (portConfig.value("remoteHost").toString().trimmed().isEmpty()) return "Port check failed: remoteHost is empty.";

            const auto remotePort = portConfig.value("remotePort").toInt();
            if (remotePort < 1 || remotePort > 65535) return "Port check failed: remotePort must be between 1 and 65535.";
            break;
        }
        case PortType::BluetoothLe: {
            if (portConfig.value("adapterAddress").toString().trimmed().isEmpty()) return "Port check failed: adapterAddress is empty.";
            if (portConfig.value("peripheralAddress").toString().trimmed().isEmpty()) return "Port check failed: peripheralAddress is empty.";
            break;
        }
        default: return QString("Unsupported port type: %1.").arg(portType);
    }

    const QJsonArray logFormats{"raw", "hex", "ascii", "utf-8"};
    if (!logFormats.contains(portConfig.value("logFormat"))) return "Port check failed: invalid logFormat.";

    const QJsonArray suffixes{"null", "crlf", "modbus crc", "modbus lrc"};
    if (!suffixes.contains(portConfig.value("txSuffix"))) return "Port check failed: invalid txSuffix.";

    const auto bufferSize = portConfig.value("bufferSize").toInt();
    if (bufferSize < 1 || bufferSize > 67108864) return "Port check failed: bufferSize must be between 1 and 67108864.";
    return {};
}

void PortModule::portManage(const int index) const {
    if (index == -1) {
        m_portManageWindow->open();
    } else {
        const auto portName = m_ports.at(index).toHash().value("name").toString();
        const auto &portObject = m_portHash[portName];
        const auto &portConfig = portObject->config();
        m_portManageWindow->open(portConfig);
    }
}

QString PortModule::portInsert(int index, QJsonObject portConfig, const QString &undoGroupId) {
    portDefaults(portConfig);

    auto exception = portCheck(portConfig);
    const auto portName = portConfig.value("portName").toString();
    if (!exception.isEmpty()) {
        emit appendLog(LogType::Error, QString("[%1]").arg(portName), exception);
        return exception;
    }

    if (index == -1) index = m_ports.size();
    if (index < 0 || index > m_ports.size()) {
        const auto error = QString("Port insert failed: invalid index %1.").arg(index);
        emit appendLog(LogType::Error, QString("[%1]").arg(portName), error);
        return error;
    }

    const auto portIndex = QSharedPointer<int>::create(index);
    auto error = g_globalManager->undoModuleGet()->push(
        tr("Port Insert (%1)").arg(portName),
        [this, portIndex, portConfig] {
            if (const auto error = portCheck(portConfig); !error.isEmpty()) return error;
            if (*portIndex < 0 || *portIndex > m_ports.size()) return QString("Port insert failed: invalid index %1.").arg(*portIndex);
            _portInsert(*portIndex, portConfig);
            return QString{};
        },
        [this, portIndex, portName, portConfig] {
            const auto port = m_portHash.value(portName);
            if (port == nullptr) return QString("Port insert undo failed: '%1' does not exist.").arg(portName);
            if (port->config() != portConfig) return QString("Port insert undo failed: '%1' configuration has changed.").arg(portName);
            *portIndex = portIndexGet(portName);
            _portRemove(*portIndex);
            return QString{};
        },
        undoGroupId);
    if (!error.isEmpty()) return error;
    return {};
}

QString PortModule::portRemove(const QString &portName, const QString &undoGroupId) {
    const auto name = portName.trimmed();
    const int index = portIndexGet(name);
    if (index < 0) return QString("Port remove failed: '%1' does not exist.").arg(name);

    const auto portIndex = QSharedPointer<int>::create(index);
    const auto portConfig = m_portHash.value(name)->config();
    const auto error = g_globalManager->undoModuleGet()->push(
        tr("Port Remove (%1)").arg(name),
        [this, portIndex, name, portConfig] {
            const auto port = m_portHash.value(name);
            if (port == nullptr) return QString("Port remove failed: '%1' does not exist.").arg(name);
            if (port->config() != portConfig) return QString("Port remove failed: '%1' configuration has changed.").arg(name);
            *portIndex = portIndexGet(name);
            _portRemove(*portIndex);
            return QString{};
        },
        [this, portIndex, name, portConfig] {
            if (m_portHash.contains(name)) return QString("Port remove undo failed: '%1' already exists.").arg(name);
            if (const auto exception = portCheck(portConfig); !exception.isEmpty()) return exception;
            if (*portIndex < 0 || *portIndex > m_ports.size()) return QString("Port remove undo failed: invalid index %1.").arg(*portIndex);
            _portInsert(*portIndex, portConfig);
            return QString{};
        },
        undoGroupId);
    return error;
}

void PortModule::portMove(const int src, const int dst) {
    if (src < 0 || src >= m_ports.size() || dst < 0 || dst >= m_ports.size()) {
        emit appendLog(LogType::Error, "[Port]", "Port move failed: invalid index.");
        return;
    }
    if (src == dst) return;

    const auto portName = m_ports.at(src).toHash().value("name").toString();
    g_globalManager->undoModuleGet()->push(
        tr("Port Move (%1: %2->%3)").arg(portName).arg(src + 1).arg(dst + 1),
        [this, src, dst] { _portMove(src, dst); },
        [this, src, dst] { _portMove(dst, src); });
}

void PortModule::portEdit(const QString &oldPortName, const QJsonObject &portConfig) {
    const int index = portIndexGet(oldPortName);
    if (index < 0) {
        emit appendLog(LogType::Error, QString("[%1]").arg(oldPortName), "Port edit failed: port does not exist.");
        return;
    }

    auto newPortConfig = portConfig;
    portDefaults(newPortConfig);
    const auto portName = newPortConfig.value("portName").toString();
    if (const auto exception = portCheck(newPortConfig, oldPortName); !exception.isEmpty()) {
        emit appendLog(LogType::Error, QString("[%1]").arg(portName), exception);
        return;
    }

    const auto oldPortConfig = m_portHash[oldPortName]->config();
    if (newPortConfig == oldPortConfig) return;

    const auto commandText = portName == oldPortName
                                 ? tr("Port Edit (%1)").arg(portName)
                                 : tr("Port Rename (%1->%2)").arg(oldPortName, portName);
    g_globalManager->undoModuleGet()->push(
        commandText,
        [this, index, newPortConfig] { _portEdit(index, newPortConfig); },
        [this, index, oldPortConfig] { _portEdit(index, oldPortConfig); });
}

void PortModule::portToggle(const int index) {
    const auto row = m_ports.at(index).toHash();
    const QString portName = row.value("name").toString();
    const bool status = row.value("active").toBool();
    auto port = m_portHash[portName];
    if (status) {
        QMetaObject::invokeMethod(port, [port] {
            port->close();
        }, Qt::QueuedConnection);
    } else {
        QMetaObject::invokeMethod(port, [port] {
            port->open();
        }, Qt::QueuedConnection);
    }
}

void PortModule::portRefresh(const QString &portName, const bool active) {
    const int index = portIndexGet(portName);
    if (index < 0) return;
    auto &row = get<QVariantHash>(m_ports[index]);
    if (row.value("active").toBool() == active) return;
    row.insert("active", active);
    emit portsChanged();
}

// private
void PortModule::portDefaults(QJsonObject &portConfig) {
    const auto defaults = portConfigGet(static_cast<PortType::Value>(portConfig.value("portType").toInt(-1))).value("defaults").toObject();
    for (auto iterator = defaults.constBegin(); iterator != defaults.constEnd(); ++iterator) {
        if (!portConfig.contains(iterator.key())) portConfig.insert(iterator.key(), iterator.value());
    }
}

int PortModule::portIndexGet(const QString &portName) const {
    for (int i = 0; i < m_ports.size(); ++i) {
        if (m_ports.at(i).toHash().value("name").toString() == portName) return i;
    }
    return -1;
}

void PortModule::_portInsert(const int index, const QJsonObject &portConfig) {
    const auto portName = portConfig.value("portName").toString();
    BasePort *port{};
    switch (static_cast<PortType::Value>(portConfig["portType"].toInt())) {
        case PortType::SerialPort: {
            port = new SerialPort(portConfig);
            break;
        }
        case PortType::Vision: {
            port = new Vision(portConfig);
            break;
        }
        case PortType::TcpClient: {
            port = new TcpClient(portConfig);
            break;
        }
        case PortType::TcpServer: {
            port = new TcpServer(portConfig);
            break;
        }
        case PortType::SslClient: {
            port = new SslClient(portConfig);
            break;
        }
        case PortType::SslServer: {
            port = new SslServer(portConfig);
            break;
        }
        case PortType::WebSocketClient: {
            port = new WebSocketClient(portConfig);
            break;
        }
        case PortType::WebSocketServer: {
            port = new WebSocketServer(portConfig);
            break;
        }
        case PortType::UdpSocket: {
            port = new UdpSocket(portConfig);
            break;
        }
        case PortType::BluetoothLe: {
            port = new BluetoothLe(portConfig);
            break;
        }
        default: break;
    }
    connect(port, &BasePort::appendLog, this, &PortModule::appendLog);
    connect(port, &BasePort::refreshPort, this, &PortModule::portRefresh);
    m_portHash.insert(portName, port);
    m_ports.insert(index, QVariantHash{{"name", portName}, {"active", false}});
    emit portsChanged();
    emit appendLog(LogType::Info, QString("[%1]").arg(portName), "initialized");
}

void PortModule::_portRemove(const int index) {
    const QString portName = m_ports.takeAt(index).toHash().value("name").toString();
    auto *port = m_portHash.take(portName);
    emit portsChanged();
    auto *thread = port->thread();
    thread->quit();
    thread->wait();
    emit appendLog(LogType::Info, QString("[%1]").arg(portName), "removed");
}

void PortModule::_portEdit(const int index, const QJsonObject &portConfig) {
    _portRemove(index);
    _portInsert(index, portConfig);
}

void PortModule::_portMove(const int src, const int dst) {
    m_ports.move(src, dst);
    emit portsChanged();
}
