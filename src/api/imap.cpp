#include "api/imap.h"

#include <QDeadlineTimer>
#include <QRegularExpression>
#include <limits>
#include <sol/error.hpp>

#include "globals.h"
#include "port/basePort.h"
#include "port/portModule.h"
#include "util/uniCast.h"

// public
Imap::Imap(QObject *parent)
    : QObject(parent) {
}

void Imap::init(const std::string &portName, const int timeout) {
    m_count = 1;
    const auto _portName = QString::fromStdString(portName);
    const auto port = g_port->m_portHash.constFind(_portName);
    if (port == g_port->m_portHash.constEnd()) throw sol::error(portName + " does not exist");
    m_portName = portName;
    m_timeout = timeout;
    m_port = port.value();

    QString exception{};

    QMetaObject::invokeMethod(m_port, [this]() -> QString {
        if (!m_port->open()) return "open failed";

        const auto rxData = m_port->readUntil("\r\n", m_timeout, "utf-8");
        const auto result = parser(rxData);
        if (!result.exception.isEmpty()) return result.exception;
        const auto untagged = std::get_if<Untagged>(&result.value);
        if (!untagged) return "invalid imap greeting";

        const auto space = untagged->value.indexOf(' ');
        if (space == -1) return "invalid imap greeting";
        const auto code = untagged->value.first(space);
        if (code == "OK" || code == "PREAUTH") return {};

        const auto text = QString::fromUtf8(untagged->value.sliced(space + 1));
        if (!text.isEmpty()) return text;
        return "unexpected imap response(" + QString::fromUtf8(code) + ")";
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

void Imap::login(const std::string &username, const std::string &password) {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    QString exception{};
    const auto _username = QByteArray::fromStdString(username);
    const auto _password = QByteArray::fromStdString(password);

    QMetaObject::invokeMethod(m_port, [this, &_username, &_password]() -> QString {
        const auto tag = nextTag();
        const auto txData = tag + " LOGIN " + _username + ' ' + _password;
        if (!m_port->write(txData, "utf-8", "crlf")) return "write failed";

        while (true) {
            const auto rxData = m_port->readUntil("\r\n", m_timeout, "utf-8");
            const auto result = parser(rxData);
            if (!result.exception.isEmpty()) return result.exception;

            if (const auto untagged = std::get_if<Untagged>(&result.value)) {
                const auto space = untagged->value.indexOf(' ');
                const auto code = space == -1 ? untagged->value : untagged->value.first(space);
                if (code != "BYE") continue;
                if (space != -1) return QString::fromUtf8(untagged->value.sliced(space + 1));
                return "server closed connection";
            }

            if (std::get_if<Continuation>(&result.value)) return "unexpected imap continuation";

            const auto tagged = std::get_if<Tagged>(&result.value);
            if (!tagged) return "invalid imap response";
            if (tagged->tag != tag) return "tag mismatch";
            if (tagged->code == "OK") return {};
            if (!tagged->text.isEmpty()) return QString::fromUtf8(tagged->text);
            return "unexpected imap response(" + QString::fromUtf8(tagged->code) + ")";
        }
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

sol::object Imap::receive(const sol::this_state ts, const sol::optional<std::string> &from, const sol::optional<int> timeout) {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    QString exception{};
    Mail mail{};
    const auto _from = QString::fromStdString(from.value_or(""));
    const auto _timeout = timeout.value_or(600000);

    QMetaObject::invokeMethod(m_port, [this, &mail, &_from, _timeout]() -> QString {
        const auto selectTag = nextTag();
        if (!m_port->write(selectTag + " SELECT INBOX", "utf-8", "crlf")) return "write failed";

        while (true) {
            const auto result = parser(m_port->readUntil("\r\n", m_timeout, "utf-8"));
            if (!result.exception.isEmpty()) return result.exception;

            const auto tagged = std::get_if<Tagged>(&result.value);
            if (!tagged) continue;
            if (tagged->tag != selectTag) return "tag mismatch";
            if (tagged->code == "OK") break;
            if (!tagged->text.isEmpty()) return QString::fromUtf8(tagged->text);
            return "unexpected imap response(" + QString::fromUtf8(tagged->code) + ")";
        }

        const QDeadlineTimer deadline(_timeout);

        while (true) {
            const auto idleTag = nextTag();
            if (!m_port->write(idleTag + " IDLE", "utf-8", "crlf")) return "write failed";

            while (true) {
                const auto result = parser(m_port->readUntil("\r\n", m_timeout, "utf-8"));
                if (!result.exception.isEmpty()) return result.exception;
                if (std::get_if<Continuation>(&result.value)) break;

                if (std::get_if<Untagged>(&result.value)) continue;

                if (const auto tagged = std::get_if<Tagged>(&result.value)) {
                    if (tagged->tag != idleTag) return "tag mismatch";
                    if (!tagged->text.isEmpty()) return QString::fromUtf8(tagged->text);
                    return "unexpected imap response(" + QString::fromUtf8(tagged->code) + ")";
                }
            }

            int sequenceNumber{};
            QString waitException{};

            while (sequenceNumber == 0) {
                if (deadline.hasExpired()) {
                    waitException = "read timeout";
                    break;
                }

                const auto result = parser(m_port->readUntil("\r\n", static_cast<int>(deadline.remainingTime()), "utf-8"));
                if (!result.exception.isEmpty()) {
                    waitException = result.exception;
                    break;
                }

                if (const auto untagged = std::get_if<Untagged>(&result.value)) {
                    const auto space = untagged->value.indexOf(' ');
                    if (space == -1) continue;
                    bool numeric{};
                    const auto number = untagged->value.first(space).toInt(&numeric);
                    const auto tail = untagged->value.sliced(space + 1);
                    const auto nextSpace = tail.indexOf(' ');
                    const auto name = nextSpace == -1 ? tail : tail.first(nextSpace);
                    if (numeric && name == "EXISTS") sequenceNumber = number;
                    continue;
                }

                if (const auto tagged = std::get_if<Tagged>(&result.value)) {
                    if (tagged->tag != idleTag) return "tag mismatch";
                    if (!tagged->text.isEmpty()) return QString::fromUtf8(tagged->text);
                    return "unexpected imap response(" + QString::fromUtf8(tagged->code) + ")";
                }
            }

            if (!m_port->write("DONE", "utf-8", "crlf")) return "write failed";

            while (true) {
                const auto result = parser(m_port->readUntil("\r\n", m_timeout, "utf-8"));
                if (!result.exception.isEmpty()) return result.exception;

                const auto tagged = std::get_if<Tagged>(&result.value);
                if (!tagged) continue;
                if (tagged->tag != idleTag) return "tag mismatch";
                if (tagged->code == "OK") break;
                if (!tagged->text.isEmpty()) return QString::fromUtf8(tagged->text);
                return "unexpected imap response(" + QString::fromUtf8(tagged->code) + ")";
            }
            if (!waitException.isEmpty()) return waitException;

            const auto fetchTag = nextTag();
            if (!m_port->write(fetchTag + " FETCH " + QByteArray::number(sequenceNumber) + " BODY.PEEK[]", "utf-8", "crlf")) return "write failed";
            QByteArray rxData{};
            const QDeadlineTimer fetchDeadline(m_timeout);
            while (true) {
                const auto header = m_port->readUntil("\r\n", static_cast<int>(fetchDeadline.remainingTime()), "utf-8", true);
                const auto result = parser(header);
                if (!result.exception.isEmpty()) return result.exception;

                int size{-1};
                if (const auto untagged = std::get_if<Untagged>(&result.value)) {
                    const auto space = untagged->value.indexOf(' ');
                    if (space != -1) {
                        bool numeric{};
                        untagged->value.first(space).toInt(&numeric);
                        const auto tail = untagged->value.sliced(space + 1);
                        const auto nextSpace = tail.indexOf(' ');
                        const auto name = nextSpace == -1 ? tail : tail.first(nextSpace);
                        if (numeric && name == "FETCH") {
                            const auto openingBrace = untagged->value.lastIndexOf('{');
                            const auto closingBrace = untagged->value.indexOf('}', openingBrace + 1);
                            if (openingBrace == -1 || closingBrace == -1) return "invalid imap fetch response";

                            bool validSize{};
                            size = untagged->value.sliced(openingBrace + 1, closingBrace - openingBrace - 1).toInt(&validSize);
                            if (!validSize || size < 0) return "invalid imap fetch response";
                        }
                    }
                }

                const auto length = header.size() + qMax(0, size);
                if (length > std::numeric_limits<int>::max()) return "imap fetch response too large";
                rxData = m_port->read(static_cast<int>(length), static_cast<int>(fetchDeadline.remainingTime()), "utf-8");
                if (rxData.isEmpty()) return "read timeout";
                if (size >= 0) {
                    rxData.remove(0, header.size());
                    break;
                }

                if (const auto tagged = std::get_if<Tagged>(&result.value)) {
                    if (tagged->tag != fetchTag) return "tag mismatch";
                    if (tagged->code != "OK" && !tagged->text.isEmpty()) return QString::fromUtf8(tagged->text);
                    return "invalid imap fetch response";
                }
                if (fetchDeadline.hasExpired()) return "read timeout";
            }

            if (m_port->readUntil("\r\n", m_timeout, "utf-8").isEmpty()) return "read timeout";

            while (true) {
                const auto result = parser(m_port->readUntil("\r\n", m_timeout, "utf-8"));
                if (!result.exception.isEmpty()) return result.exception;

                const auto tagged = std::get_if<Tagged>(&result.value);
                if (!tagged) continue;
                if (tagged->tag != fetchTag) return "tag mismatch";
                if (tagged->code == "OK") break;
                if (!tagged->text.isEmpty()) return QString::fromUtf8(tagged->text);
                return "unexpected imap response(" + QString::fromUtf8(tagged->code) + ")";
            }

            mail = mailParser(rxData);
            if (_from.isEmpty()) break;
            if (mail.header.value("From").toString().contains(_from)) break;
        }

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());

    QVariantList body{};
    body.reserve(mail.body.size());
    for (const auto &part: mail.body) {
        body.append(QVariantHash{
            {"contentType", part.contentType},
            {"data", part.data}
        });
    }

    QVariantList attachments{};
    attachments.reserve(mail.attachments.size());
    for (const auto &attachment: mail.attachments) {
        attachments.append(QVariantHash{
            {"name", attachment.name},
            {"contentType", attachment.contentType},
            {"data", attachment.data}
        });
    }

    return uni_cast<sol::object>(ts, QVariantHash{
        {"header", mail.header},
        {"body", body},
        {"attachments", attachments}
    });
}

void Imap::logout() {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    QString exception{};

    QMetaObject::invokeMethod(m_port, [this]() -> QString {
        const auto tag = nextTag();
        if (!m_port->write(tag + " LOGOUT", "utf-8", "crlf")) return "write failed";

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

// private
Imap::Result Imap::parser(const QByteArray &rxData) {
    if (rxData.isEmpty()) return {.value = std::monostate{}, .exception = "read timeout"};

    const auto response = rxData.trimmed();
    // Continuation
    if (response.startsWith('+')) return {.value = Continuation{.value = response.sliced(1).trimmed()}, .exception = {}};
    // Untagged
    if (response.startsWith('*')) return {.value = Untagged{.value = response.sliced(1).trimmed()}, .exception = {}};
    // Tagged
    const auto space1 = response.indexOf(' ');
    if (space1 == -1) return {.value = std::monostate{}, .exception = "invalid imap tagged response"};
    const auto space2 = response.indexOf(' ', space1 + 1);
    if (space2 == -1) return {.value = std::monostate{}, .exception = "invalid imap tagged response"};
    return {.value = Tagged{.tag = response.first(space1), .code = response.sliced(space1 + 1, space2 - space1 - 1), .text = response.sliced(space2 + 1)}, .exception = {}};
}

QByteArray Imap::nextTag() {
    return 'A' + QByteArray::number(m_count++).rightJustified(3, '0');
}

Imap::Mail Imap::mailParser(const QByteArray &rxData) {
    Mail mail{};
    QList<QByteArray> boundaries{{"\r\n\r\n"}};
    qsizetype pos = 0;
    bool first = true;

    while (true) {
        qsizetype start{};
        qsizetype end{};
        QByteArray data = rxData;
        QVariantHash hash{};
        QString key{};
        QVariant value{};

        bool isRoot = false;
        if (boundaries.isEmpty()) break;
        const auto boundary = boundaries.last();
        if (boundary == "\r\n\r\n") {
            boundaries.pop_back();
        } else {
            start = rxData.indexOf(boundary, pos);
            if (start == -1) break;
            end = rxData.indexOf("\r\n", start);
            if (end == -1) break;
            if (rxData.sliced(start, end - start) == boundary + "--") {
                boundaries.pop_back();
                continue;
            }
            start = end + 2;
            end = rxData.indexOf(boundary, start);
            if (end == -1) break;
            data = rxData.sliced(start, end - start);
        }

        const auto separator = data.indexOf("\r\n\r\n");
        const auto headers = data.first(separator + 4).replace("\r\n", "\n");
        auto body = data.sliced(separator + 4);

        for (const auto &header: headers.split('\n')) {
            if (header.contains(':')) {
                if (!key.isEmpty()) {
                    if (value.toByteArray().contains("boundary")) {
                        const auto _value = value.toByteArray();
                        const auto quote1 = _value.indexOf('"', _value.indexOf("boundary"));
                        if (quote1 == -1) continue;
                        const auto quote2 = _value.indexOf('"', quote1 + 1);
                        if (quote2 == -1) continue;
                        boundaries.append("--" + _value.sliced(quote1 + 1, quote2 - quote1 - 1));
                        isRoot = true;
                    }
                    hash[key] = value.toString();
                    key.clear();
                    value.clear();
                }
                const auto comma = header.indexOf(':');
                key = header.first(comma);
                value = header.sliced(comma + 2);
            } else {
                value = value.toString() + " " + header.trimmed();
            }
        }

        if (!key.isEmpty()) {
            if (value.toByteArray().contains("boundary")) {
                const auto _value = value.toByteArray();
                const auto quote1 = _value.indexOf('"', _value.indexOf("boundary"));
                if (quote1 == -1) continue;
                const auto quote2 = _value.indexOf('"', quote1 + 1);
                if (quote2 == -1) continue;
                boundaries.append("--" + _value.sliced(quote1 + 1, quote2 - quote1 - 1));
                isRoot = true;
            }
            hash[key] = value.toString();
        }

        if (!isRoot && !body.isEmpty()) {
            if (hash.value("Content-Transfer-Encoding").toByteArray().contains("base64")) {
                body = QByteArray::fromBase64(body);
            }
            hash["Data"] = body;
        }

        if (first) {
            mail.header = hash;
            mail.header.remove("Data");
            first = false;
        }

        if (hash.contains("Content-Disposition") && hash.contains("Data")) {
            const auto disposition = hash.value("Content-Disposition").toByteArray();
            const auto quote1 = disposition.indexOf('"', disposition.indexOf("filename"));
            if (quote1 != -1) {
                const auto quote2 = disposition.indexOf('"', quote1 + 1);
                if (quote2 != -1) {
                    mail.attachments.append({
                        QString::fromUtf8(rfc2047Parser(disposition.sliced(quote1 + 1, quote2 - quote1 - 1))),
                        hash.value("Content-Type").toString(),
                        hash.value("Data").toByteArray()
                    });
                }
            }
        } else if (hash.contains("Data")) {
            mail.body.append({
                hash.value("Content-Type").toString(),
                hash.value("Data").toByteArray()
            });
        }
        pos = start;
    }

    return mail;
}

QByteArray Imap::rfc2047Parser(const QByteArray &text) {
    static const QRegularExpression re(R"(=\?([^?]+)\?([bBqQ])\?([^?]+)\?=)");
    const auto m = re.match(text);
    if (!m.hasMatch()) return text;

    const QByteArray encoding = m.captured(2).toLatin1().toUpper();
    const QByteArray payload = m.captured(3).toLatin1();

    if (encoding == 'B') return QByteArray::fromBase64(payload);
    emit appendLog(LogLevel::Warning, "contact author:", QString("unsupported rfc2047 encoding (Q)"));
    return {};
}
