#include "api/http.h"

#include <QScopeGuard>
#include <QThread>
#include <sol/error.hpp>
#include <sol/protected_function.hpp>

#include "globals.h"
#include "port/basePort.h"
#include "port/portModule.h"
#include "util/uniCast.h"

// public
Http::Http(QObject *parent)
    : QObject(parent) {
}

void Http::init(const std::string &portName, const int timeout) {
    const QString name = QString::fromStdString(portName);
    const auto port = g_port->m_portHash.constFind(name);
    if (port == g_port->m_portHash.constEnd()) throw sol::error(portName + " does not exist");

    m_portName = portName;
    m_timeout = timeout;
    m_port = port.value();

    QByteArray remoteHost{};
    QString exception{};

    QMetaObject::invokeMethod(m_port, [&remoteHost, this]() -> QString {
        if (!m_port->open()) return "open failed";

        remoteHost = m_port->config().value("remoteHost").toString().toUtf8();

        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
    m_remoteHost = remoteHost;
}

sol::object Http::request(const sol::this_state ts, const std::string &method, const std::string &target, const sol::optional<sol::table> &header,
                          const sol::optional<std::string> &body) const {
    return exchange(ts, method, target, header, body, nullptr);
}

sol::object Http::stream(const sol::this_state ts, const std::string &method, const std::string &target, const sol::protected_function &fn,
                         const sol::optional<sol::table> &header, const sol::optional<std::string> &body) const {
    return exchange(ts, method, target, header, body, &fn);
}

// private
sol::object Http::exchange(const sol::this_state ts, const std::string &method, const std::string &target, const sol::optional<sol::table> &header,
                           const sol::optional<std::string> &body, const sol::protected_function *fn) const {
    if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");
    // method check
    if (method.empty() || method.find_first_not_of("!#$%&'*+-.^_`|~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") != std::string::npos) throw sol::error("invalid HTTP method");
    if (method == "CONNECT") throw sol::error("HTTP CONNECT tunneling is not supported");
    Result result{};
    QByteArray txData = QByteArray::fromStdString(method) + " " + QByteArray::fromStdString(target) + " HTTP/1.1\r\n";
    txData += "Host: " + m_remoteHost + "\r\n";
    const auto _body = body.has_value() ? QByteArray::fromStdString(body.value()) : QByteArray{};
    if (body.has_value()) txData += "Content-Length: " + QByteArray::number(_body.size()) + "\r\n";
    if (header.has_value()) {
        for (const auto &[key, value]: header.value()) {
            if (!key.is<std::string>() || !value.is<std::string>()) continue;
            const auto name = QByteArray::fromStdString(key.as<std::string>());
            const auto fieldValue = QByteArray::fromStdString(value.as<std::string>());
            if (name.compare("Host", Qt::CaseInsensitive) == 0 ||
                name.compare("Content-Length", Qt::CaseInsensitive) == 0 ||
                name.compare("Transfer-Encoding", Qt::CaseInsensitive) == 0)
                continue;
            txData += name + ": " + fieldValue + "\r\n";
        }
    }
    txData += "\r\n";
    if (body.has_value()) txData += _body;

    // close on error or early stop
    bool complete = false;
    const auto cleanup = qScopeGuard([&complete, this] {
        if (!complete && m_port) QMetaObject::invokeMethod(m_port, [this] { m_port->close(); }, Qt::BlockingQueuedConnection);
    });

    // send request
    QString exception{};
    QMetaObject::invokeMethod(m_port, [&result, this, &txData]() -> QString {
        if (!m_port->write(txData, "utf-8", "null")) return "write failed";

        // skip interim responses
        while (true) {
            const auto rxHeader = m_port->readUntil("\r\n\r\n", m_timeout, "utf-8");
            if (rxHeader.isEmpty()) return "read timeout";
            result = parser(rxHeader);
            if (!result.exception.isEmpty()) return result.exception;
            if (result.statusCode == StatusCode::SwitchingProtocols) return "HTTP protocol upgrade is not supported";
            if (result.statusCode >= 200) return {};
        }
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());

    auto response = uni_cast<sol::object>(ts, QVariantHash{
        {"version", result.version},
        {"statusCode", result.statusCode},
        {"reason", result.reason},
        {"header", result.header},
        {"body", QByteArray{}}
    }).as<sol::table>();
    if (fn) response["stopped"] = false;

    // no response body
    if (method == "HEAD" || result.statusCode == StatusCode::NoContent || result.statusCode == StatusCode::NotModified) {
        complete = true;
        return response;
    }

    const bool chunked = result.header.value("transfer-encoding").toString().contains("chunked", Qt::CaseInsensitive);
    // remaining bytes; -1 reads until close
    qint64 remaining = chunked ? 0 : -1;
    if (!chunked && !result.header.contains("transfer-encoding") && result.header.contains("content-length")) {
        bool ok{};
        remaining = result.header.value("content-length").toString().toLongLong(&ok);
        if (!ok || remaining < 0) throw sol::error(m_portName + ": invalid HTTP content length");
        if (remaining == 0) {
            complete = true;
            return response;
        }
    }

    bool done = false;
    while (!done) {
        if (QThread::currentThread()->isInterruptionRequested()) throw sol::error("terminated");
        if (m_port.isNull()) throw sol::error(m_portName + ": port is no longer available");

        QByteArray data{};
        QMetaObject::invokeMethod(m_port, [&data, &remaining, &done, this, chunked]() -> QString {
            // read chunk size
            if (chunked && remaining == 0) {
                auto sizeLine = m_port->readUntil("\r\n", m_timeout, "utf-8");
                if (sizeLine.isEmpty()) return "read timeout";
                sizeLine.chop(2);
                const auto semicolon = sizeLine.indexOf(';');
                if (semicolon != -1) sizeLine.truncate(semicolon);
                bool ok{};
                remaining = sizeLine.trimmed().toLongLong(&ok, 16);
                if (!ok || remaining < 0) return "invalid HTTP chunk size";

                // zero chunk: discard trailers
                if (remaining == 0) {
                    while (true) {
                        const auto trailer = m_port->readUntil("\r\n", m_timeout, "utf-8");
                        if (trailer.isEmpty()) return "read timeout";
                        if (trailer == "\r\n") break;
                    }
                    done = true;
                    return {};
                }
            }

            // wait without consuming
            const bool ready = !m_port->read(1, m_timeout, "utf-8", true).isEmpty();
            const auto info = m_port->info();
            if (!ready) {
                if (remaining == -1 && info.value("status").toString() == "unconnected") {
                    done = true;
                    return {};
                }
                if (info.value("status").toString() == "unconnected") return "incomplete HTTP body";
                return "read timeout";
            }

            // read available bytes, up to 64 KiB
            int length = qMin(info.value("bufferUsed").toInt(), 64 * 1024);
            if (remaining >= 0) length = static_cast<int>(qMin<qint64>(length, remaining));
            data = m_port->read(length, 0, "utf-8");
            if (remaining >= 0) remaining -= data.size();
            if (remaining == 0) {
                if (chunked) {
                    if (m_port->read(2, m_timeout, "utf-8") != "\r\n") return "invalid HTTP chunk delimiter";
                } else done = true;
            }
            return {};
        }, Qt::BlockingQueuedConnection, &exception);
        if (!exception.isEmpty()) throw sol::error(m_portName + ": " + exception.toStdString());
        if (data.isEmpty()) continue;

        // lua thread: callback or collect body
        if (fn) {
            auto returned = (*fn)(data.toStdString(), response);
            if (!returned.valid()) throw returned.get<sol::error>();
            if (returned.return_count() > 0) {
                const auto value = returned.get<sol::object>();
                if (value.is<bool>() && !value.as<bool>()) {
                    response["stopped"] = true;
                    return response;
                }
            }
        } else {
            result.body += data;
        }
    }

    complete = true;
    if (!fn) response["body"] = result.body.toStdString();
    return response;
}

Http::Result Http::parser(const QByteArray &rxData) {
    // status line
    const auto eol = rxData.indexOf("\r\n");
    if (eol == -1) return {.exception = "invalid HTTP status line"};
    const auto line = rxData.first(eol);
    const auto space1 = line.indexOf(' ');
    if (space1 == -1) return {.exception = "invalid HTTP status line"};
    const auto space2 = line.indexOf(' ', space1 + 1);
    bool validStatusCode{};
    const auto version = QString::fromUtf8(line.first(space1));
    const auto status = space2 == -1 ? line.sliced(space1 + 1) : line.sliced(space1 + 1, space2 - space1 - 1);
    const auto statusCode = status.toInt(&validStatusCode);
    const auto reason = space2 == -1 ? QString{} : QString::fromUtf8(line.sliced(space2 + 1));
    if (!validStatusCode || statusCode < StatusCode::Continue || statusCode > StatusCode::NetworkAuthenticationRequired) return {.exception = "invalid HTTP status line"};

    // header
    QVariantHash header{};
    qsizetype pos = eol + 2;
    while (pos < rxData.size() - 2) {
        const auto _eol = rxData.indexOf("\r\n", pos);
        if (_eol == pos) break;

        const auto _line = rxData.mid(pos, _eol - pos);
        const auto colon = _line.indexOf(':');
        if (colon > 0) {
            const auto name = _line.first(colon);
            const auto key = QString::fromUtf8(name).toLower();
            const auto value = QString::fromUtf8(_line.mid(colon + 1).trimmed());
            header[key] = value;
        }

        pos = _eol + 2;
    }

    return {version, statusCode, reason, header, {}, {}};
}
