#ifndef UNICOMM_DEVICEDISCOVERY_H
#define UNICOMM_DEVICEDISCOVERY_H

#include <QStringList>
#include <QVariantHash>
#include <QVariantList>

#include <functional>

namespace DeviceDiscovery {
    [[nodiscard]] QVariantList serialPorts();

    [[nodiscard]] QStringList screens();

    [[nodiscard]] QStringList cameras();
}

namespace DeviceDiscovery::Bluetooth {
    void peripherals(const std::function<void(QVariantHash)> &callback);
}

#endif //UNICOMM_DEVICEDISCOVERY_H
