#ifndef UNICOMM_PORT_H
#define UNICOMM_PORT_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <sol/object.hpp>
#include <sol/table.hpp>

class BasePort;

class Port final : public QObject {
    Q_OBJECT

public:
    explicit Port(QString portName, QObject *parent = nullptr);

    ~Port() override = default;

    [[nodiscard]] static sol::table list(sol::this_state ts);

    [[nodiscard]] static Port *create(const sol::table &config, QObject *parent);

    [[nodiscard]] static Port *get(const std::string &portName, QObject *parent);

    static void remove(const std::string &portName);

    [[nodiscard]] sol::object info(sol::this_state ts) const;

    void open() const;

    void close() const;

    void clear() const;

    void write(const std::string &data, int messageType, const std::string &peerIp) const;

    void write(const std::string &data, int mode, const std::string &serviceUuid, const std::string &characteristicUuid) const;

    [[nodiscard]] sol::object read(sol::this_state ts, int length, int timeout, const std::string &peerIp, bool peek = false) const;

    [[nodiscard]] sol::object read(sol::this_state ts, const std::string &serviceUuid, const std::string &characteristicUuid) const;

    [[nodiscard]] sol::object read(sol::this_state ts, int length, int timeout, const std::string &serviceUuid, const std::string &characteristicUuid, bool peek = false) const;

    [[nodiscard]] sol::object readUntil(sol::this_state ts, const std::string &text, int timeout, const std::string &peerIp, bool peek = false) const;

    [[nodiscard]] sol::object readUntil(sol::this_state ts, const std::string &text, int timeout, const std::string &serviceUuid, const std::string &characteristicUuid, bool peek = false) const;

    void subscribe(int mode, const std::string &serviceUuid, const std::string &characteristicUuid) const;

    void unsubscribe(const std::string &serviceUuid, const std::string &characteristicUuid) const;

private:
    QString m_portName{};
    QPointer<BasePort> m_port{};
};

#endif //UNICOMM_PORT_H
