#include "port/portManageWindow.h"

#include <QCamera>
#include <QCameraDevice>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreenCapture>
#include <QtConcurrentRun>
#include <simpleble/Peripheral.h>

#include <exception>
#include <utility>

#include "globals.h"
#include "mainWindow/mainWindow.h"
#include "core/globalManager.h"
#include "port/module/deviceDiscovery.h"

// public
PortManageWindow::PortManageWindow()
    : QQuickView(g_globalManager->engineGet(), nullptr),
      m_bluetoothServices(new QStandardItemModel(this)),
      m_mediaCaptureSession(new QMediaCaptureSession(this)),
      m_imageProvider(new ImageProvider()) {
    m_bluetoothServices->setItemRoleNames({
        {Qt::DisplayRole, "uuid"},
        {Qt::UserRole + 1, "capabilities"},
        {Qt::UserRole + 2, "name"},
        {Qt::UserRole + 3, "shortUuid"}
    });
}

PortManageWindow::~PortManageWindow() {
    m_bleScanFuture.waitForFinished();
    m_bleConnectFuture.waitForFinished();
}

void PortManageWindow::propertySet() {
    setTitle(tr("Manage Port"));
    setTransientParent(g_globalManager->mainWindowGet()->windowHandle());
    engine()->addImageProvider("capture", m_imageProvider);

    setInitialProperties({{"portManageWindow", QVariant::fromValue(this)}});
    setResizeMode(SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/port/portManageWindow.qml"));
}

void PortManageWindow::open(const QJsonObject &portConfig) {
    serialPortRefresh();
    localHostRefresh();
    visionRefresh();
    bleRefresh();
    m_oldPortName = portConfig["portName"].toString();
    if (portConfig.isEmpty()) {
        m_config.clear();
        typeSelect(PortType::SerialPort);
    } else {
        m_config = portConfig.toVariantHash();
        const auto type = static_cast<PortType::Value>(portConfig["portType"].toInt());
        if (type == PortType::BluetoothLe) {
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
            m_config["peripheralAddress"] = address;
        }
        emit configChanged();
    }
    rootObject()->setProperty("pageIndex", portConfig.isEmpty() ? 0 : 1);
    resize(600, 500);
    show();
}

void PortManageWindow::typeSelect(const PortType::Value type) {
    if (!m_config.isEmpty() && m_config.value("portType").toInt() == type) return;
    m_config = {{"portType", type}};
    if (type != PortType::Vision) {
        m_config.insert("logFormat", "utf-8");
        m_config.insert("txSuffix", "null");
        m_config.insert("bufferSize", 65536);
    }
    switch (type) {
        case PortType::SerialPort: {
            const auto device = m_serialPorts.value(0).toHash();
            const auto deviceName = device.value("deviceName").toString();
            const auto name = device.value("name").toString();
            m_config.insert("deviceName", deviceName);
            m_config.insert("portName", name.isEmpty() ? deviceName : name);
            break;
        }
        case PortType::Vision: {
            const auto deviceName = m_visionSources.value(0);
            m_config.insert("deviceName", deviceName);
            m_config.insert("portName", deviceName);
            m_config.insert("rois", QVariantList{});
            m_config.insert("pipeline", QVariantList{});
            m_config.insert("recognition", QVariantHash{{"mode", RecognitionMethod::OCR}});
            break;
        }
        case PortType::TcpClient:
        case PortType::SslClient:
        case PortType::WebSocketClient:
            m_config.insert("remoteHost", "");
            m_config.insert("remotePort", type == PortType::SslClient ? 443 : type == PortType::WebSocketClient ? 80 : 0);
            break;
        case PortType::TcpServer:
        case PortType::SslServer:
        case PortType::WebSocketServer:
        case PortType::UdpSocket:
            m_config.insert("localHost", m_localHosts.value(0));
            m_config.insert("localPort", type == PortType::SslServer ? 443 : type == PortType::WebSocketServer ? 80 : 0);
            if (type == PortType::UdpSocket) {
                m_config.insert("remoteHost", "");
                m_config.insert("remotePort", 0);
            }
            break;
        case PortType::BluetoothLe:
            m_config.insert("adapterAddress", "");
            m_config.insert("peripheralAddress", "");
            break;
        default: break;
    }
    emit configChanged();
}

void PortManageWindow::save() {
    QJsonObject portConfig = QJsonObject::fromVariantHash(m_config);
    if (portConfig["portType"].toInt() == PortType::Vision) {
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
        const auto draft = portConfig.value("recognition").toObject();
        const auto mode = draft.value("mode").toInt(RecognitionMethod::OCR);
        QJsonObject recognition{{"mode", mode}};
        if (mode == RecognitionMethod::TemplateMatch) {
            const auto templateUrl = draft.value("template").toString();
            if (templateUrl.isEmpty()) return;
            recognition["template"] = templateUrl;
        }
        portConfig = {
            {"portType", PortType::Vision},
            {"portName", portConfig.value("portName")},
            {"deviceName", portConfig.value("deviceName")},
            {"rois", portConfig.value("rois").toArray()},
            {"pipeline", portConfig.value("pipeline").toArray()},
            {"recognition", recognition}
        };
    }
    if (m_oldPortName.isEmpty()) {
        emit insertPort(-1, portConfig, {});
    } else {
        emit editPort(m_oldPortName, portConfig);
    }
    hide();
}

void PortManageWindow::bleConnect(const QVariant &peripheral) {
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

void PortManageWindow::visionCapture(QVideoSink *videoSink) {
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
    m_mediaCaptureSession->setVideoSink(videoSink);
    const auto deviceName = m_config.value("deviceName").toString();
    for (QScreen *screen: QGuiApplication::screens()) {
        if (deviceName == screen->name()) {
            m_screenCapture = new QScreenCapture(this);
            m_screenCapture->setScreen(screen);
            m_mediaCaptureSession->setScreenCapture(m_screenCapture);
            m_screenCapture->start();
            return;
        }
    }
    for (const QCameraDevice &camera: QMediaDevices::videoInputs()) {
        if (deviceName == camera.description()) {
            m_cameraCapture = new QCamera(camera, this);
            m_mediaCaptureSession->setCamera(m_cameraCapture);
            m_cameraCapture->start();
            return;
        }
    }
}

QVariantHash PortManageWindow::previewLoad(const int index) const {
    const auto rois = m_config.value("rois").toList();
    if (index < 0 || index >= rois.size()) return {{"source", "qrc:/icon/null.svg"}, {"recognition", QVariantHash{}}};
    const auto recognition = QJsonObject::fromVariantHash(m_config.value("recognition").toHash());
    const auto mode = recognition.value("mode").toInt(RecognitionMethod::OCR);
    if (mode < RecognitionMethod::OCR || mode > RecognitionMethod::TemplateMatch) return {};
    if (mode == RecognitionMethod::TemplateMatch && recognition.value("template").toString().isEmpty()) return {};
    const QJsonObject config{
        {"rois", QJsonArray{QJsonValue::fromVariant(rois.at(index))}},
        {"pipeline", QJsonArray::fromVariantList(m_config.value("pipeline").toList())},
        {"recognition", recognition}
    };
    m_imageProvider->preview(m_mediaCaptureSession->videoSink(), config);
    return {
        {"source", "image://capture/" + QString::number(QDateTime::currentMSecsSinceEpoch())},
        {"recognition", m_imageProvider->recognition()}
    };
}

// private
QVariantList PortManageWindow::serialPortsGet() const {
    return m_serialPorts;
}

QStringList PortManageWindow::localHostsGet() const {
    return m_localHosts;
}

QStringList PortManageWindow::visionSourcesGet() const {
    return m_visionSources;
}

QVariantList PortManageWindow::bluetoothPeripheralsGet() const {
    return m_bluetoothPeripherals;
}

QAbstractItemModel *PortManageWindow::bluetoothServicesGet() const {
    return m_bluetoothServices;
}

QString PortManageWindow::bluetoothServiceAddressGet() const {
    return m_bluetoothServiceAddress;
}

void PortManageWindow::serialPortRefresh() {
    const auto serialPorts = DeviceDiscovery::serialPorts();
    if (m_serialPorts == serialPorts) return;
    m_serialPorts = serialPorts;
    emit serialPortsChanged();
}

void PortManageWindow::localHostRefresh() {
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

void PortManageWindow::visionRefresh() {
    const auto visionSources = DeviceDiscovery::screens() + DeviceDiscovery::cameras();
    if (m_visionSources == visionSources) return;
    m_visionSources = visionSources;
    emit visionSourcesChanged();
}

void PortManageWindow::bleRefresh() {
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

// public
ImageProvider::ImageProvider()
    : QQuickImageProvider(Pixmap) {
}

QPixmap ImageProvider::requestPixmap(const QString &, QSize *, const QSize &) {
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
