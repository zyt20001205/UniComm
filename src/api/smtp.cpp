#include "api/smtp.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <sol/error.hpp>
#include <sol/table_core.hpp>

#include "globals.h"
#include "port/basePort.h"
#include "port/portModule.h"
#include "util/uniCast.h"

// public
Smtp::Smtp(QObject *parent)
    : QObject(parent) {
}

void Smtp::init(const std::string &portName, const int timeout) {
    const auto _portName = QString::fromStdString(portName);
    const auto port = g_port->m_portHash.constFind(_portName);
    if (port == g_port->m_portHash.constEnd()) throw sol::error(portName + " does not exist");
    m_portName = portName;
    m_timeout = timeout;
    m_port = port.value();

    QString exception{};

    QMetaObject::invokeMethod(m_port, [this]() -> QString {
        if (!m_port->open()) return "open failed";

        const auto result = response();
        if (!result.exception.isEmpty()) return result.exception;
        if (result.code != StatusCode::ServiceReady) return "unexpected smtp response(" + QString::number(result.code) + ")";

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

void Smtp::authLogin(const std::string &username, const std::string &password) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    QString exception{};
    const auto _username = QByteArray::fromStdString(username).toBase64();
    const auto _password = QByteArray::fromStdString(password).toBase64();

    QMetaObject::invokeMethod(m_port, [this, &_username, &_password]() -> QString {
        if (!m_port->write("AUTH LOGIN", "utf-8", "crlf")) return "write failed";

        auto result = response();
        if (!result.exception.isEmpty()) return result.exception;
        if (result.code != StatusCode::AuthenticationChallenge) return "unexpected smtp response(" + QString::number(result.code) + ")";

        if (!m_port->write(_username, "utf-8", "crlf")) return "write failed";

        result = response();
        if (!result.exception.isEmpty()) return result.exception;
        if (result.code != StatusCode::AuthenticationChallenge) return "unexpected smtp response(" + QString::number(result.code) + ")";

        if (!m_port->write(_password, "utf-8", "crlf")) return "write failed";

        result = response();
        if (!result.exception.isEmpty()) return result.exception;
        if (result.code != StatusCode::AuthenticationSucceeded) return "unexpected smtp response(" + QString::number(result.code) + ")";

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

void Smtp::ehlo() const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    QString exception{};

    QMetaObject::invokeMethod(m_port, [this]() -> QString {
        if (!m_port->write("EHLO localhost", "utf-8", "crlf")) return "write failed";

        const auto result = response();
        if (!result.exception.isEmpty()) return result.exception;
        if (result.code != StatusCode::RequestedMailActionOkay) return "unexpected smtp response(" + QString::number(result.code) + ")";

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

void Smtp::send(const sol::table &mail) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    QString exception{};

    const auto _from = QByteArray::fromStdString(mail.get<std::string>("from"));
    const auto _to = mail.get<sol::object>("to");
    const auto _cc = mail.get<sol::object>("cc");
    const auto _bcc = mail.get<sol::object>("bcc");

    // from
    const auto from = "MAIL FROM: <" + _from + ">";

    // to
    const auto toHelper = [](const sol::object &value) {
        QList<QByteArray> result{};
        const auto append = [&result](const std::string &address) {
            result.append("RCPT TO: <" + QByteArray::fromStdString(address) + ">");
        };

        if (value.is<std::string>()) {
            append(value.as<std::string>());
        } else if (value.is<sol::table>()) {
            for (const auto &[key, recipient]: value.as<sol::table>()) {
                if (recipient.is<std::string>()) append(recipient.as<std::string>());
            }
        }
        return result;
    };

    QList<QByteArray> to{};
    to.append(toHelper(_to));
    to.append(toHelper(_cc));
    to.append(toHelper(_bcc));

    // content
    const auto contentHelper = [](const sol::object &value) {
        QByteArray result{};
        const auto append = [&result](const std::string &address) {
            if (!result.isEmpty()) result += ", ";
            result += "<" + QByteArray::fromStdString(address) + ">";
        };

        if (value.is<std::string>()) {
            append(value.as<std::string>());
        } else if (value.is<sol::table>()) {
            for (const auto &[key, recipient]: value.as<sol::table>()) {
                if (recipient.is<std::string>()) append(recipient.as<std::string>());
            }
        }
        return result;
    };

    const auto subject = QByteArray::fromStdString(mail.get<std::string>("subject"));
    const auto body = QByteArray::fromStdString(mail.get<std::string>("body"));
    const auto boundary = QByteArray::number(QDateTime::currentMSecsSinceEpoch());

    QByteArray data = "From: <" + _from + ">\r\n"
                      + "To: " + contentHelper(_to) + "\r\n";
    const auto cc = contentHelper(_cc);
    if (!cc.isEmpty()) data += "Cc: " + cc + "\r\n";

    data += "Subject: " + subject + "\r\n"
            + "Date: " + QDateTime::currentDateTime().toString(Qt::RFC2822Date).toUtf8() + "\r\n"
            + "MIME-Version: 1.0\r\n"
            + "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n"
            + "\r\n"
            + "--" + boundary + "\r\n"
            + "Content-Type: text/plain; charset=utf-8\r\n"
            + "Content-Transfer-Encoding: 8bit\r\n"
            + "\r\n"
            + body + "\r\n";

    // attachment
    const auto attachmentHelper = [this, &boundary](const sol::object &value) {
        QByteArray result{};
        const auto append = [this, &boundary, &result](const std::string &attachment) {
            const LPath luaPath = QString::fromStdString(attachment);
            const auto documentUrl = uni_cast<QUrl>(luaPath);
            const auto documentPath = documentUrl.toLocalFile();

            QFile attachmentFile(documentPath);
            if (!attachmentFile.open(QIODevice::ReadOnly))
                throw sol::error(m_portName + ": attachment invalid");

            const QMimeDatabase mimeDb{};
            const auto mimeType = mimeDb.mimeTypeForFile(documentPath).name();
            const auto fileName = QFileInfo(documentPath).fileName();

            result += "--" + boundary + "\r\n"
                    + "Content-Type: " + mimeType.toUtf8() + "\r\n"
                    + "Content-Transfer-Encoding: base64\r\n"
                    + "Content-Disposition: attachment; filename=\"" + fileName.toUtf8() + "\"\r\n"
                    + "\r\n";

            const auto base64 = attachmentFile.readAll().toBase64();
            for (qsizetype offset = 0; offset < base64.size(); offset += 76) {
                const auto length = qMin<qsizetype>(76, base64.size() - offset);
                result += base64.sliced(offset, length) + "\r\n";
            }
            if (base64.isEmpty()) result += "\r\n";
        };

        if (value.is<std::string>()) {
            append(value.as<std::string>());
        } else if (value.is<sol::table>()) {
            for (const auto &[key, attachment]: value.as<sol::table>()) {
                if (attachment.is<std::string>()) append(attachment.as<std::string>());
            }
        }
        return result;
    };

    data += attachmentHelper(mail.get<sol::object>("attachment"));
    data += "--" + boundary + "--\r\n.";

    QMetaObject::invokeMethod(m_port, [this, &from, &to, &data]() -> QString {
        // from
        if (!m_port->write(from, "utf-8", "crlf")) return "write failed";

        auto result = response();
        if (!result.exception.isEmpty()) return result.exception;
        if (result.code != StatusCode::RequestedMailActionOkay) return "unexpected smtp response(" + QString::number(result.code) + ")";

        // to
        for (const auto &value: to) {
            if (!m_port->write(value, "utf-8", "crlf")) return "write failed";

            result = response();
            if (!result.exception.isEmpty()) return result.exception;
            if (result.code != StatusCode::RequestedMailActionOkay && result.code != StatusCode::UserNotLocalWillForward)
                return "unexpected smtp response(" + QString::number(result.code) + ")";
        }

        // data
        if (!m_port->write("DATA", "utf-8", "crlf")) return "write failed";

        result = response();
        if (!result.exception.isEmpty()) return result.exception;
        if (result.code != StatusCode::StartMailInput) return "unexpected smtp response(" + QString::number(result.code) + ")";

        if (!m_port->write(data, "utf-8", "crlf")) return "write failed";

        result = response();
        if (!result.exception.isEmpty()) return result.exception;
        if (result.code != StatusCode::RequestedMailActionOkay) return "unexpected smtp response(" + QString::number(result.code) + ")";

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

void Smtp::quit() const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    QString exception{};

    QMetaObject::invokeMethod(m_port, [this]() -> QString {
        if (!m_port->write("QUIT", "utf-8", "crlf")) return "write failed";

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
}

// private
Smtp::Result Smtp::response() const {
    while (true) {
        const auto result = parser(m_port->readUntil("\r\n", m_timeout, "utf-8", false));
        if (!result.exception.isEmpty() || result.code != 0) return result;
    }
}

Smtp::Result Smtp::parser(const QByteArray &rxData) {
    if (rxData.isEmpty()) return {0, "read timeout"};
    if (rxData.size() < 4) return {0, "invalid smtp response"};
    const auto code = rxData.first(3).toInt();
    if (rxData.at(3) == '-') return {};
    if (code >= 400 && code < 600) return {code, QString::fromUtf8(rxData.sliced(4).trimmed())};
    return {code, {}};
}
