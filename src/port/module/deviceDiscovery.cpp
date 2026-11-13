#include "port/module/deviceDiscovery.h"

#include <QCameraDevice>
#include <QEventLoop>
#include <QMediaDevices>
#include <QScreen>
#include <QScopeGuard>
#include <QSerialPortInfo>
#include <QStringList>
#include <QTimer>
#include <simpleble/SimpleBLE.h>

#include <exception>
#include <vector>

#include "util/uniCast.h"

QVariantList DeviceDiscovery::serialPorts() {
    QVariantList devices{};
    for (const auto &port: QSerialPortInfo::availablePorts()) {
        const auto deviceName = port.portName();
        const auto name = port.description();
        devices.append(QVariantHash{
            {"deviceName", deviceName},
            {"name", name},
            {"display", name.isEmpty() ? deviceName : QString("%1 \u00b7 %2").arg(deviceName, name)}
        });
    }
    return devices;
}

QStringList DeviceDiscovery::screens() {
    QStringList devices{};
    for (const auto *screen: QGuiApplication::screens()) devices.append(screen->name());
    return devices;
}

QStringList DeviceDiscovery::cameras() {
    QStringList devices{};
    for (const auto &camera: QMediaDevices::videoInputs()) devices.append(camera.description());
    return devices;
}

void DeviceDiscovery::Bluetooth::scan(const std::function<void(QVariantHash)> &callback) {
    std::vector<SimpleBLE::Adapter> scanning{};
    const auto cleanup = qScopeGuard([&scanning] {
        for (auto &adapter: scanning) {
            adapter.set_callback_on_scan_found(nullptr);
            adapter.set_callback_on_scan_updated(nullptr);
            try {
                if (adapter.scan_is_active()) adapter.scan_stop();
            } catch (const std::exception &exception) {
                qWarning("Bluetooth scan stop failed: %s", exception.what());
            } catch (...) {
                qWarning("Bluetooth scan stop failed");
            }
        }
    });
    const auto discovered = [callback](SimpleBLE::Peripheral peripheral) {
        const auto address = QString::fromStdString(peripheral.address());
        const auto name = QString::fromStdString(peripheral.identifier());
        callback(QVariantHash{
            {"name", name},
            {"address", address},
            {"display", name.isEmpty() ? address : QString("%1 \u00b7 %2").arg(address, name)},
            {"peripheral", QVariant::fromValue(peripheral)}
        });
    };
    for (auto adapter: SimpleBLE::Adapter::get_adapters()) {
        if (!adapter.is_powered()) continue;
        scanning.push_back(adapter);
        adapter.set_callback_on_scan_found(discovered);
        adapter.set_callback_on_scan_updated(discovered);
        adapter.scan_start();
    }
    if (scanning.empty()) return;

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();
}

QVariantList DeviceDiscovery::Bluetooth::connect(const QVariant &peripheral) {
    auto device = peripheral.value<SimpleBLE::Peripheral>();
    const bool connectedHere = !device.is_connected();
    const auto cleanup = qScopeGuard([&device, connectedHere] {
        if (!connectedHere) return;
        try {
            device.disconnect();
        } catch (const std::exception &exception) {
            qWarning("Bluetooth disconnect failed: %s", exception.what());
        } catch (...) {
            qWarning("Bluetooth disconnect failed");
        }
    });
    if (connectedHere) device.connect();

    QVariantList services{};
    for (auto service: device.services()) {
        const auto serviceUuid = QString::fromStdString(service.uuid());
        QVariantList characteristics{};
        for (auto characteristic: service.characteristics()) {
            const auto characteristicUuid = QString::fromStdString(characteristic.uuid());
            QStringList capabilities{};
            for (const auto &capability: characteristic.capabilities()) capabilities.append(QString::fromStdString(capability));
            characteristics.append(QVariantHash{
                {"uuid", characteristicUuid},
                {"name", uni_cast<QBluetoothUuidName>(characteristicUuid).value},
                {"capabilities", capabilities}
            });
        }
        services.append(QVariantHash{
            {"uuid", serviceUuid},
            {"name", uni_cast<QBluetoothUuidName>(serviceUuid).value},
            {"characteristics", characteristics}
        });
    }
    return services;
}
