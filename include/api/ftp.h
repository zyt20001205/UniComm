#ifndef UNICOMM_FTP_H
#define UNICOMM_FTP_H

#include <QObject>
#include <QPointer>
#include <QString>

#include <sol/object.hpp>
#include <sol/table.hpp>

#include <string>

class BasePort;

class Ftp final : public QObject {
    Q_OBJECT

public:
    explicit Ftp(QObject *parent = nullptr);

    ~Ftp() override = default;

    void init(const std::string &portName, int timeout);

    void login(const std::string &username, const std::string &password) const;

    [[nodiscard]] std::string pwd() const;

    void cd(const std::string &path) const;

    [[nodiscard]] sol::table list(sol::this_state ts, const sol::optional<std::string> &path) const;

    [[nodiscard]] sol::object stat(sol::this_state ts, const std::string &path) const;

    [[nodiscard]] bool exists(const std::string &path) const;

    void mkdir(const std::string &path) const;

    void rmdir(const std::string &path) const;

    void remove(const std::string &path) const;

    void rename(const std::string &from, const std::string &to) const;

    [[nodiscard]] std::string download(const std::string &path) const;

    void upload(const std::string &path, const std::string &data) const;

    void quit() const;

private:
    struct StatusCode {
        // https://www.iana.org/assignments/ftp-commands-extensions/ftp-commands-extensions.xhtml#ftp-commands-extensions-2
        enum {
            // 1xx Positive Preliminary
            RestartMarkerReply = 110,
            ServiceReadyInMinutes = 120,
            DataConnectionAlreadyOpen = 125,
            FileStatusOkay = 150,

            // 2xx Positive Completion
            CommandOkay = 200,
            CommandNotImplementedSuperfluous = 202,
            SystemStatus = 211,
            DirectoryStatus = 212,
            FileStatus = 213,
            HelpMessage = 214,
            SystemType = 215,
            ServiceReady = 220,
            ServiceClosingControlConnection = 221,
            DataConnectionOpen = 225,
            ClosingDataConnection = 226,
            EnteringPassiveMode = 227,
            EnteringExtendedPassiveMode = 229,
            UserLoggedIn = 230,
            RequestedFileActionOkay = 250,
            PathnameCreated = 257,

            // 3xx Positive Intermediate
            UserNameOkay = 331,
            NeedAccountForLogin = 332,
            RequestedFileActionPending = 350,

            // 4xx Transient Negative Completion
            ServiceNotAvailable = 421,
            CannotOpenDataConnection = 425,
            ConnectionClosedTransferAborted = 426,
            RequestedFileActionNotTaken = 450,
            RequestedActionAborted = 451,
            InsufficientStorage = 452,

            // 5xx Permanent Negative Completion
            SyntaxError = 500,
            SyntaxErrorInParameters = 501,
            CommandNotImplemented = 502,
            BadSequenceOfCommands = 503,
            CommandNotImplementedForParameter = 504,
            NetworkProtocolNotSupported = 522,
            NotLoggedIn = 530,
            NeedAccountForStoringFiles = 532,
            FileUnavailable = 550,
            PageTypeUnknown = 551,
            ExceededStorageAllocation = 552,
            FileNameNotAllowed = 553
        };
    };

    struct CtrlResult {
        int code{};
        QString text{};
        QString exception{};
    };

    struct DataResult {
        int code{}; // Transfer-command rejection code; not passive connection setup errors.
        QByteArray data{};
        QString exception{};
    };

    [[nodiscard]] CtrlResult ctrlResponse() const;

    [[nodiscard]] static CtrlResult ctrlParser(const QByteArray &rxData);

    [[nodiscard]] DataResult dataResponse(const QByteArray &command) const;

    [[nodiscard]] QString dataRequest(const QByteArray &command, const QByteArray &data) const;

    [[nodiscard]] DataResult dataTransfer(const QByteArray &command, const QByteArray *data) const;

    std::string m_portName{};
    int m_timeout{};
    QPointer<BasePort> m_port{};
};

#endif //UNICOMM_FTP_H
