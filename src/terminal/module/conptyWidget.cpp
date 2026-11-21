#include "terminal/module/conptyWidget.h"

#include <QMetaObject>
#include <QProcessEnvironment>
#include <QUrl>
#include <windows.h>

#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE ProcThreadAttributeValue(22, FALSE, TRUE, FALSE)
#endif

ConptyWidget::ConptyWidget(QObject *parent) : QObject(parent) {
}

ConptyWidget::~ConptyWidget() {
    stop();
}

bool ConptyWidget::start(const QUrl &program, const QString &arguments, const QUrl &workingDirectory, const QString &environment, const int rows, const int cols) {
    if (program.isEmpty() || rows < 1 || cols < 1) return false;

    HANDLE h_inputRead{};
    HANDLE h_inputWrite{};
    HANDLE h_outputRead{};
    HANDLE h_outputWrite{};

    if (!CreatePipe(&h_inputRead, &h_inputWrite, nullptr, 0)) return false;
    if (!CreatePipe(&h_outputRead, &h_outputWrite, nullptr, 0)) {
        CloseHandle(h_inputRead);
        CloseHandle(h_inputWrite);
        return false;
    }

    HPCON pseudoConsole{};
    const HRESULT hr = CreatePseudoConsole(COORD{static_cast<SHORT>(cols), static_cast<SHORT>(rows)}, h_inputRead, h_outputWrite, 0, &pseudoConsole);
    CloseHandle(h_inputRead);
    CloseHandle(h_outputWrite);
    if (FAILED(hr)) {
        CloseHandle(h_inputWrite);
        CloseHandle(h_outputRead);
        return false;
    }

    SIZE_T attributeListSize{};
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeListSize);
    auto *attributeList = static_cast<PPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, attributeListSize));
    if (!attributeList || !InitializeProcThreadAttributeList(attributeList, 1, 0, &attributeListSize)) {
        if (attributeList) HeapFree(GetProcessHeap(), 0, attributeList);
        ClosePseudoConsole(pseudoConsole);
        CloseHandle(h_inputWrite);
        CloseHandle(h_outputRead);
        return false;
    }

    if (!UpdateProcThreadAttribute(
        attributeList,
        0,
        PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
        pseudoConsole,
        sizeof(pseudoConsole),
        nullptr,
        nullptr
    )) {
        DeleteProcThreadAttributeList(attributeList);
        HeapFree(GetProcessHeap(), 0, attributeList);
        ClosePseudoConsole(pseudoConsole);
        CloseHandle(h_inputWrite);
        CloseHandle(h_outputRead);
        return false;
    }

    STARTUPINFOEXW startupInfo{};
    startupInfo.StartupInfo.cb = sizeof(startupInfo);
    startupInfo.lpAttributeList = attributeList;

    const auto &_program = program.toLocalFile();
    const auto &commandLine = QString("\"%1\" %2").arg(_program, arguments);
    const auto &applicationName = _program.toStdWString();
    auto command = commandLine.toStdWString();
    const auto &directory = workingDirectory.toLocalFile().toStdWString();
    auto _environment = environmentParse(environment).toStdWString();

    PROCESS_INFORMATION processInfo{};
    const BOOL created = CreateProcessW(
        applicationName.c_str(),
        command.data(),
        nullptr,
        nullptr,
        FALSE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,
        _environment.data(),
        directory.empty() ? nullptr : directory.c_str(),
        &startupInfo.StartupInfo,
        &processInfo
    );

    DeleteProcThreadAttributeList(attributeList);
    HeapFree(GetProcessHeap(), 0, attributeList);

    if (!created) {
        ClosePseudoConsole(pseudoConsole);
        CloseHandle(h_inputWrite);
        CloseHandle(h_outputRead);
        return false;
    }

    m_pseudoConsole = pseudoConsole;
    m_conptyInputWrite = h_inputWrite;
    m_conptyOutputRead = h_outputRead;
    m_processHandle = processInfo.hProcess;
    m_threadHandle = processInfo.hThread;

    m_readerThread = QThread::create([this] {
        outputRead();
        emit quit();
    });
    m_readerThread->start();
    m_processThread = QThread::create([this] {
        WaitForSingleObject(m_processHandle, INFINITE);
        emit quit();
    });
    m_processThread->start();
    return true;
}

void ConptyWidget::inputWrite(const QByteArray &bytes) const {
    DWORD written{};
    WriteFile(
        m_conptyInputWrite,
        bytes.constData(),
        bytes.size(),
        &written,
        nullptr
    );
}

void ConptyWidget::resize(const int rows, const int cols) const {
    if (m_pseudoConsole) ResizePseudoConsole(m_pseudoConsole, COORD{static_cast<SHORT>(cols), static_cast<SHORT>(rows)});
}

void ConptyWidget::stop() {
    if (m_conptyInputWrite) inputWrite("exit\r\n");

    if (m_processHandle) WaitForSingleObject(m_processHandle, 1000);

    closeHandle(m_conptyInputWrite);

    if (m_pseudoConsole) {
        ClosePseudoConsole(m_pseudoConsole);
        m_pseudoConsole = nullptr;
    }

    if (m_readerThread) {
        m_readerThread->quit();
        m_readerThread->wait(1000);
        delete m_readerThread;
        m_readerThread = nullptr;
    }

    if (m_processThread) {
        m_processThread->quit();
        m_processThread->wait(1000);
        delete m_processThread;
        m_processThread = nullptr;
    }

    closeHandle(m_conptyOutputRead);
    closeHandle(m_threadHandle);
    closeHandle(m_processHandle);
}

bool ConptyWidget::running() const {
    if (!m_processHandle) return false;
    return WaitForSingleObject(m_processHandle, 0) == WAIT_TIMEOUT;
}

// private:
void ConptyWidget::outputRead() {
    char buffer[4096]{};
    DWORD read{};
    while (ReadFile(m_conptyOutputRead, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        const QByteArray bytes(buffer, static_cast<qsizetype>(read));
        emit outputWrite(bytes);
    }
}

QString ConptyWidget::environmentParse(const QString &environment) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.value("TERM").isEmpty()) env.insert("TERM", "xterm-256color");
    if (env.value("COLORTERM").isEmpty()) env.insert("COLORTERM", "truecolor");
    if (env.value("TERM_PROGRAM").isEmpty()) env.insert("TERM_PROGRAM", "UniComm");

    for (const auto &entry: environment.split(';', Qt::SkipEmptyParts)) {
        const qsizetype separator = entry.indexOf('=');
        if (separator <= 0) continue;
        const auto &name = entry.first(separator).trimmed();
        if (name.isEmpty() || name.contains(QChar::Null)) continue;
        const auto &value = entry.sliced(separator + 1);
        if (value.contains(QChar::Null)) continue;
        env.insert(name, value);
    }

    auto keys = env.keys();
    keys.sort(Qt::CaseInsensitive);

    QString block{};
    for (const auto &key: keys) {
        block += key;
        block += '=';
        block += env.value(key);
        block += QChar::Null;
    }
    block += QChar::Null;
    return block;
}

void ConptyWidget::closeHandle(void *&handle) {
    if (handle) {
        CloseHandle(handle);
        handle = nullptr;
    }
}
