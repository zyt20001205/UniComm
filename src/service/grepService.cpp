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
    m_process->setArguments({"--version"});
    m_process->start();
    if (m_process->waitForFinished() && m_process->exitStatus() == QProcess::NormalExit && m_process->exitCode() == 0) {
        detailSet(QString::fromUtf8(m_process->readAllStandardOutput()).trimmed().toHtmlEscaped().replace('\n', "<br>"));
    } else {
        statusSet(Status::Error);
    }
    connect(m_process, &QProcess::started, this, [this] { statusSet(Status::Running); });
}

QJsonObject GrepService::grep(const QString &pattern) {
    QStringList args{};
    args << "--json" << pattern << g_workspaceUrl.toLocalFile();
    m_process->setArguments(args);
    statusSet(Status::Stopped);
    m_process->start();
    constexpr int MAX_MATCH = 50;
    bool truncated = false;
    QJsonArray result{};
    const bool success = m_process->waitForFinished() && m_process->exitStatus() == QProcess::NormalExit && (m_process->exitCode() == 0 || m_process->exitCode() == 1);
    if (success) {
        const auto out = m_process->readAllStandardOutput();
        for (const auto &value: out.split('\n')) {
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
    statusSet(success ? Status::Stopped : Status::Error);
    const QJsonObject response{
        {"count", result.size()},
        {"truncated", truncated},
        {"result", result}
    };
    eventAppend("grep", response);
    return response;
}
