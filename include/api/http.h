#ifndef UNICOMM_HTTP_H
#define UNICOMM_HTTP_H

#include <QPointer>
#include <QVariant>
#include <sol/object.hpp>
#include <sol/table.hpp>

#include <string>

class BasePort;

class Http final : public QObject {
    Q_OBJECT

public:
    explicit Http(QObject *parent = nullptr);

    ~Http() override = default;

    void init(const std::string &portName, int timeout);

    [[nodiscard]] sol::object request(sol::this_state ts, const std::string &method, const std::string &target, const sol::optional<sol::table> &header,
                                      const sol::optional<std::string> &body) const;

    [[nodiscard]] sol::object stream(sol::this_state ts, const std::string &method, const std::string &target, const sol::protected_function &fn,
                                     const sol::optional<sol::table> &header, const sol::optional<std::string> &body) const;

private:
    struct StatusCode {
        // https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml
        enum {
            // 1xx Informational
            Continue = 100,
            SwitchingProtocols = 101,
            Processing = 102,
            EarlyHints = 103,

            // 2xx Success
            OK = 200,
            Created = 201,
            Accepted = 202,
            NonAuthoritativeInformation = 203,
            NoContent = 204,
            ResetContent = 205,
            PartialContent = 206,
            MultiStatus = 207,
            AlreadyReported = 208,
            ImUsed = 226,

            // 3xx Redirection
            MultipleChoices = 300,
            MovedPermanently = 301,
            Found = 302,
            SeeOther = 303,
            NotModified = 304,
            UseProxy = 305,
            TemporaryRedirect = 307,
            PermanentRedirect = 308,

            // 4xx Client Error
            BadRequest = 400,
            Unauthorized = 401,
            PaymentRequired = 402,
            Forbidden = 403,
            NotFound = 404,
            MethodNotAllowed = 405,
            NotAcceptable = 406,
            ProxyAuthenticationRequired = 407,
            RequestTimeout = 408,
            Conflict = 409,
            Gone = 410,
            LengthRequired = 411,
            PreconditionFailed = 412,
            ContentTooLarge = 413,
            UriTooLong = 414,
            UnsupportedMediaType = 415,
            RangeNotSatisfiable = 416,
            ExpectationFailed = 417,
            MisdirectedRequest = 421,
            UnprocessableContent = 422,
            Locked = 423,
            FailedDependency = 424,
            TooEarly = 425,
            UpgradeRequired = 426,
            PreconditionRequired = 428,
            TooManyRequests = 429,
            RequestHeaderFieldsTooLarge = 431,
            UnavailableForLegalReasons = 451,

            // 5xx Server Error
            InternalServerError = 500,
            NotImplemented = 501,
            BadGateway = 502,
            ServiceUnavailable = 503,
            GatewayTimeout = 504,
            HttpVersionNotSupported = 505,
            VariantAlsoNegotiates = 506,
            InsufficientStorage = 507,
            LoopDetected = 508,
            NetworkAuthenticationRequired = 511
        };
    };

    struct Result {
        QString version{};
        int statusCode{};
        QString reason{};
        QVariantHash header{};
        QByteArray body{};
        QString exception{};
    };

    [[nodiscard]] sol::object exchange(sol::this_state ts, const std::string &method, const std::string &target, const sol::optional<sol::table> &header,
                                       const sol::optional<std::string> &body, const sol::protected_function *fn) const;

    [[nodiscard]] static Result parser(const QByteArray &rxData);

    std::string m_portName{};
    int m_timeout{};
    QByteArray m_remoteHost{};
    QPointer<BasePort> m_port{};
};

#endif //UNICOMM_HTTP_H
