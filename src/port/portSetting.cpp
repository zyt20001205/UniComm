#include "port/portSetting.h"

#include <QCamera>
#include <QCameraDevice>
#include <QHostInfo>
#include <QImageCapture>
#include <QJsonArray>
#include <QJsonObject>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QQuickWidget>
#include <QScreenCapture>
#include <QVideoSink>
#include <QtConcurrentRun>
#include <simpleble/Peripheral.h>

#include <exception>
#include <utility>

#include "globals.h"
#include "core/globalManager.h"
#include "port/module/deviceDiscovery.h"

// public
PortSetting::PortSetting(QWidget *parent)
    : QObject(parent),
      m_window(new QQuickView()),
      m_bluetoothServices(new QStandardItemModel(this)),
      m_mediaCaptureSession(new QMediaCaptureSession(this)),
      m_roiModel(new RoiModel(this)),
      m_pipelineModel(new PipelineModel(this)),
      m_imageProvider(new ImageProvider()) {
    m_bluetoothServices->setItemRoleNames({
        {Qt::DisplayRole, "uuid"},
        {Qt::UserRole + 1, "capabilities"},
        {Qt::UserRole + 2, "name"},
        {Qt::UserRole + 3, "shortUuid"}
    });
}

PortSetting::~PortSetting() {
    m_bleScanFuture.waitForFinished();
    m_bleConnectFuture.waitForFinished();
    delete m_window;
}

void PortSetting::propertySet(const QVariantHash &objects) {
    m_window->setTitle(tr("Port Setting"));
    m_window->setTransientParent(g_mainWindow->windowHandle());
    m_window->engine()->addImageProvider("capture", m_imageProvider);

    m_window->rootContext()->setContextProperty("portSetting", this);
    m_window->rootContext()->setContextProperty("global", g_globalManager);
    m_window->rootContext()->setContextProperty("mainToolTip", objects["mainWindowToolTip"]);
    m_window->rootContext()->setContextProperty("roiModel", m_roiModel);
    m_window->rootContext()->setContextProperty("pipelineModel", m_pipelineModel);

    m_window->setResizeMode(QQuickView::SizeRootObjectToView);
    m_window->setSource(QUrl("qrc:/qml/port/portSetting.qml"));
    m_root = m_window->rootObject();
}

void PortSetting::propertyGet(const QVariantMap &objects) {
    m_swipeView = qvariant_cast<QObject *>(objects["swipeView"]);
    // serial port
    m_serialPortNameTextField = qvariant_cast<QObject *>(objects["serialPortNameTextField"]);
    m_serialPortDeviceNameComboBox = qvariant_cast<QObject *>(objects["serialPortDeviceNameComboBox"]);
    m_serialPortBaudRateSpinBox = qvariant_cast<QObject *>(objects["serialPortBaudRateSpinBox"]);
    m_serialPortDataBitsComboBox = qvariant_cast<QObject *>(objects["serialPortDataBitsComboBox"]);
    m_serialPortParityComboBox = qvariant_cast<QObject *>(objects["serialPortParityComboBox"]);
    m_serialPortStopBitsComboBox = qvariant_cast<QObject *>(objects["serialPortStopBitsComboBox"]);
    // vision
    m_visionNameComboBox = qvariant_cast<QObject *>(objects["visionNameComboBox"]);
    // tcp client
    m_tcpClientNameTextField = qvariant_cast<QObject *>(objects["tcpClientNameTextField"]);
    m_tcpClientRemoteHostTextField = qvariant_cast<QObject *>(objects["tcpClientRemoteHostTextField"]);
    m_tcpClientRemotePortSpinBox = qvariant_cast<QObject *>(objects["tcpClientRemotePortSpinBox"]);
    // tcp server
    m_tcpServerNameTextField = qvariant_cast<QObject *>(objects["tcpServerNameTextField"]);
    m_tcpServerLocalHostComboBox = qvariant_cast<QObject *>(objects["tcpServerLocalHostComboBox"]);
    m_tcpServerLocalPortSpinBox = qvariant_cast<QObject *>(objects["tcpServerLocalPortSpinBox"]);
    // ssl client
    m_sslClientNameTextField = qvariant_cast<QObject *>(objects["sslClientNameTextField"]);
    m_sslClientRemoteHostTextField = qvariant_cast<QObject *>(objects["sslClientRemoteHostTextField"]);
    m_sslClientRemotePortSpinBox = qvariant_cast<QObject *>(objects["sslClientRemotePortSpinBox"]);
    // ssl server
    m_sslServerNameTextField = qvariant_cast<QObject *>(objects["sslServerNameTextField"]);
    m_sslServerLocalHostComboBox = qvariant_cast<QObject *>(objects["sslServerLocalHostComboBox"]);
    m_sslServerLocalPortSpinBox = qvariant_cast<QObject *>(objects["sslServerLocalPortSpinBox"]);
    m_sslServerCertificateTextField = qvariant_cast<QObject *>(objects["sslServerCertificateTextField"]);
    m_sslServerPrivateKeyTextField = qvariant_cast<QObject *>(objects["sslServerPrivateKeyTextField"]);
    // web socket client
    m_webSocketClientNameTextField = qvariant_cast<QObject *>(objects["webSocketClientNameTextField"]);
    m_webSocketClientUrlTextField = qvariant_cast<QObject *>(objects["webSocketClientUrlTextField"]);
    // web socket server
    m_webSocketServerNameTextField = qvariant_cast<QObject *>(objects["webSocketServerNameTextField"]);
    m_webSocketServerLocalHostComboBox = qvariant_cast<QObject *>(objects["webSocketServerLocalHostComboBox"]);
    m_webSocketServerLocalPortSpinBox = qvariant_cast<QObject *>(objects["webSocketServerLocalPortSpinBox"]);
    // udp socket
    m_udpSocketNameTextField = qvariant_cast<QObject *>(objects["udpSocketNameTextField"]);
    m_udpSocketLocalHostComboBox = qvariant_cast<QObject *>(objects["udpSocketLocalHostComboBox"]);
    m_udpSocketLocalPortSpinBox = qvariant_cast<QObject *>(objects["udpSocketLocalPortSpinBox"]);
    m_udpSocketRemoteHostTextField = qvariant_cast<QObject *>(objects["udpSocketRemoteHostTextField"]);
    m_udpSocketRemotePortSpinBox = qvariant_cast<QObject *>(objects["udpSocketRemotePortSpinBox"]);
    // bluetooth le
    m_bluetoothNameTextField = qvariant_cast<QObject *>(objects["bluetoothNameTextField"]);
    m_bluetoothPeripheralComboBox = qvariant_cast<QObject *>(objects["bluetoothPeripheralComboBox"]);
    // format
    m_logFormatComboBox = qvariant_cast<QObject *>(objects["logFormatComboBox"]);
    m_txSuffixComboBox = qvariant_cast<QObject *>(objects["txSuffixComboBox"]);
    m_bufferSizeSpinBox = qvariant_cast<QObject *>(objects["bufferSizeSpinBox"]);
    // image
    m_videoSink = objects["videoSink"].value<QVideoSink *>();
    m_mediaCaptureSession->setVideoSink(m_videoSink);
    m_previewImage = qvariant_cast<QObject *>(objects["previewImage"]);
    m_recognitionComboBox = qvariant_cast<QObject *>(objects["recognitionComboBox"]);
    m_templateTextField = qvariant_cast<QObject *>(objects["templateTextField"]);
}

void PortSetting::portSettingImport(const QJsonObject &portConfig) {
    serialPortRefresh();
    localHostRefresh();
    visionRefresh();
    bleRefresh();
    processRefresh(portConfig);
    if (portConfig.isEmpty()) {
        m_root->setProperty("portType", 0);
        m_swipeView->setProperty("currentIndex", 0);
        m_oldPortName = "";
        // serial port
        m_serialPortNameTextField->setProperty("text", "");
        if (m_serialPortDeviceNameComboBox->property("count").toInt()) {
            m_serialPortDeviceNameComboBox->setProperty("currentIndex", 0);
        }
        m_serialPortBaudRateSpinBox->setProperty("value", 115200);
        m_serialPortDataBitsComboBox->setProperty("currentValue", 8);
        m_serialPortParityComboBox->setProperty("currentValue", 0);
        m_serialPortStopBitsComboBox->setProperty("currentValue", 1);
        // vision
        if (m_visionNameComboBox->property("count").toInt()) {
            m_visionNameComboBox->setProperty("currentIndex", 0);
        }
        // tcp client
        m_tcpClientNameTextField->setProperty("text", "");
        m_tcpClientRemoteHostTextField->setProperty("text", "");
        m_tcpClientRemotePortSpinBox->setProperty("value", 0);
        // tcp server
        m_tcpServerNameTextField->setProperty("text", "");
        if (m_tcpServerLocalHostComboBox->property("count").toInt()) {
            m_tcpServerLocalHostComboBox->setProperty("currentIndex", 0);
        }
        m_tcpServerLocalHostComboBox->setProperty("editText", m_tcpServerLocalHostComboBox->property("currentText"));
        m_tcpServerLocalPortSpinBox->setProperty("value", 0);
        // ssl client
        m_sslClientNameTextField->setProperty("text", "");
        m_sslClientRemoteHostTextField->setProperty("text", "");
        m_sslClientRemotePortSpinBox->setProperty("value", 0);
        // ssl server
        m_sslServerNameTextField->setProperty("text", "");
        if (m_sslServerLocalHostComboBox->property("count").toInt()) {
            m_sslServerLocalHostComboBox->setProperty("currentIndex", 0);
        }
        m_sslServerLocalHostComboBox->setProperty("editText", m_sslServerLocalHostComboBox->property("currentText"));
        m_sslServerLocalPortSpinBox->setProperty("value", 0);
        m_sslServerCertificateTextField->setProperty("text", "");
        m_sslServerPrivateKeyTextField->setProperty("text", "");
        // web socket client
        m_webSocketClientNameTextField->setProperty("text", "");
        m_webSocketClientUrlTextField->setProperty("text", "");
        // web socket server
        m_webSocketServerNameTextField->setProperty("text", "");
        if (m_webSocketServerLocalHostComboBox->property("count").toInt()) {
            m_webSocketServerLocalHostComboBox->setProperty("currentIndex", 0);
        }
        m_webSocketServerLocalHostComboBox->setProperty("editText", m_webSocketServerLocalHostComboBox->property("currentText"));
        m_webSocketServerLocalPortSpinBox->setProperty("value", 0);
        // udp socket
        m_udpSocketNameTextField->setProperty("text", "");
        if (m_udpSocketLocalHostComboBox->property("count").toInt()) {
            m_udpSocketLocalHostComboBox->setProperty("currentIndex", 0);
        }
        m_udpSocketLocalPortSpinBox->setProperty("value", 0);
        m_udpSocketRemoteHostTextField->setProperty("text", "");
        m_udpSocketRemotePortSpinBox->setProperty("value", 0);
        // bluetooth le
        m_bluetoothNameTextField->setProperty("text", "");
        m_bluetoothPeripheralComboBox->setProperty("currentValue", QVariant{});
        // format
        m_logFormatComboBox->setProperty("currentValue", "utf-8");
        m_txSuffixComboBox->setProperty("currentValue", "null");
        m_bufferSizeSpinBox->setProperty("value", 65536);
    } else {
        m_swipeView->setProperty("currentIndex", 1);
        m_oldPortName = portConfig["portName"].toString();
        const int portType = portConfig["portType"].toInt();
        m_root->setProperty("portType", portType);
        switch (portType) {
            case PortType::SerialPort: {
                m_serialPortNameTextField->setProperty("text", portConfig["portName"].toString());
                m_serialPortDeviceNameComboBox->setProperty("currentValue", portConfig["deviceName"].toString());
                m_serialPortBaudRateSpinBox->setProperty("value", portConfig["baudRate"].toInt());
                m_serialPortDataBitsComboBox->setProperty("currentValue", portConfig["dataBits"].toInt());
                m_serialPortParityComboBox->setProperty("currentValue", portConfig["parity"].toInt());
                m_serialPortStopBitsComboBox->setProperty("currentValue", portConfig["stopBits"].toInt());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::Vision: {
                m_visionNameComboBox->setProperty("currentValue", portConfig["portName"].toString());
                const auto &recognition = portConfig["recognition"].toObject();
                const auto mode = recognition["mode"].toInt();
                m_recognitionComboBox->setProperty("currentIndex", mode);
                switch (mode) {
                    case Recognition::OCR: {
                    }
                        break;
                    case Recognition::CornerShiTomasi: {
                    }
                        break;
                    case Recognition::CornerHarris: {
                    }
                        break;
                    case Recognition::TemplateMatch: {
                        m_templateTextField->setProperty("text", recognition["template"].toString());
                    }
                        break;
                    default: return;
                }
            }
            break;
            case PortType::TcpClient: {
                m_tcpClientNameTextField->setProperty("text", portConfig["portName"].toString());
                m_tcpClientRemoteHostTextField->setProperty("text", portConfig["remoteHost"].toString());
                m_tcpClientRemotePortSpinBox->setProperty("value", portConfig["remotePort"].toInt());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::TcpServer: {
                m_tcpServerNameTextField->setProperty("text", portConfig["portName"].toString());
                m_tcpServerLocalHostComboBox->setProperty("currentValue", portConfig["localHost"].toString());
                m_tcpServerLocalHostComboBox->setProperty("editText", portConfig["localHost"].toString());
                m_tcpServerLocalPortSpinBox->setProperty("value", portConfig["localPort"].toInt());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::SslClient: {
                m_sslClientNameTextField->setProperty("text", portConfig["portName"].toString());
                m_sslClientRemoteHostTextField->setProperty("text", portConfig["remoteHost"].toString());
                m_sslClientRemotePortSpinBox->setProperty("value", portConfig["remotePort"].toInt());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::SslServer: {
                m_sslServerNameTextField->setProperty("text", portConfig["portName"].toString());
                m_sslServerLocalHostComboBox->setProperty("currentValue", portConfig["localHost"].toString());
                m_sslServerLocalHostComboBox->setProperty("editText", portConfig["localHost"].toString());
                m_sslServerLocalPortSpinBox->setProperty("value", portConfig["localPort"].toInt());
                m_sslServerCertificateTextField->setProperty("text", portConfig["certificate"].toString());
                m_sslServerPrivateKeyTextField->setProperty("text", portConfig["privateKey"].toString());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::WebSocketClient: {
                m_webSocketClientNameTextField->setProperty("text", portConfig["portName"].toString());
                m_webSocketClientUrlTextField->setProperty("text", portConfig["url"].toString());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::WebSocketServer: {
                m_webSocketServerNameTextField->setProperty("text", portConfig["portName"].toString());
                m_webSocketServerLocalHostComboBox->setProperty("currentValue", portConfig["localHost"].toString());
                m_webSocketServerLocalHostComboBox->setProperty("editText", portConfig["localHost"].toString());
                m_webSocketServerLocalPortSpinBox->setProperty("value", portConfig["localPort"].toInt());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::UdpSocket: {
                m_udpSocketNameTextField->setProperty("text", portConfig["portName"].toString());
                m_udpSocketLocalHostComboBox->setProperty("currentValue", portConfig["localHost"].toString());
                m_udpSocketLocalPortSpinBox->setProperty("value", portConfig["localPort"].toInt());
                m_udpSocketRemoteHostTextField->setProperty("text", portConfig["remoteHost"].toString());
                m_udpSocketRemotePortSpinBox->setProperty("value", portConfig["remotePort"].toInt());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::BluetoothLe: {
                m_bluetoothNameTextField->setProperty("text", portConfig["portName"].toString());
                const auto adapterAddress = portConfig["adapterAddress"].toString();
                auto address = portConfig["peripheralAddress"].toString();
                bool found = false;
                for (auto &peripheral: m_bluetoothPeripherals) {
                    auto current = peripheral.toHash();
                    const auto currentAddress = current.value("address").toString();
                    if (currentAddress.compare(address, Qt::CaseInsensitive) != 0) continue;
                    address = currentAddress;
                    if (current.value("adapterAddress").toString().compare(adapterAddress, Qt::CaseInsensitive) != 0) {
                        current["adapterAddress"] = adapterAddress;
                        current.remove("peripheral");
                        peripheral = current;
                        emit bluetoothPeripheralsChanged();
                    }
                    found = true;
                    break;
                }
                if (!found) {
                    m_bluetoothPeripherals.append(QVariantHash{{"adapterAddress", adapterAddress}, {"address", address}, {"name", ""}, {"display", address}});
                    emit bluetoothPeripheralsChanged();
                }
                m_bluetoothPeripheralComboBox->setProperty("currentValue", address);
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            default: break;
        }
    }
    m_window->resize(600, 500);
    m_window->show();
}

void PortSetting::portSettingExport() {
    const int portType = m_root->property("portType").toInt();
    QJsonObject portConfig{};
    switch (portType) {
        case PortType::SerialPort: {
            portConfig = {
                {"portType", portType},
                {"portName", m_serialPortNameTextField->property("text").toString()},
                {"deviceName", m_serialPortDeviceNameComboBox->property("currentValue").toString()},
                {"baudRate", m_serialPortBaudRateSpinBox->property("value").toInt()},
                {"dataBits", m_serialPortDataBitsComboBox->property("currentValue").toInt()},
                {"parity", m_serialPortParityComboBox->property("currentValue").toInt()},
                {"stopBits", m_serialPortStopBitsComboBox->property("currentValue").toInt()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
        }
        break;
        case PortType::Vision: {
            if (m_screenCapture) {
                m_mediaCaptureSession->setScreenCapture(nullptr);
                m_screenCapture->stop();
                m_screenCapture->deleteLater();
                m_screenCapture = nullptr;
            } else if (m_cameraCapture) {
                m_mediaCaptureSession->setCamera(nullptr);
                m_cameraCapture->stop();
                m_cameraCapture->deleteLater();
                m_cameraCapture = nullptr;
            }
            QJsonArray roiArray{};
            for (int i = 0; i < m_roiModel->rowCount(); ++i) {
                const QJsonArray roi = QJsonArray::fromVariantList(m_roiModel->item(i, 0)->data(Qt::WhatsThisRole).toList());
                roiArray.append(roi);
            }
            QJsonArray pipelineArray{};
            for (int i = 0; i < m_pipelineModel->rowCount(); ++i) {
                const QJsonObject session = QJsonObject::fromVariantHash(m_pipelineModel->item(i, 0)->data(Qt::WhatsThisRole).toHash());
                pipelineArray.append(session);
            }
            portConfig = {
                {"portType", portType},
                {"portName", m_visionNameComboBox->property("currentValue").toString()},
                {"roi", roiArray},
                {"pipeline", pipelineArray}
            };
            // recognition
            QJsonObject recognition{};
            const auto mode = m_recognitionComboBox->property("currentIndex").toInt();
            recognition["mode"] = mode;
            switch (mode) {
                case Recognition::OCR: {
                }
                    break;
                case Recognition::CornerShiTomasi: {
                }
                    break;
                case Recognition::CornerHarris: {
                }
                    break;
                case Recognition::TemplateMatch: {
                    const auto templateUrl = m_templateTextField->property("text").toString();
                    if (templateUrl.isEmpty()) return;
                    recognition["template"] = m_templateTextField->property("text").toString();
                }
                    break;
                default: return;
            }
            portConfig["recognition"] = recognition;
        }
        break;
        case PortType::TcpClient: {
            portConfig = {
                {"portType", portType},
                {"portName", m_tcpClientNameTextField->property("text").toString()},
                {"remoteHost", m_tcpClientRemoteHostTextField->property("text").toString()},
                {"remotePort", m_tcpClientRemotePortSpinBox->property("value").toInt()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
        }
        break;
        case PortType::TcpServer: {
            portConfig = {
                {"portType", portType},
                {"portName", m_tcpServerNameTextField->property("text").toString()},
                {"localHost", m_tcpServerLocalHostComboBox->property("editText").toString()},
                {"localPort", m_tcpServerLocalPortSpinBox->property("value").toInt()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
        }
        break;
        case PortType::SslClient: {
            portConfig = {
                {"portType", portType},
                {"portName", m_sslClientNameTextField->property("text").toString()},
                {"remoteHost", m_sslClientRemoteHostTextField->property("text").toString()},
                {"remotePort", m_sslClientRemotePortSpinBox->property("value").toInt()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
        }
        break;
        case PortType::SslServer: {
            portConfig = {
                {"portType", portType},
                {"portName", m_sslServerNameTextField->property("text").toString()},
                {"localHost", m_sslServerLocalHostComboBox->property("editText").toString()},
                {"localPort", m_sslServerLocalPortSpinBox->property("value").toInt()},
                {"certificate", m_sslServerCertificateTextField->property("text").toString()},
                {"privateKey", m_sslServerPrivateKeyTextField->property("text").toString()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
        }
        break;
        case PortType::WebSocketClient: {
            portConfig = {
                {"portType", portType},
                {"portName", m_webSocketClientNameTextField->property("text").toString()},
                {"url", m_webSocketClientUrlTextField->property("text").toString()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
        }
        break;
        case PortType::WebSocketServer: {
            portConfig = {
                {"portType", portType},
                {"portName", m_webSocketServerNameTextField->property("text").toString()},
                {"localHost", m_webSocketServerLocalHostComboBox->property("editText").toString()},
                {"localPort", m_webSocketServerLocalPortSpinBox->property("value").toInt()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
        }
        break;
        case PortType::UdpSocket: {
            portConfig = {
                {"portType", portType},
                {"portName", m_udpSocketNameTextField->property("text").toString()},
                {"localHost", m_udpSocketLocalHostComboBox->property("currentValue").toString()},
                {"localPort", m_udpSocketLocalPortSpinBox->property("value").toInt()},
                {"remoteHost", m_udpSocketRemoteHostTextField->property("text").toString()},
                {"remotePort", m_udpSocketRemotePortSpinBox->property("value").toInt()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
        }
        break;
        case PortType::BluetoothLe: {
            const auto peripheral = m_bluetoothPeripherals.value(m_bluetoothPeripheralComboBox->property("currentIndex").toInt()).toHash();
            portConfig = {
                {"portType", portType},
                {"portName", m_bluetoothNameTextField->property("text").toString()},
                {"adapterAddress", peripheral.value("adapterAddress").toString()},
                {"peripheralAddress", m_bluetoothPeripheralComboBox->property("currentValue").toString()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
        }
        break;
        default: break;
    }
    if (m_oldPortName.isEmpty()) {
        emit insertPort(-1, portConfig, {});
    } else {
        emit editPort(m_oldPortName, portConfig);
    }
    m_window->hide();
}

void PortSetting::dialogResize(const int width, const int height) const {
    m_window->resize(width, height);
}

void PortSetting::bleConnect(const QVariant &peripheral) {
    if (m_bleConnectFuture.isRunning() || !peripheral.canConvert<SimpleBLE::Peripheral>()) return;
    m_bluetoothServiceAddress = QString::fromStdString(peripheral.value<SimpleBLE::Peripheral>().address());
    m_bluetoothServices->clear();
    emit bluetoothServicesChanged();
    m_bleConnectFuture = QtConcurrent::run([peripheral] {
        try {
            return DeviceDiscovery::Bluetooth::connect(peripheral);
        } catch (const std::exception &exception) {
            qWarning("Bluetooth service discovery failed: %s", exception.what());
        } catch (...) {
            qWarning("Bluetooth service discovery failed");
        }
        return QVariantList{};
    });
    m_bleConnectFuture.then(this, [this, address = m_bluetoothServiceAddress](const QVariantList &services) {
        if (m_bluetoothServiceAddress != address) return;
        m_bluetoothServices->clear();
        for (const auto &service: services) {
            const auto data = service.toHash();
            auto *serviceItem = new QStandardItem(data.value("uuid").toString());
            serviceItem->setEditable(false);
            serviceItem->setData(QStringList{}, Qt::UserRole + 1);
            serviceItem->setData(data.value("name"), Qt::UserRole + 2);
            serviceItem->setData(data.value("shortUuid"), Qt::UserRole + 3);
            for (const auto &characteristic: data.value("characteristics").toList()) {
                const auto characteristicData = characteristic.toHash();
                auto *characteristicItem = new QStandardItem(characteristicData.value("uuid").toString());
                characteristicItem->setEditable(false);
                characteristicItem->setData(characteristicData.value("capabilities"), Qt::UserRole + 1);
                characteristicItem->setData(characteristicData.value("name"), Qt::UserRole + 2);
                characteristicItem->setData(characteristicData.value("shortUuid"), Qt::UserRole + 3);
                for (const auto &descriptor: characteristicData.value("descriptors").toList()) {
                    const auto descriptorData = descriptor.toHash();
                    auto *descriptorItem = new QStandardItem(descriptorData.value("uuid").toString());
                    descriptorItem->setEditable(false);
                    descriptorItem->setData(QStringList{}, Qt::UserRole + 1);
                    descriptorItem->setData(descriptorData.value("name"), Qt::UserRole + 2);
                    descriptorItem->setData(descriptorData.value("shortUuid"), Qt::UserRole + 3);
                    characteristicItem->appendRow(descriptorItem);
                }
                serviceItem->appendRow(characteristicItem);
            }
            m_bluetoothServices->appendRow(serviceItem);
        }
        emit bluetoothServicesChanged();
    });
}

void PortSetting::visionCapture() {
    m_window->resize(1600, 900);
    if (m_screenCapture) {
        m_mediaCaptureSession->setScreenCapture(nullptr);
        m_screenCapture->stop();
        m_screenCapture->deleteLater();
        m_screenCapture = nullptr;
    } else if (m_cameraCapture) {
        m_mediaCaptureSession->setCamera(nullptr);
        m_cameraCapture->stop();
        m_cameraCapture->deleteLater();
        m_cameraCapture = nullptr;
    }
    const auto &portName = m_visionNameComboBox->property("currentValue").toString();
    for (QScreen *screen: QGuiApplication::screens()) {
        if (portName == screen->name()) {
            m_screenCapture = new QScreenCapture(this);
            m_screenCapture->setScreen(screen);
            m_mediaCaptureSession->setScreenCapture(m_screenCapture);
            m_screenCapture->start();
            return;
        }
    }
    for (const QCameraDevice &camera: QMediaDevices::videoInputs()) {
        if (portName == camera.description()) {
            m_cameraCapture = new QCamera(camera, this);
            m_mediaCaptureSession->setCamera(m_cameraCapture);
            m_cameraCapture->start();
            return;
        }
    }
}

void PortSetting::previewLoad(const int index) const {
    if (index == -1 || !m_roiModel->item(index, 0)) {
        m_previewImage->setProperty("source", "qrc:/icon/null.svg");
        m_previewImage->setProperty("recognition", QVariantHash{});
        return;
    }
    QJsonArray roiArray{};
    const QJsonArray roi = QJsonArray::fromVariantList(m_roiModel->item(index, 0)->data(Qt::WhatsThisRole).toList());
    roiArray.append(roi);
    QJsonArray pipeline{};
    for (int i = 0; i < m_pipelineModel->rowCount(); ++i) {
        const QJsonObject session = QJsonObject::fromVariantHash(m_pipelineModel->item(i, 0)->data(Qt::WhatsThisRole).toHash());
        pipeline.append(session);
    }
    QJsonObject recognition{};
    const auto mode = m_recognitionComboBox->property("currentIndex").toInt();
    recognition["mode"] = mode;
    switch (mode) {
        case Recognition::OCR: {
        }
        break;
        case Recognition::CornerShiTomasi: {
        }
        break;
        case Recognition::CornerHarris: {
        }
        break;
        case Recognition::TemplateMatch: {
            const auto templateUrl = m_templateTextField->property("text").toString();
            if (templateUrl.isEmpty()) return;
            recognition["template"] = m_templateTextField->property("text").toString();
        }
        break;
        default: return;
    }
    const QJsonObject config{
        {"roi", roiArray},
        {"pipeline", pipeline},
        {"recognition", recognition}
    };
    m_imageProvider->preview(m_videoSink, config);
    m_previewImage->setProperty("source", "image://capture/" + QString::number(QDateTime::currentMSecsSinceEpoch()));
    m_previewImage->setProperty("recognition", m_imageProvider->recognition());
}

void PortSetting::roiInsert(const QVariantList &roi) const {
    QString text{};
    if (roi.size() == 4) {
        text = "Rectangle";
    } else if (roi.size() == 8) {
        text = "Quadrilateral";
    }
    auto *item = new QStandardItem(text); // NOLINT
    m_roiModel->appendRow(item);
    item->setData(roi, Qt::WhatsThisRole);
    QMetaObject::invokeMethod(m_root, "indicatorReload");
}

void PortSetting::roiRemove(const int index) const {
    m_roiModel->removeRow(index);
    QMetaObject::invokeMethod(m_root, "indicatorReload");
}

void PortSetting::roiSwap(const int src, const int dst) const {
    const auto tmp = m_roiModel->takeRow(src);
    m_roiModel->insertRow(dst, tmp);
    QMetaObject::invokeMethod(m_root, "indicatorReload");
}

void PortSetting::pipelineInsert(const QVariantHash &session) const {
    const int type = session["type"].toInt();
    switch (type) {
        case ImagePipeline::Scale: {
            auto *item = new QStandardItem("Scale"); // NOLINT
            m_pipelineModel->appendRow(item);
            item->setData(session, Qt::WhatsThisRole);
        }
        break;
        case ImagePipeline::Threshold: {
            auto *item = new QStandardItem("Threshold"); // NOLINT
            m_pipelineModel->appendRow(item);
            item->setData(session, Qt::WhatsThisRole);
        }
        break;
        default: break;
    }
}

void PortSetting::pipelineRemove(const int index) const {
    m_pipelineModel->removeRow(index);
}

void PortSetting::pipelineSwap(const int src, const int dst) const {
    const auto tmp = m_pipelineModel->takeRow(src);
    m_pipelineModel->insertRow(dst, tmp);
}

// private
QVariantList PortSetting::serialPortsGet() const {
    return m_serialPorts;
}

QStringList PortSetting::localHostsGet() const {
    return m_localHosts;
}

QStringList PortSetting::visionSourcesGet() const {
    return m_visionSources;
}

QVariantList PortSetting::bluetoothPeripheralsGet() const {
    return m_bluetoothPeripherals;
}

QAbstractItemModel *PortSetting::bluetoothServicesGet() const {
    return m_bluetoothServices;
}

QString PortSetting::bluetoothServiceAddressGet() const {
    return m_bluetoothServiceAddress;
}

void PortSetting::serialPortRefresh() {
    const auto serialPorts = DeviceDiscovery::serialPorts();
    if (m_serialPorts == serialPorts) return;
    m_serialPorts = serialPorts;
    emit serialPortsChanged();
}

void PortSetting::localHostRefresh() {
    QStringList localHosts{};
    for (const QHostAddress &address: QHostInfo::fromName(QHostInfo::localHostName()).addresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol || address.protocol() == QAbstractSocket::IPv6Protocol) {
            localHosts.append(address.toString());
        }
    }
    if (m_localHosts == localHosts) return;
    m_localHosts = localHosts;
    emit localHostsChanged();
}

void PortSetting::visionRefresh() {
    const auto visionSources = DeviceDiscovery::screens() + DeviceDiscovery::cameras();
    if (m_visionSources == visionSources) return;
    m_visionSources = visionSources;
    emit visionSourcesChanged();
}

void PortSetting::bleRefresh() {
    m_bluetoothServices->clear();
    m_bluetoothServiceAddress.clear();
    emit bluetoothServicesChanged();
    if (m_bleScanFuture.isRunning()) return;
    m_bluetoothPeripherals.clear();
    emit bluetoothPeripheralsChanged();
    m_bleScanFuture = QtConcurrent::run([this] {
        try {
            DeviceDiscovery::Bluetooth::scan([this](QVariantHash device) {
                QMetaObject::invokeMethod(this, [this, device = std::move(device)] {
                    const auto address = device.value("address").toString();
                    for (auto &peripheral: m_bluetoothPeripherals) {
                        auto current = peripheral.toHash();
                        if (current.value("address").toString().compare(address, Qt::CaseInsensitive) != 0) continue;
                        if (current.value("adapterAddress").toString().compare(device.value("adapterAddress").toString(), Qt::CaseInsensitive) != 0) return;
                        if (current.value("peripheral").isValid()
                            && (device.value("name").toString().isEmpty() || current.value("name") == device.value("name"))) return;
                        auto updated = device;
                        updated["address"] = current.value("address");
                        if (device.value("name").toString().isEmpty()) {
                            updated["name"] = current.value("name");
                            updated["display"] = current.value("display");
                        }
                        peripheral = updated;
                        emit bluetoothPeripheralsChanged();
                        return;
                    }
                    m_bluetoothPeripherals.append(device);
                    emit bluetoothPeripheralsChanged();
                }, Qt::QueuedConnection);
            });
        } catch (const std::exception &exception) {
            qWarning("Bluetooth discovery failed: %s", exception.what());
        } catch (...) {
            qWarning("Bluetooth discovery failed");
        }
    });
}

void PortSetting::processRefresh(const QJsonObject &portConfig) const {
    m_roiModel->clear();
    m_pipelineModel->clear();
    QMetaObject::invokeMethod(m_root, "indicatorReload");
    m_recognitionComboBox->setProperty("currentIndex", 0);
    if (!portConfig.isEmpty()) {
        // roi
        for (const auto &value: portConfig["roi"].toArray()) {
            QJsonArray roi = value.toArray();
            roiInsert(roi.toVariantList());
        }
        for (const auto &value: portConfig["pipeline"].toArray()) {
            QVariantHash session = value.toObject().toVariantHash();
            pipelineInsert(session);
        }
    }
}

// public
ImageProvider::ImageProvider()
    : QQuickImageProvider(Pixmap) {
}

QPixmap ImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) {
    return m_preview;
}

void ImageProvider::preview(const QVideoSink *videoSink, const QJsonObject &config) {
    m_preview = {};
    m_recognition = {};
    if (!videoSink) {
        return;
    }
    const auto videoFrame = videoSink->videoFrame();
    const auto image = videoFrame.toImage();
    if (image.isNull()) {
        return;
    }
    m_imageProcess.configSet(config);
    const auto results = m_imageProcess.process(image);
    if (results.isEmpty()) {
        return;
    }
    m_preview = QPixmap::fromImage(results.first().pipelineFrame);
    m_recognition = results.first().result;
}

QVariantHash ImageProvider::recognition() const {
    return m_recognition;
}

// public
RoiModel::RoiModel(QObject *parent)
    : QStandardItemModel(parent) {
    connect(this, &QAbstractItemModel::rowsInserted, this, &RoiModel::emptyChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &RoiModel::emptyChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &RoiModel::emptyChanged);
}

// public
PipelineModel::PipelineModel(QObject *parent)
    : QStandardItemModel(parent) {
    connect(this, &QAbstractItemModel::rowsInserted, this, &PipelineModel::emptyChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &PipelineModel::emptyChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &PipelineModel::emptyChanged);
}
