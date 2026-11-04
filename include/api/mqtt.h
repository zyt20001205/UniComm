#ifndef UNICOMM_MQTT_H
#define UNICOMM_MQTT_H

#include <QObject>
#include <QPointer>
#include <QQueue>
#include <sol/object.hpp>
#include <sol/optional.hpp>

#include <optional>
#include <string>

class BasePort;

class Mqtt final : public QObject {
    Q_OBJECT

public:
    struct QoS {
        enum {
            AtMostOnce = 0,
            AtLeastOnce = 1,
            ExactlyOnce = 2
        };
    };

    explicit Mqtt(QObject *parent = nullptr);

    ~Mqtt() override = default;

    void init(const std::string &portName, int timeout);

    void connect(const std::string &clientId, const sol::optional<std::string> &username, const sol::optional<std::string> &password,
                 sol::optional<int> keepAlive, sol::optional<bool> cleanSession);

    [[nodiscard]] int subscribe(const std::string &topicFilter, sol::optional<int> qos);

    void publish(const std::string &topic, const std::string &payload, sol::optional<int> qos, sol::optional<bool> retain) const;

    [[nodiscard]] sol::object consume(sol::this_state ts);

private:
    struct ControlPacketType {
        // https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
        enum {
            Connect = 1,
            Connack = 2,
            Publish = 3,
            Puback = 4,
            Pubrec = 5,
            Pubrel = 6,
            Pubcomp = 7,
            Subscribe = 8,
            Suback = 9,
            Unsubscribe = 10,
            Unsuback = 11,
            Pingreq = 12,
            Pingresp = 13,
            Disconnect = 14
        };
    };

    struct ConnectReturnCode {
        enum {
            ConnectionAccepted = 0,
            UnacceptableProtocolVersion = 1,
            IdentifierRejected = 2,
            ServerUnavailable = 3,
            BadUsernameOrPassword = 4,
            NotAuthorized = 5
        };
    };

    struct SubscribeReturnCode {
        enum {
            Failure = 0x80
        };
    };

    struct Result {
        int type{};
        quint8 flags{};
        QByteArray data{};
        QString exception{};
    };

    [[nodiscard]] QString request(int type, quint8 flags, const QByteArray &data) const;

    [[nodiscard]] Result response(std::optional<int> type = std::nullopt);

    [[nodiscard]] static Result parser(const QByteArray &rxData);

    std::string m_portName{};
    int m_timeout{};
    QPointer<BasePort> m_port{};
    quint16 m_packetId{};
    QQueue<Result> m_messages{};
};

#endif //UNICOMM_MQTT_H
