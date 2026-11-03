#ifndef UNICOMM_MQTT_H
#define UNICOMM_MQTT_H

#include <QObject>
#include <QPointer>

#include <string>

class BasePort;

class Mqtt final : public QObject {
    Q_OBJECT

public:
    explicit Mqtt(QObject *parent = nullptr);

    ~Mqtt() override = default;

    void init(const std::string &portName, int timeout);

private:
    std::string m_portName{};
    int m_timeout{};
    QPointer<BasePort> m_port{};
};

#endif //UNICOMM_MQTT_H
