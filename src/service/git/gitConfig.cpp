#include "service/git/gitConfig.h"

#include <QProcess>

#include "globals.h"

GitConfig::GitConfig(QObject *parent)
    : QObject(parent),
      m_process(new QProcess(this)) {
    m_process->setWorkingDirectory(g_workspaceUrl.toLocalFile());
    connect(m_process, &QProcess::finished, this, [this](const int exitcode) { processFinished(exitcode); });
}

void GitConfig::localHttpProxyGet() {
    processEnqueue(GitCommand::LocalHttpProxyGet, QStringList{"config", "--local", "http.proxy"});
}

void GitConfig::localHttpsProxyGet() {
    processEnqueue(GitCommand::LocalHttpsProxyGet, QStringList{"config", "--local", "https.proxy"});
}

void GitConfig::globalHttpProxyGet() {
    processEnqueue(GitCommand::GlobalHttpProxyGet, QStringList{"config", "--global", "http.proxy"});
}

void GitConfig::globalHttpsProxyGet() {
    processEnqueue(GitCommand::GlobalHttpsProxyGet, QStringList{"config", "--global", "https.proxy"});
}

void GitConfig::gitProxySet(const QString &localHttpProxy, const QString &localHttpsProxy, const QString &globalHttpProxy, const QString &globalHttpsProxy) {
    m_localHttpProxy = localHttpProxy;
    m_localHttpsProxy = localHttpsProxy;
    m_globalHttpProxy = globalHttpProxy;
    m_globalHttpsProxy = globalHttpsProxy;
    localHttpProxySet();
}

void GitConfig::processEnqueue(const int command, const QStringList &arguments) {
    m_queue.enqueue(QVariantHash{
        {"command", command},
        {"arguments", arguments},
    });
    if (m_process->state() == QProcess::NotRunning) processDequeue();
}

void GitConfig::processDequeue() {
    if (m_process->state() != QProcess::NotRunning) return;
    if (m_queue.isEmpty()) return;
    const auto &session = m_queue.dequeue();
    m_command = session["command"].toInt();
    m_process->start("git", session["arguments"].toStringList());
}

void GitConfig::processFinished(const int exitcode) {
    const auto output = m_process->readAllStandardOutput();
    const auto error = m_process->readAllStandardError();
    const auto command = m_command;
    m_command = GitCommand::Null;
    // output
    if (exitcode == 0) {
        // output parser
        switch (command) {
            case GitCommand::LocalHttpProxyGet: m_proxies.insert("localHttpProxy", QString::fromUtf8(output).trimmed());
                break;
            case GitCommand::LocalHttpsProxyGet: m_proxies.insert("localHttpsProxy", QString::fromUtf8(output).trimmed());
                break;
            case GitCommand::GlobalHttpProxyGet: m_proxies.insert("globalHttpProxy", QString::fromUtf8(output).trimmed());
                break;
            case GitCommand::GlobalHttpsProxyGet: m_proxies.insert("globalHttpsProxy", QString::fromUtf8(output).trimmed());
                break;
            default: break;
        }
        // state machine
        switch (command) {
            case GitCommand::LocalHttpProxyGet: localHttpsProxyGet();
                break;
            case GitCommand::LocalHttpsProxyGet: globalHttpProxyGet();
                break;
            case GitCommand::GlobalHttpProxyGet: globalHttpsProxyGet();
                break;
            case GitCommand::GlobalHttpsProxyGet: emit showProxyDialog(m_proxies);
                break;
            case GitCommand::LocalHttpProxySet: localHttpsProxySet();
                break;
            case GitCommand::LocalHttpsProxySet: globalHttpProxySet();
                break;
            case GitCommand::GlobalHttpProxySet: globalHttpsProxySet();
                break;
            case GitCommand::GlobalHttpsProxySet: {
                m_localHttpProxy.clear();
                m_localHttpsProxy.clear();
                m_globalHttpProxy.clear();
                m_globalHttpsProxy.clear();
            }
            break;
            default: break;
        }
    }
    // error
    else {
        QString title{};
        QString text{};
        // error parser
        switch (command) {
            case GitCommand::LocalHttpProxyGet: m_proxies.insert("localHttpProxy", "");
                break;
            case GitCommand::LocalHttpsProxyGet: m_proxies.insert("localHttpsProxy", "");
                break;
            case GitCommand::GlobalHttpProxyGet: m_proxies.insert("globalHttpProxy", "");
                break;
            case GitCommand::GlobalHttpsProxyGet: m_proxies.insert("globalHttpsProxy", "");
                break;
            default: {
                title = tr("Git command failed");
                text = QString::fromUtf8(error).trimmed();
            }
            break;
        }
        if (!title.isEmpty() && !text.isEmpty()) emit showErrorDialog(title, text);
        // state machine
        switch (command) {
            case GitCommand::LocalHttpProxyGet: localHttpsProxyGet();
                break;
            case GitCommand::LocalHttpsProxyGet: globalHttpProxyGet();
                break;
            case GitCommand::GlobalHttpProxyGet: globalHttpsProxyGet();
                break;
            case GitCommand::GlobalHttpsProxyGet: emit showProxyDialog(m_proxies);
                break;
            case GitCommand::LocalHttpProxySet: localHttpsProxySet();
                break;
            case GitCommand::LocalHttpsProxySet: globalHttpProxySet();
                break;
            case GitCommand::GlobalHttpProxySet: globalHttpsProxySet();
                break;
            case GitCommand::GlobalHttpsProxySet: {
                m_localHttpProxy = "";
                m_localHttpsProxy = "";
                m_globalHttpProxy = "";
                m_globalHttpsProxy = "";
            }
            break;
            default: break;
        }
    }
    processDequeue();
}

void GitConfig::localHttpProxySet() {
    if (m_localHttpProxy.isEmpty()) processEnqueue(GitCommand::LocalHttpProxySet, QStringList{"config", "--local", "--unset", "http.proxy"});
    else processEnqueue(GitCommand::LocalHttpProxySet, QStringList{"config", "--local", "http.proxy", m_localHttpProxy});
}

void GitConfig::localHttpsProxySet() {
    if (m_localHttpsProxy.isEmpty()) processEnqueue(GitCommand::LocalHttpsProxySet, QStringList{"config", "--local", "--unset", "https.proxy"});
    else processEnqueue(GitCommand::LocalHttpsProxySet, QStringList{"config", "--local", "https.proxy", m_localHttpsProxy});
}

void GitConfig::globalHttpProxySet() {
    if (m_globalHttpProxy.isEmpty()) processEnqueue(GitCommand::GlobalHttpProxySet, QStringList{"config", "--global", "--unset", "http.proxy"});
    else processEnqueue(GitCommand::GlobalHttpProxySet, QStringList{"config", "--global", "http.proxy", m_globalHttpProxy});
}

void GitConfig::globalHttpsProxySet() {
    if (m_globalHttpsProxy.isEmpty()) processEnqueue(GitCommand::GlobalHttpsProxySet, QStringList{"config", "--global", "--unset", "https.proxy"});
    else processEnqueue(GitCommand::GlobalHttpsProxySet, QStringList{"config", "--global", "https.proxy", m_globalHttpsProxy});
}
