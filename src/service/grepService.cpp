#include "service/grepService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>

#include <globals.h>

// public
GrepService::GrepService(QObject *parent)
    : BaseService("Grep", parent),
      m_process(new QProcess(this)) {
    m_process->setProgram(QCoreApplication::applicationDirPath() + "/ripgrep/rg.exe");
    connect(m_process, &QProcess::started, this, [this] { statusSet(Status::Running); });
}

QJsonObject GrepService::grep(const QString &pattern) {
    QStringList args{};
    args << "--json" << pattern << g_workspaceUrl.toLocalFile();
    m_process->setArguments(args);
    detailSet({});
    statusSet(Status::Stopped);
    m_process->start();
    constexpr int MAX_MATCH = 50;
    bool truncated = false;
    QJsonArray result{};
    const bool finished = m_process->waitForFinished();
    const auto processError = m_process->errorString();
    if (!finished && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished();
    }
    const bool success = finished && m_process->exitStatus() == QProcess::NormalExit
        && (m_process->exitCode() == 0 || m_process->exitCode() == 1);
    if (success) {
        const auto out = m_process->readAllStandardOutput();
        for (const auto &value : out.split('\n')) {
            if (value.isEmpty()) continue;
            const auto json = QJsonDocument::fromJson(value).object();
            if (json.value("type").toString() != "match") continue;
            if (result.size() == MAX_MATCH) {
                truncated = true;
                break;
            }
            auto data = json.value("data").toObject();
            const auto path = data.value("path").toObject()["text"].toString();
            data.remove("path");
            data["url"] = QUrl::fromLocalFile(path).toString();
            result.append(data);
        }
    }
    if (!success) {
        const auto error = QString::fromUtf8(m_process->readAllStandardError()).trimmed();
        detailSet((error.isEmpty() ? processError : error).toHtmlEscaped());
    }
    statusSet(success ? Status::Stopped : Status::Error);
    const QJsonObject response{
        {"count", result.size()},
        {"truncated", truncated},
        {"result", result}
    };
    eventAppend("grep", response);
    return response;
}
