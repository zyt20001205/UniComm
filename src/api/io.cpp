#include "api/io.h"

#include <fcntl.h>
#include <io.h>
#include <QTextToSpeech>
#include <QThread>
#include <QVariant>
#include <sol/object.hpp>
#include <windows.h>

#include "globals.h"
#include "util/uniCast.h"

IO::IO(const QString &threadId, const bool capture, QObject *parent)
    : QObject(parent),
      m_threadId(threadId),
      m_capture(capture) {
}

IO::~IO() {
    finish();
    handleClose(m_inputWrite);
    if (m_stdin) {
        fclose(static_cast<FILE *>(m_stdin));
        m_stdin = nullptr;
    }
}

void IO::redirect(lua_State *L) {
    HANDLE inputRead{};
    HANDLE inputWrite{};
    HANDLE stdoutRead{};
    HANDLE stdoutWrite{};
    HANDLE stderrRead{};
    HANDLE stderrWrite{};
    if (!CreatePipe(&inputRead, &inputWrite, nullptr, 0)) throw sol::error("create stdin pipe failed");
    if (!CreatePipe(&stdoutRead, &stdoutWrite, nullptr, 0)) {
        CloseHandle(inputRead);
        CloseHandle(inputWrite);
        throw sol::error("create stdout pipe failed");
    }
    if (!CreatePipe(&stderrRead, &stderrWrite, nullptr, 0)) {
        CloseHandle(inputRead);
        CloseHandle(inputWrite);
        CloseHandle(stdoutRead);
        CloseHandle(stdoutWrite);
        throw sol::error("create stderr pipe failed");
    }

    void *stdinHandle = inputRead;
    void *stdoutHandle = stdoutWrite;
    void *stderrHandle = stderrWrite;
    auto *stdinFile = fileOpen(stdinHandle, _O_RDONLY | _O_TEXT, "r");
    auto *stdoutFile = fileOpen(stdoutHandle, _O_WRONLY | _O_TEXT, "w");
    auto *stderrFile = fileOpen(stderrHandle, _O_WRONLY | _O_TEXT, "w");
    handleClose(stdinHandle);
    handleClose(stdoutHandle);
    handleClose(stderrHandle);
    if (!stdinFile || !stdoutFile || !stderrFile) {
        if (stdinFile) fclose(stdinFile);
        if (stdoutFile) fclose(stdoutFile);
        if (stderrFile) fclose(stderrFile);
        CloseHandle(inputWrite);
        CloseHandle(stdoutRead);
        CloseHandle(stderrRead);
        throw sol::error("open standard stream failed");
    }

    lua_getglobal(L, "io");
    lua_getfield(L, -1, "stdin");
    auto *stdinStream = static_cast<luaL_Stream *>(luaL_testudata(L, -1, LUA_FILEHANDLE));
    lua_pop(L, 1);
    lua_getfield(L, -1, "stdout");
    auto *stdoutStream = static_cast<luaL_Stream *>(luaL_testudata(L, -1, LUA_FILEHANDLE));
    lua_pop(L, 1);
    lua_getfield(L, -1, "stderr");
    auto *stderrStream = static_cast<luaL_Stream *>(luaL_testudata(L, -1, LUA_FILEHANDLE));
    lua_pop(L, 1);
    lua_pop(L, 1);
    if (!stdinStream || !stdoutStream || !stderrStream) {
        fclose(stdinFile);
        fclose(stdoutFile);
        fclose(stderrFile);
        CloseHandle(inputWrite);
        CloseHandle(stdoutRead);
        CloseHandle(stderrRead);
        throw sol::error("standard stream is unavailable");
    }

    stdinStream->f = stdinFile;
    stdoutStream->f = stdoutFile;
    stderrStream->f = stderrFile;
    m_inputWrite = inputWrite;
    m_stdoutRead = stdoutRead;
    m_stderrRead = stderrRead;
    m_stdin = stdinFile;
    m_stdout = stdoutFile;
    m_stderr = stderrFile;
    m_stdoutThread = QThread::create([this] {
        char buffer[4096]{};
        DWORD read{};
        while (ReadFile(m_stdoutRead, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
            const QByteArray data(buffer, static_cast<qsizetype>(read));
            append(Stream::Output, data);
            emit writeTerminal(m_threadId, data);
        }
    });
    m_stderrThread = QThread::create([this] {
        char buffer[4096]{};
        DWORD read{};
        while (ReadFile(m_stderrRead, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
            const QByteArray data(buffer, static_cast<qsizetype>(read));
            append(Stream::Error, data);
            emit writeTerminal(m_threadId, data);
        }
    });
    m_stdoutThread->start();
    m_stderrThread->start();

    pathCast(L, "io", "open", 1);
    pathCast(L, "io", "input", 1);
    pathCast(L, "io", "output", 1);
    pathCast(L, "io", "lines", 1);
    pathCast(L, "os", "remove", 1);
    pathCast(L, "os", "rename", 2);
}

QJsonObject IO::finish() {
    if (m_stdout) {
        fclose(static_cast<FILE *>(m_stdout));
        m_stdout = nullptr;
    }
    if (m_stderr) {
        fclose(static_cast<FILE *>(m_stderr));
        m_stderr = nullptr;
    }
    if (m_stdoutThread) {
        m_stdoutThread->wait();
        delete m_stdoutThread;
        m_stdoutThread = nullptr;
    }
    if (m_stderrThread) {
        m_stderrThread->wait();
        delete m_stderrThread;
        m_stderrThread = nullptr;
    }
    handleClose(m_stdoutRead);
    handleClose(m_stderrRead);
    const QMutexLocker locker(&m_outputMutex);
    return {
        {"output", QString::fromUtf8(m_output.output)},
        {"err", QString::fromUtf8(m_output.error)},
        {"truncated", m_output.truncated}
    };
}

void IO::stdIn(const QByteArray &data) const {
    if (!m_inputWrite || data.isEmpty()) return;
    DWORD written{};
    WriteFile(m_inputWrite, data.constData(), data.size(), &written, nullptr);
}

void IO::stdOut(const QByteArray &data) const {
    auto *file = static_cast<FILE *>(m_stdout);
    if (!file || fwrite(data.constData(), 1, static_cast<size_t>(data.size()), file) != static_cast<size_t>(data.size()))
        throw sol::error("stdout write failed");
}

void IO::stdErr(const QByteArray &data) const {
    auto *file = static_cast<FILE *>(m_stderr);
    if (!file || fwrite(data.constData(), 1, static_cast<size_t>(data.size()), file) != static_cast<size_t>(data.size()))
        throw sol::error("stderr write failed");
}

void IO::print(const sol::variadic_args &args) const {
    QByteArray data{};
    std::function<void(const QVariant &, int)> printing = [&](const QVariant &value, const int depth) {
        if (value.typeId() == QMetaType::QVariantMap) {
            const auto map = value.toMap();
            data.append('{');
            for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
                data.append('\n');
                data.append(QByteArray(" ").repeated((depth + 1) * 4));
                data.append(it.key().toUtf8());
                data.append(": ");
                printing(it.value(), depth + 1);
            }
            if (!map.isEmpty()) {
                data.append('\n');
                data.append(QByteArray(" ").repeated(depth * 4));
            }
            data.append('}');
        } else data.append(value.toString().toUtf8());
    };

    bool first = true;
    for (const auto &arg: uni_cast<QVariantList>(args)) {
        if (!first) data.append('\t');
        printing(arg, 0);
        first = false;
    }
    data.append('\n');
    stdOut(data);
}

void IO::speak(const std::string &text) {
    QTextToSpeech tts;
    if (tts.engine().isEmpty()) {
        throw sol::error(tr("No TTS engine found").toStdString());
    }
    if (text.empty()) {
        return;
    }
    tts.setLocale(QLocale::English);
    // tts.setLocale(QLocale::Chinese);
    tts.setRate(0.0);
    tts.setVolume(1.0);
    QEventLoop loop;
    connect(&tts, &QTextToSpeech::stateChanged, [&](const QTextToSpeech::State state) { if (state == QTextToSpeech::Ready) loop.quit(); });
    tts.say(QString::fromStdString(text));
    loop.exec();
}

// private
void IO::append(const Stream stream, const QByteArray &data) {
    if (!m_capture) return;

    const QMutexLocker locker(&m_outputMutex);
    auto &buffer = stream == Stream::Output ? m_output.output : m_output.error;
    const auto available = MaxStreamSize - buffer.size();
    if (available <= 0) {
        m_output.truncated = true;
        return;
    }
    const auto length = qMin(available, data.size());
    buffer.append(data.first(length));
    if (length < data.size()) m_output.truncated = true;
}

std::FILE *IO::fileOpen(void *&handle, const int flags, const char *mode) {
    const auto descriptor = _open_osfhandle(reinterpret_cast<std::intptr_t>(handle), flags);
    if (descriptor == -1) return nullptr;
    handle = nullptr;
    auto *file = _fdopen(descriptor, mode);
    if (!file) {
        _close(descriptor);
        return nullptr;
    }
    setvbuf(file, nullptr, _IONBF, 0);
    return file;
}

void IO::handleClose(void *&handle) {
    if (!handle) return;
    CloseHandle(handle);
    handle = nullptr;
}

void IO::pathCast(lua_State *L, const char *library, const char *function, const int pathCount) {
    lua_getglobal(L, library);
    lua_getfield(L, -1, function);
    lua_pushinteger(L, pathCount);
    lua_pushcclosure(L, [](lua_State *L) -> int {
        const int arguments = lua_gettop(L);
        const int pathCount = static_cast<int>(lua_tointeger(L, lua_upvalueindex(2)));
        for (int i = 1; i <= pathCount && i <= arguments; ++i) {
            if (lua_type(L, i) != LUA_TSTRING) continue;
            size_t size{};
            const char *path = lua_tolstring(L, i, &size);
            const auto documentUrl = uni_cast<QUrl>(LPath(QString::fromUtf8(path, static_cast<qsizetype>(size))));
            const auto resolved = documentUrl.toLocalFile().toUtf8();
            lua_pushlstring(L, resolved.constData(), static_cast<size_t>(resolved.size()));
            lua_replace(L, i);
        }

        lua_pushvalue(L, lua_upvalueindex(1));
        lua_insert(L, 1);
        lua_call(L, arguments, LUA_MULTRET);
        return lua_gettop(L);
    }, 2);
    lua_setfield(L, -2, function);
    lua_pop(L, 1);
}
