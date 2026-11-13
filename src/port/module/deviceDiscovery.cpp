#include "port/module/deviceDiscovery.h"

#include <QCameraDevice>
#include <QMediaDevices>
#include <QScreen>
#include <QScopeGuard>
#include <QSerialPortInfo>
#include <QTimer>
#include <simpleble/SimpleBLE.h>

#include <exception>
#include <vector>

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

void DeviceDiscovery::Bluetooth::peripherals(const std::function<void(QVariantHash)> &callback) {
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
            {"display", name.isEmpty() ? address : QString("%1 \u00b7 %2").arg(address, name)}
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
