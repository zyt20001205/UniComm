#ifndef UNICOMM_IMAP_H
#define UNICOMM_IMAP_H

#include <QPointer>
#include <QVariantHash>
#include <sol/object.hpp>
#include <variant>

#include "types/logTypes.h"

class BasePort;

class Imap final : public QObject {
    Q_OBJECT

public:
    explicit Imap(QObject *parent = nullptr);

    ~Imap() override = default;

    void init(const std::string &portName, int timeout);

    void login(const std::string &username, const std::string &password);

    [[nodiscard]] sol::object receive(sol::this_state ts, const sol::optional<std::string> &from,
                                      sol::optional<int> timeout);

    void logout();

signals:
    void appendLog(LogType::Value type, const QString &prefix, const QString &message);

private:
    struct Untagged {
        QByteArray value{};
    };

    struct Continuation {
        QByteArray value{};
    };

    struct Tagged {
        QByteArray tag{};
        QByteArray code{};
        QByteArray text{};
    };

    struct Result {
        std::variant<std::monostate, Untagged, Continuation, Tagged> value{};
        QString exception{};
    };

    struct MailBody {
        QString contentType{};
        QByteArray data{};
    };

    struct MailAttachment {
        QString name{};
        QString contentType{};
        QByteArray data{};
    };

    struct Mail {
        QVariantHash header{};
        QList<MailBody> body{};
        QList<MailAttachment> attachments{};
    };

    [[nodiscard]] static Result parser(const QByteArray &rxData);

    [[nodiscard]] QByteArray nextTag();

    [[nodiscard]] Mail mailParser(const QByteArray &rxData);

    [[nodiscard]] QByteArray rfc2047Parser(const QByteArray &text);

    std::string m_portName{};
    int m_timeout{};
    QPointer<BasePort> m_port{};
    int m_count{};
};

#endif //UNICOMM_IMAP_H
