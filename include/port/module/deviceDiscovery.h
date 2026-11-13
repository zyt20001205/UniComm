#ifndef UNICOMM_DEVICEDISCOVERY_H
#define UNICOMM_DEVICEDISCOVERY_H

#include <QVariantList>

#include <functional>

namespace DeviceDiscovery {
    [[nodiscard]] QVariantList serialPorts();

    [[nodiscard]] QStringList screens();

    [[nodiscard]] QStringList cameras();
}

namespace DeviceDiscovery::Bluetooth {
    void scan(const std::function<void(QVariantHash)> &callback);

    [[nodiscard]] QVariantList connect(const QVariant &peripheral);
}

#endif //UNICOMM_DEVICEDISCOVERY_H
