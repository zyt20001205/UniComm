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
#include <QStringList>
#include <QThread>
#include <QVBoxLayout>
#include <QVideoSink>
#include <visa.h>

#include "globals.h"
#include "core/globalManager.h"
#include "port/module/bluetoothDiscovery.h"
#include "port/module/deviceDiscovery.h"

// public
PortSetting::PortSetting(QWidget *parent)
    : QObject(parent),
      m_window(new QQuickView()),
      m_serialPortStandardItemModel(new QStandardItemModel(this)),
      m_visaStandardItemModel(new QStandardItemModel(this)),
      m_localHostStandardItemModel(new QStandardItemModel(this)),
      m_videoStreamStandardItemModel(new QStandardItemModel(this)),
      m_bluetoothAdapterStandardItemModel(new QStandardItemModel(this)),
      m_bluetoothPeripheralStandardItemModel(new QStandardItemModel(this)),
      m_bluetoothServiceStandardItemModel(new QStandardItemModel(this)),
      m_bluetoothTxCharacteristicStandardItemModel(new QStandardItemModel(this)),
      m_bluetoothRxCharacteristicStandardItemModel(new QStandardItemModel(this)),
      m_bluetoothThread(new QThread(this)),
      m_bluetoothDiscovery(new BluetoothDiscovery()),
      m_mediaCaptureSession(new QMediaCaptureSession(this)),
      m_roiModel(new RoiModel(this)),
      m_pipelineModel(new PipelineModel(this)),
      m_imageProvider(new ImageProvider()) {
    m_bluetoothDiscovery->moveToThread(m_bluetoothThread);
    connect(m_bluetoothThread, &QThread::finished, m_bluetoothDiscovery, &QObject::deleteLater);
    connect(m_bluetoothDiscovery, &BluetoothDiscovery::adaptersUpdated, this, &PortSetting::bluetoothAdaptersUpdate);
    connect(m_bluetoothDiscovery, &BluetoothDiscovery::peripheralsUpdated, this, &PortSetting::bluetoothPeripheralsUpdate);
    connect(m_bluetoothDiscovery, &BluetoothDiscovery::servicesUpdated, this, &PortSetting::bluetoothServicesUpdate);
    connect(m_bluetoothDiscovery, &BluetoothDiscovery::statusUpdated, this, &PortSetting::bluetoothStatusUpdate);
    connect(m_bluetoothDiscovery, &BluetoothDiscovery::busyChanged, this, &PortSetting::bluetoothBusyUpdate);
    m_bluetoothThread->start();
}

PortSetting::~PortSetting() {
    m_bluetoothThread->quit();
    m_bluetoothThread->wait();
    delete m_window;
}

void PortSetting::propertySet(const QVariantHash &objects) {
    m_window->setTitle(tr("Port Setting"));
    m_window->setTransientParent(g_mainWindow->windowHandle());
    m_window->engine()->addImageProvider("capture", m_imageProvider);

    m_window->rootContext()->setContextProperty("portSetting", this);
    m_window->rootContext()->setContextProperty("global", g_globalManager);
    m_window->rootContext()->setContextProperty("mainToolTip", objects["mainWindowToolTip"]);
    m_window->rootContext()->setContextProperty("serialPortStandardItemModel", m_serialPortStandardItemModel);
    m_window->rootContext()->setContextProperty("visaStandardItemModel", m_visaStandardItemModel);
    m_window->rootContext()->setContextProperty("localHostStandardItemModel", m_localHostStandardItemModel);
    m_window->rootContext()->setContextProperty("videoStreamStandardItemModel", m_videoStreamStandardItemModel);
    m_window->rootContext()->setContextProperty("bluetoothAdapterStandardItemModel", m_bluetoothAdapterStandardItemModel);
    m_window->rootContext()->setContextProperty("bluetoothPeripheralStandardItemModel", m_bluetoothPeripheralStandardItemModel);
    m_window->rootContext()->setContextProperty("bluetoothServiceStandardItemModel", m_bluetoothServiceStandardItemModel);
    m_window->rootContext()->setContextProperty("bluetoothTxCharacteristicStandardItemModel", m_bluetoothTxCharacteristicStandardItemModel);
    m_window->rootContext()->setContextProperty("bluetoothRxCharacteristicStandardItemModel", m_bluetoothRxCharacteristicStandardItemModel);
    m_window->rootContext()->setContextProperty("roiModel", m_roiModel);
    m_window->rootContext()->setContextProperty("pipelineModel", m_pipelineModel);

    m_window->setResizeMode(QQuickView::SizeRootObjectToView);
    m_window->setSource(QUrl("qrc:/qml/port/portSetting.qml"));
    m_root = m_window->rootObject();
}

void PortSetting::propertyGet(const QVariantMap &objects) {
    m_swipeView = qvariant_cast<QObject *>(objects["swipeView"]);
    // serial port
    m_serialPortNameComboBox = qvariant_cast<QObject *>(objects["serialPortNameComboBox"]);
    m_serialPortBaudRateSpinBox = qvariant_cast<QObject *>(objects["serialPortBaudRateSpinBox"]);
    m_serialPortDataBitsComboBox = qvariant_cast<QObject *>(objects["serialPortDataBitsComboBox"]);
    m_serialPortParityComboBox = qvariant_cast<QObject *>(objects["serialPortParityComboBox"]);
    m_serialPortStopBitsComboBox = qvariant_cast<QObject *>(objects["serialPortStopBitsComboBox"]);
    // visa
    m_visaNameComboBox = qvariant_cast<QObject *>(objects["visaNameComboBox"]);
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
    m_webSocketClientMessageTypeComboBox = qvariant_cast<QObject *>(objects["webSocketClientMessageTypeComboBox"]);
    // web socket server
    m_webSocketServerNameTextField = qvariant_cast<QObject *>(objects["webSocketServerNameTextField"]);
    m_webSocketServerLocalHostComboBox = qvariant_cast<QObject *>(objects["webSocketServerLocalHostComboBox"]);
    m_webSocketServerLocalPortSpinBox = qvariant_cast<QObject *>(objects["webSocketServerLocalPortSpinBox"]);
    m_webSocketServerSecureSwitch = qvariant_cast<QObject *>(objects["webSocketServerSecureSwitch"]);
    m_webSocketServerCertificateTextField = qvariant_cast<QObject *>(objects["webSocketServerCertificateTextField"]);
    m_webSocketServerPrivateKeyTextField = qvariant_cast<QObject *>(objects["webSocketServerPrivateKeyTextField"]);
    m_webSocketServerMessageTypeComboBox = qvariant_cast<QObject *>(objects["webSocketServerMessageTypeComboBox"]);
    // udp socket
    m_udpSocketNameTextField = qvariant_cast<QObject *>(objects["udpSocketNameTextField"]);
    m_udpSocketLocalHostComboBox = qvariant_cast<QObject *>(objects["udpSocketLocalHostComboBox"]);
    m_udpSocketLocalPortSpinBox = qvariant_cast<QObject *>(objects["udpSocketLocalPortSpinBox"]);
    m_udpSocketRemoteHostTextField = qvariant_cast<QObject *>(objects["udpSocketRemoteHostTextField"]);
    m_udpSocketRemotePortSpinBox = qvariant_cast<QObject *>(objects["udpSocketRemotePortSpinBox"]);
    // video stream
    m_videoStreamNameComboBox = qvariant_cast<QObject *>(objects["videoStreamNameComboBox"]);
    // bluetooth le
    m_bluetoothNameTextField = qvariant_cast<QObject *>(objects["bluetoothNameTextField"]);
    m_bluetoothAdapterComboBox = qvariant_cast<QObject *>(objects["bluetoothAdapterComboBox"]);
    m_bluetoothPeripheralComboBox = qvariant_cast<QObject *>(objects["bluetoothPeripheralComboBox"]);
    m_bluetoothServiceComboBox = qvariant_cast<QObject *>(objects["bluetoothServiceComboBox"]);
    m_bluetoothTxCharacteristicComboBox = qvariant_cast<QObject *>(objects["bluetoothTxCharacteristicComboBox"]);
    m_bluetoothRxCharacteristicComboBox = qvariant_cast<QObject *>(objects["bluetoothRxCharacteristicComboBox"]);
    m_bluetoothWriteTypeComboBox = qvariant_cast<QObject *>(objects["bluetoothWriteTypeComboBox"]);
    m_bluetoothSubscribeTypeComboBox = qvariant_cast<QObject *>(objects["bluetoothSubscribeTypeComboBox"]);
    m_bluetoothStatusLabel = qvariant_cast<QObject *>(objects["bluetoothStatusLabel"]);
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
    visaRefresh();
    localHostRefresh();
    videoStreamRefresh();
    m_bluetoothConfig = portConfig["portType"].toInt(-1) == PortType::BluetoothLe ? portConfig : QJsonObject{};
    m_bluetoothServices.clear();
    m_bluetoothAdapterStandardItemModel->clear();
    m_bluetoothPeripheralStandardItemModel->clear();
    m_bluetoothServiceStandardItemModel->clear();
    m_bluetoothTxCharacteristicStandardItemModel->clear();
    m_bluetoothRxCharacteristicStandardItemModel->clear();
    bluetoothAdapterRefresh();
    processRefresh(portConfig);
    if (portConfig.isEmpty()) {
        m_root->setProperty("portType", 0);
        m_swipeView->setProperty("currentIndex", 0);
        m_oldPortName = "";
        // serial port
        if (m_serialPortNameComboBox->property("count").toInt()) {
            m_serialPortNameComboBox->setProperty("currentIndex", 0);
        }
        m_serialPortBaudRateSpinBox->setProperty("value", 115200);
        m_serialPortDataBitsComboBox->setProperty("currentValue", 8);
        m_serialPortParityComboBox->setProperty("currentValue", 0);
        m_serialPortStopBitsComboBox->setProperty("currentValue", 1);
        // visa
        if (m_visaNameComboBox->property("count").toInt()) {
            m_visaNameComboBox->setProperty("currentIndex", 0);
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
        m_sslServerLocalPortSpinBox->setProperty("value", 0);
        m_sslServerCertificateTextField->setProperty("text", "");
        m_sslServerPrivateKeyTextField->setProperty("text", "");
        // web socket client
        m_webSocketClientNameTextField->setProperty("text", "");
        m_webSocketClientUrlTextField->setProperty("text", "");
        m_webSocketClientMessageTypeComboBox->setProperty("currentValue", "binary");
        // web socket server
        m_webSocketServerNameTextField->setProperty("text", "");
        if (m_webSocketServerLocalHostComboBox->property("count").toInt()) {
            m_webSocketServerLocalHostComboBox->setProperty("currentIndex", 0);
        }
        m_webSocketServerLocalPortSpinBox->setProperty("value", 0);
        m_webSocketServerSecureSwitch->setProperty("checked", false);
        m_webSocketServerCertificateTextField->setProperty("text", "");
        m_webSocketServerPrivateKeyTextField->setProperty("text", "");
        m_webSocketServerMessageTypeComboBox->setProperty("currentValue", "binary");
        // udp socket
        m_udpSocketNameTextField->setProperty("text", "");
        if (m_udpSocketLocalHostComboBox->property("count").toInt()) {
            m_udpSocketLocalHostComboBox->setProperty("currentIndex", 0);
        }
        m_udpSocketLocalPortSpinBox->setProperty("value", 0);
        m_udpSocketRemoteHostTextField->setProperty("text", "");
        m_udpSocketRemotePortSpinBox->setProperty("value", 0);
        // video stream
        if (m_videoStreamNameComboBox->property("count").toInt()) {
            m_videoStreamNameComboBox->setProperty("currentIndex", 0);
        }
        // bluetooth le
        m_bluetoothNameTextField->setProperty("text", "");
        m_bluetoothAdapterComboBox->setProperty("currentIndex", -1);
        m_bluetoothPeripheralComboBox->setProperty("currentIndex", -1);
        m_bluetoothServiceComboBox->setProperty("currentIndex", -1);
        m_bluetoothTxCharacteristicComboBox->setProperty("currentIndex", -1);
        m_bluetoothRxCharacteristicComboBox->setProperty("currentIndex", -1);
        m_bluetoothWriteTypeComboBox->setProperty("currentValue", "request");
        m_bluetoothSubscribeTypeComboBox->setProperty("currentValue", "notify");
        m_bluetoothStatusLabel->setProperty("text", "");
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
                m_serialPortNameComboBox->setProperty("currentValue", portConfig["portName"].toString());
                m_serialPortBaudRateSpinBox->setProperty("value", portConfig["baudRate"].toInt());
                m_serialPortDataBitsComboBox->setProperty("currentValue", portConfig["dataBits"].toInt());
                m_serialPortParityComboBox->setProperty("currentValue", portConfig["parity"].toInt());
                m_serialPortStopBitsComboBox->setProperty("currentValue", portConfig["stopBits"].toInt());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::Visa: {
                m_visaNameComboBox->setProperty("currentValue", portConfig["portName"].toString());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
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
                m_webSocketClientMessageTypeComboBox->setProperty("currentValue", portConfig["messageType"].toString());
                m_logFormatComboBox->setProperty("currentValue", portConfig["logFormat"].toString());
                m_txSuffixComboBox->setProperty("currentValue", portConfig["txSuffix"].toString());
                m_bufferSizeSpinBox->setProperty("value", portConfig["bufferSize"].toInt());
            }
            break;
            case PortType::WebSocketServer: {
                m_webSocketServerNameTextField->setProperty("text", portConfig["portName"].toString());
                m_webSocketServerLocalHostComboBox->setProperty("currentValue", portConfig["localHost"].toString());
                m_webSocketServerLocalPortSpinBox->setProperty("value", portConfig["localPort"].toInt());
                m_webSocketServerSecureSwitch->setProperty("checked", portConfig["secure"].toBool());
                m_webSocketServerCertificateTextField->setProperty("text", portConfig["certificate"].toString());
                m_webSocketServerPrivateKeyTextField->setProperty("text", portConfig["privateKey"].toString());
                m_webSocketServerMessageTypeComboBox->setProperty("currentValue", portConfig["messageType"].toString());
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
            case PortType::VideoStream: {
                m_videoStreamNameComboBox->setProperty("currentValue", portConfig["portName"].toString());
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
            case PortType::BluetoothLe: {
                const auto adapterName = portConfig["adapterName"].toString();
                const auto adapterAddress = portConfig["adapterAddress"].toString();
                auto *adapterItem = new QStandardItem(adapterName.isEmpty() ? adapterAddress : QString("%1 [%2]").arg(adapterName, adapterAddress)); // NOLINT
                adapterItem->setData(adapterName, Qt::UserRole);
                adapterItem->setData(adapterAddress, Qt::WhatsThisRole);
                m_bluetoothAdapterStandardItemModel->appendRow(adapterItem);

                const auto peripheralName = portConfig["peripheralName"].toString();
                const auto peripheralAddress = portConfig["peripheralAddress"].toString();
                auto *peripheralItem = new QStandardItem(peripheralName.isEmpty() ? peripheralAddress : QString("%1 [%2]").arg(peripheralName, peripheralAddress)); // NOLINT
                peripheralItem->setData(peripheralName, Qt::UserRole);
                peripheralItem->setData(peripheralAddress, Qt::WhatsThisRole);
                m_bluetoothPeripheralStandardItemModel->appendRow(peripheralItem);

                const auto serviceUuid = portConfig["serviceUuid"].toString();
                auto *serviceItem = new QStandardItem(serviceUuid); // NOLINT
                serviceItem->setData(serviceUuid, Qt::WhatsThisRole);
                m_bluetoothServiceStandardItemModel->appendRow(serviceItem);

                const auto txCharacteristicUuid = portConfig["txCharacteristicUuid"].toString();
                auto *txItem = new QStandardItem(txCharacteristicUuid); // NOLINT
                txItem->setData(txCharacteristicUuid, Qt::WhatsThisRole);
                m_bluetoothTxCharacteristicStandardItemModel->appendRow(txItem);

                const auto rxCharacteristicUuid = portConfig["rxCharacteristicUuid"].toString();
                auto *rxItem = new QStandardItem(rxCharacteristicUuid); // NOLINT
                rxItem->setData(rxCharacteristicUuid, Qt::WhatsThisRole);
                m_bluetoothRxCharacteristicStandardItemModel->appendRow(rxItem);

                m_bluetoothNameTextField->setProperty("text", portConfig["portName"].toString());
                m_bluetoothAdapterComboBox->setProperty("currentValue", adapterAddress);
                m_bluetoothPeripheralComboBox->setProperty("currentValue", peripheralAddress);
                m_bluetoothServiceComboBox->setProperty("currentValue", serviceUuid);
                m_bluetoothTxCharacteristicComboBox->setProperty("currentValue", txCharacteristicUuid);
                m_bluetoothRxCharacteristicComboBox->setProperty("currentValue", rxCharacteristicUuid);
                m_bluetoothWriteTypeComboBox->setProperty("currentValue", portConfig["writeType"].toString("request"));
                m_bluetoothSubscribeTypeComboBox->setProperty("currentValue", portConfig["subscribeType"].toString("notify"));
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
                {"portName", m_serialPortNameComboBox->property("currentValue").toString()},
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
        case PortType::Visa: {
            portConfig = {
                {"portType", portType},
                {"portName", m_visaNameComboBox->property("currentValue").toString()},
                {"logFormat", m_logFormatComboBox->property("currentValue").toString()},
                {"txSuffix", m_txSuffixComboBox->property("currentValue").toString()},
                {"bufferSize", m_bufferSizeSpinBox->property("value").toInt()}
            };
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
                {"localHost", m_tcpServerLocalHostComboBox->property("currentValue").toString()},
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
                {"localHost", m_sslServerLocalHostComboBox->property("currentValue").toString()},
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
                {"messageType", m_webSocketClientMessageTypeComboBox->property("currentValue").toString()},
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
                {"localHost", m_webSocketServerLocalHostComboBox->property("currentValue").toString()},
                {"localPort", m_webSocketServerLocalPortSpinBox->property("value").toInt()},
                {"secure", m_webSocketServerSecureSwitch->property("checked").toBool()},
                {"certificate", m_webSocketServerCertificateTextField->property("text").toString()},
                {"privateKey", m_webSocketServerPrivateKeyTextField->property("text").toString()},
                {"messageType", m_webSocketServerMessageTypeComboBox->property("currentValue").toString()},
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
        case PortType::VideoStream: {
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
                {"portName", m_videoStreamNameComboBox->property("currentValue").toString()},
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
        case PortType::BluetoothLe: {
            const auto *adapterItem = m_bluetoothAdapterStandardItemModel->item(m_bluetoothAdapterComboBox->property("currentIndex").toInt());
            const auto *peripheralItem = m_bluetoothPeripheralStandardItemModel->item(m_bluetoothPeripheralComboBox->property("currentIndex").toInt());
            portConfig = {
                {"portType", portType},
                {"portName", m_bluetoothNameTextField->property("text").toString()},
                {"adapterName", adapterItem->data(Qt::UserRole).toString()},
                {"adapterAddress", m_bluetoothAdapterComboBox->property("currentValue").toString()},
                {"peripheralName", peripheralItem->data(Qt::UserRole).toString()},
                {"peripheralAddress", m_bluetoothPeripheralComboBox->property("currentValue").toString()},
                {"serviceUuid", m_bluetoothServiceComboBox->property("currentValue").toString()},
                {"txCharacteristicUuid", m_bluetoothTxCharacteristicComboBox->property("currentValue").toString()},
                {"rxCharacteristicUuid", m_bluetoothRxCharacteristicComboBox->property("currentValue").toString()},
                {"writeType", m_bluetoothWriteTypeComboBox->property("currentValue").toString()},
                {"subscribeType", m_bluetoothSubscribeTypeComboBox->property("currentValue").toString()},
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

void PortSetting::bluetoothScan(const QString &adapterAddress) {
    if (adapterAddress.isEmpty()) return;
    m_bluetoothConfig.remove("peripheralName");
    m_bluetoothConfig.remove("peripheralAddress");
    m_bluetoothConfig.remove("serviceUuid");
    m_bluetoothConfig.remove("txCharacteristicUuid");
    m_bluetoothConfig.remove("rxCharacteristicUuid");
    m_bluetoothServices.clear();
    m_bluetoothPeripheralStandardItemModel->clear();
    m_bluetoothServiceStandardItemModel->clear();
    m_bluetoothTxCharacteristicStandardItemModel->clear();
    m_bluetoothRxCharacteristicStandardItemModel->clear();
    QMetaObject::invokeMethod(m_bluetoothDiscovery, [this, adapterAddress] { m_bluetoothDiscovery->scan(adapterAddress); }, Qt::QueuedConnection);
}

void PortSetting::bluetoothDiscover(const QString &adapterAddress, const QString &peripheralAddress) {
    if (adapterAddress.isEmpty() || peripheralAddress.isEmpty()) return;
    m_bluetoothConfig.remove("serviceUuid");
    m_bluetoothConfig.remove("txCharacteristicUuid");
    m_bluetoothConfig.remove("rxCharacteristicUuid");
    m_bluetoothServices.clear();
    m_bluetoothServiceStandardItemModel->clear();
    m_bluetoothTxCharacteristicStandardItemModel->clear();
    m_bluetoothRxCharacteristicStandardItemModel->clear();
    QMetaObject::invokeMethod(m_bluetoothDiscovery, [this, adapterAddress, peripheralAddress] { m_bluetoothDiscovery->discover(adapterAddress, peripheralAddress); }, Qt::QueuedConnection);
}

void PortSetting::bluetoothServiceSelect(const QString &serviceUuid) {
    if (m_bluetoothServices.isEmpty()) return;
    const auto previousTx = m_bluetoothTxCharacteristicComboBox->property("currentValue").toString();
    const auto previousRx = m_bluetoothRxCharacteristicComboBox->property("currentValue").toString();
    m_bluetoothTxCharacteristicStandardItemModel->clear();
    m_bluetoothRxCharacteristicStandardItemModel->clear();
    for (const auto &value: m_bluetoothServices) {
        const auto service = value.toHash();
        if (service["uuid"].toString() != serviceUuid) continue;
        for (const auto &characteristicValue: service["characteristics"].toList()) {
            const auto characteristic = characteristicValue.toHash();
            const auto uuid = characteristic["uuid"].toString();
            if (characteristic["writeRequest"].toBool() || characteristic["writeCommand"].toBool()) {
                QStringList types{};
                if (characteristic["writeRequest"].toBool()) types.append("request");
                if (characteristic["writeCommand"].toBool()) types.append("command");
                auto *item = new QStandardItem(QString("%1 [%2]").arg(uuid, types.join(", "))); // NOLINT
                item->setData(uuid, Qt::WhatsThisRole);
                m_bluetoothTxCharacteristicStandardItemModel->appendRow(item);
            }
            if (characteristic["notify"].toBool() || characteristic["indicate"].toBool()) {
                QStringList types{};
                if (characteristic["notify"].toBool()) types.append("notify");
                if (characteristic["indicate"].toBool()) types.append("indicate");
                auto *item = new QStandardItem(QString("%1 [%2]").arg(uuid, types.join(", "))); // NOLINT
                item->setData(uuid, Qt::WhatsThisRole);
                m_bluetoothRxCharacteristicStandardItemModel->appendRow(item);
            }
        }
        break;
    }

    const bool configuredService = m_bluetoothConfig["serviceUuid"].toString() == serviceUuid;
    m_bluetoothTxCharacteristicComboBox->setProperty("currentValue", configuredService ? m_bluetoothConfig["txCharacteristicUuid"].toString() : previousTx);
    m_bluetoothRxCharacteristicComboBox->setProperty("currentValue", configuredService ? m_bluetoothConfig["rxCharacteristicUuid"].toString() : previousRx);
    if (m_bluetoothTxCharacteristicComboBox->property("currentIndex").toInt() < 0 && m_bluetoothTxCharacteristicComboBox->property("count").toInt()) m_bluetoothTxCharacteristicComboBox->setProperty("currentIndex", 0);
    if (m_bluetoothRxCharacteristicComboBox->property("currentIndex").toInt() < 0 && m_bluetoothRxCharacteristicComboBox->property("count").toInt()) m_bluetoothRxCharacteristicComboBox->setProperty("currentIndex", 0);
}

void PortSetting::videoCapture() {
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
    const auto &portName = m_videoStreamNameComboBox->property("currentValue").toString();
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
void PortSetting::serialPortRefresh() const {
    m_serialPortStandardItemModel->clear();
    for (const auto &portName: DeviceDiscovery::serialPorts()) {
        auto *item = new QStandardItem(portName); // NOLINT
        item->setData(portName, Qt::WhatsThisRole);
        m_serialPortStandardItemModel->appendRow(item);
    }
}

void PortSetting::visaRefresh() const {
    ViFindList findList;
    ViUInt32 numInst;
    ViChar portName[VI_FIND_BUFLEN];
    // resource manager check
    ViStatus status = viOpenDefaultRM(&g_rm);
    if (status != VI_SUCCESS) {
        qDebug() << "failed to start visa resource manager";
        return;
    }
    // device check
    status = viFindRsrc(g_rm, "?*INSTR", &findList, &numInst, portName);
    if (status != VI_SUCCESS) {
        qDebug() << "failed to find visa instruments";
        viClose(g_rm);
        return;
    }
    // standard item model construct
    m_visaStandardItemModel->clear();
    // first device
    auto *firstItem = new QStandardItem(QString(portName)); // NOLINT
    firstItem->setData(QString(portName), Qt::WhatsThisRole);
    m_visaStandardItemModel->appendRow(firstItem);
    // following device
    for (ViUInt32 i = 1; i < numInst; i++) {
        status = viFindNext(findList, portName);
        if (status == VI_SUCCESS) {
            auto *followingItem = new QStandardItem(QString(portName)); // NOLINT
            followingItem->setData(QString(portName), Qt::WhatsThisRole);
            m_visaStandardItemModel->appendRow(followingItem);
        }
    }
    // free resource
    viClose(findList);
    viClose(g_rm);
}

void PortSetting::localHostRefresh() const {
    m_localHostStandardItemModel->clear();
    for (const QHostAddress &address: QHostInfo::fromName(QHostInfo::localHostName()).addresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            const QString portName = address.toString();
            auto *item = new QStandardItem(portName); // NOLINT
            item->setData(portName, Qt::WhatsThisRole);
            m_localHostStandardItemModel->appendRow(item);
        } else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
            auto portName = address.toString();
            // const QString scopeId = address.scopeId();
            // if (!scopeId.isEmpty()) portName.remove("%"+scopeId);
            auto *item = new QStandardItem(portName); // NOLINT
            item->setData(portName, Qt::WhatsThisRole);
            m_localHostStandardItemModel->appendRow(item);
        }
    }
}

void PortSetting::videoStreamRefresh() const {
    m_videoStreamStandardItemModel->clear();
    for (const auto &portName: DeviceDiscovery::screens()) {
        auto *item = new QStandardItem(portName); // NOLINT
        item->setData(portName, Qt::WhatsThisRole);
        m_videoStreamStandardItemModel->appendRow(item);
    }
    for (const auto &portName: DeviceDiscovery::cameras()) {
        auto *item = new QStandardItem(portName); // NOLINT
        item->setData(portName, Qt::WhatsThisRole);
        m_videoStreamStandardItemModel->appendRow(item);
    }
}

void PortSetting::bluetoothAdapterRefresh() const {
    QMetaObject::invokeMethod(m_bluetoothDiscovery, [this] { m_bluetoothDiscovery->adaptersRefresh(); }, Qt::QueuedConnection);
}

void PortSetting::bluetoothAdaptersUpdate(const QVariantList &adapters) {
    const auto configuredAddress = m_bluetoothConfig["adapterAddress"].toString();
    QString selectedAddress{};
    m_bluetoothAdapterStandardItemModel->clear();
    for (const auto &value: adapters) {
        const auto adapter = value.toHash();
        const auto name = adapter["name"].toString();
        const auto address = adapter["address"].toString();
        auto *item = new QStandardItem(name.isEmpty() ? address : QString("%1 [%2]").arg(name, address)); // NOLINT
        item->setData(name, Qt::UserRole);
        item->setData(address, Qt::WhatsThisRole);
        m_bluetoothAdapterStandardItemModel->appendRow(item);
        if (address.compare(configuredAddress, Qt::CaseInsensitive) == 0) selectedAddress = address;
    }
    if (selectedAddress.isEmpty() && !configuredAddress.isEmpty()) {
        const auto name = m_bluetoothConfig["adapterName"].toString();
        auto *item = new QStandardItem(name.isEmpty() ? configuredAddress : QString("%1 [%2]").arg(name, configuredAddress)); // NOLINT
        item->setData(name, Qt::UserRole);
        item->setData(configuredAddress, Qt::WhatsThisRole);
        m_bluetoothAdapterStandardItemModel->appendRow(item);
        selectedAddress = configuredAddress;
    }
    if (!selectedAddress.isEmpty()) m_bluetoothAdapterComboBox->setProperty("currentValue", selectedAddress);
    else if (m_bluetoothAdapterComboBox->property("count").toInt()) m_bluetoothAdapterComboBox->setProperty("currentIndex", 0);
}

void PortSetting::bluetoothPeripheralsUpdate(const QVariantList &peripherals) {
    const auto configuredAddress = m_bluetoothConfig["peripheralAddress"].toString();
    QString selectedAddress{};
    m_bluetoothPeripheralStandardItemModel->clear();
    for (const auto &value: peripherals) {
        const auto peripheral = value.toHash();
        const auto name = peripheral["name"].toString();
        const auto address = peripheral["address"].toString();
        QString display = name.isEmpty() ? address : QString("%1 [%2]").arg(name, address);
        display += QString(" (%1 dBm)").arg(peripheral["rssi"].toInt());
        auto *item = new QStandardItem(display); // NOLINT
        item->setData(name, Qt::UserRole);
        item->setData(address, Qt::WhatsThisRole);
        m_bluetoothPeripheralStandardItemModel->appendRow(item);
        if (address.compare(configuredAddress, Qt::CaseInsensitive) == 0) selectedAddress = address;
    }
    if (!selectedAddress.isEmpty()) m_bluetoothPeripheralComboBox->setProperty("currentValue", selectedAddress);
    else if (m_bluetoothPeripheralComboBox->property("count").toInt()) m_bluetoothPeripheralComboBox->setProperty("currentIndex", 0);
}

void PortSetting::bluetoothServicesUpdate(const QVariantList &services) {
    m_bluetoothServices = services;
    const auto configuredUuid = m_bluetoothConfig["serviceUuid"].toString();
    QString selectedUuid{};
    m_bluetoothServiceStandardItemModel->clear();
    for (const auto &value: services) {
        const auto uuid = value.toHash()["uuid"].toString();
        auto *item = new QStandardItem(uuid); // NOLINT
        item->setData(uuid, Qt::WhatsThisRole);
        m_bluetoothServiceStandardItemModel->appendRow(item);
        if (uuid.compare(configuredUuid, Qt::CaseInsensitive) == 0) selectedUuid = uuid;
    }
    if (!selectedUuid.isEmpty()) m_bluetoothServiceComboBox->setProperty("currentValue", selectedUuid);
    else if (m_bluetoothServiceComboBox->property("count").toInt()) m_bluetoothServiceComboBox->setProperty("currentIndex", 0);
    bluetoothServiceSelect(m_bluetoothServiceComboBox->property("currentValue").toString());
}

void PortSetting::bluetoothStatusUpdate(const QString &status) const {
    if (m_bluetoothStatusLabel) m_bluetoothStatusLabel->setProperty("text", status);
}

void PortSetting::bluetoothBusyUpdate(const bool busy) const {
    if (m_root) m_root->setProperty("bluetoothBusy", busy);
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
    const auto results = m_imageProcess.detail(image);
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
