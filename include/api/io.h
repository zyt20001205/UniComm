#ifndef UNICOMM_IO_H
#define UNICOMM_IO_H

#include <cstdio>

#include <QJsonObject>
#include <QMutex>
#include <QObject>

class QThread;

struct lua_State;

namespace sol {
    struct variadic_args;
}

class IO final : public QObject {
    Q_OBJECT

public:
    explicit IO(const QString &threadId, bool capture, QObject *parent = nullptr);

    ~IO() override;

    void redirect(lua_State *L);

    QJsonObject finish();

    void print(const sol::variadic_args &args) const;

    void stdIn(const QByteArray &data) const;

    void stdOut(const QByteArray &data) const;

    void stdErr(const QByteArray &data) const;

    static void speak(const std::string &text);

signals:
    void writeTerminal(const QString &id, const QByteArray &data);

private:
    struct OutputBuffer {
        QByteArray output{};
        QByteArray error{};
        bool truncated{};
    };

    enum class Stream {
        Output,
        Error
    };

    void append(Stream stream, const QByteArray &data);

    [[nodiscard]] static std::FILE *fileOpen(void *&handle, int flags, const char *mode);

    static void handleClose(void *&handle);

    static void pathCast(lua_State *L, const char *library, const char *function, int pathCount);

    static constexpr qsizetype MaxStreamSize = 64 * 1024;

    QString m_threadId{};
    bool m_capture{};
    void *m_inputWrite{};
    void *m_stdoutRead{};
    void *m_stderrRead{};
    void *m_stdin{};
    void *m_stdout{};
    void *m_stderr{};
    QThread *m_stdoutThread{};
    QThread *m_stderrThread{};
    QMutex m_outputMutex{};
    OutputBuffer m_output{};
};

#endif //UNICOMM_IO_H
