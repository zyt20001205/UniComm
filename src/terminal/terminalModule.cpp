#include "terminal/terminalModule.h"

#include <QJsonArray>
#include <QQuickView>
#include <QUuid>

#include "globals.h"
#include "mainWindow/mainWindow.h"
#include "core/globalManager.h"
#include "core/logModule.h"
#include "runtime/threadpoolModule.h"
#include "terminal/terminalPage.h"

// public
TerminalModule::TerminalModule(QWidget *parent)
    : QObject(parent),
      m_config(g_workspaceConfig["terminalConfig"].toObject()),
      m_manageWindow(new QQuickView(g_globalManager->engineGet(), nullptr)) {
}

TerminalModule::~TerminalModule() {
    delete m_manageWindow;
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] terminal module destructed").arg(timestamp);
}

void TerminalModule::propertySet() const {
    // manage window
    m_manageWindow->setTitle(tr("Manage Terminal"));
    m_manageWindow->setTransientParent(g_globalManager->mainWindowGet()->windowHandle());
    m_manageWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    m_manageWindow->setSource(QUrl("qrc:/qml/terminal/terminalManageWindow.qml"));
}

void TerminalModule::terminalConfigSave() const {
    g_workspaceConfig["terminalConfig"] = m_config;
}

void TerminalModule::terminalManage() const {
    m_manageWindow->resize(1080, 720);
    m_manageWindow->show();
}

QVariantList TerminalModule::terminalsGet() const {
    return m_config["terminals"].toArray().toVariantList();
}

void TerminalModule::terminalSave(const QVariantList &terminals) {
    QJsonArray config{};
    for (const auto &terminal: terminals) {
        auto session = QJsonObject::fromVariantMap(terminal.toMap());
        if (session["id"].toString().isEmpty()) session["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        config.append(session);
    }
    if (config != m_config["terminals"].toArray()) {
        m_config["terminals"] = config;
        emit terminalsChanged();
    }
    m_manageWindow->close();
}

TerminalPage *TerminalModule::terminalConstruct(const QUrl &terminalUrl, const TerminalBackend::Value backend) {
    const auto parts = terminalUrl.path().split('/', Qt::SkipEmptyParts);
    if (parts.size() != 2) return nullptr;

    const auto uniqueName = terminalUrl.toString(QUrl::FullyEncoded);
    if (auto *terminalPage = m_terminalPageHash.value(uniqueName)) return terminalPage;

    QVariantHash session{};
    QString terminalTitle{};
    switch (backend) {
        case TerminalBackend::Process: {
            const auto &id = parts.at(0);
            for (const auto &value: m_config["terminals"].toArray()) {
                auto candidate = value.toObject().toVariantHash();
                if (candidate.take("id").toString() != id) continue;
                session = candidate;
                terminalTitle = session["name"].toString();
                break;
            }
            if (session.isEmpty()) return nullptr;
            break;
        }
        case TerminalBackend::Lua: {
            const auto &id = parts.at(1);
            terminalTitle = id;
            break;
        }
    }

    auto *terminalPage = new TerminalPage(uniqueName, session, m_config, backend); // NOLINT
    if (backend == TerminalBackend::Lua) {
        const auto &id = parts.at(1);
        connect(terminalPage, &TerminalPage::readTerminal, this, [id](const QByteArray &data) { g_globalManager->threadpoolModuleGet()->inputWrite(id, data); });
    }
    terminalPage->setTitle(terminalTitle);
    terminalPage->propertySet();

    m_terminalPageHash.insert(uniqueName, terminalPage);
    connect(terminalPage, &TerminalPage::destroyed, this, [this, uniqueName] { m_terminalPageHash.remove(uniqueName); });
    return terminalPage;
}

void TerminalModule::terminalOpen(const QString &id, const TerminalBackend::Value backend) {
    QUrl terminalUrl{};
    terminalUrl.setScheme("terminal");
    switch (backend) {
        case TerminalBackend::Process:
            terminalUrl.setPath(QStringLiteral("/%1/%2").arg(id, QUuid::createUuid().toString(QUuid::WithoutBraces)));
            break;
        case TerminalBackend::Lua:
            terminalUrl.setPath(QStringLiteral("/lua/%1").arg(id));
            break;
    }
    auto *terminalPage = terminalConstruct(terminalUrl, backend);
    if (!terminalPage) return;
    g_globalManager->logModuleGet()->addDockWidgetAsTab(terminalPage);
}

void TerminalModule::terminalClose(const QString &id) const {
    QUrl terminalUrl{};
    terminalUrl.setScheme("terminal");
    terminalUrl.setPath(QStringLiteral("/lua/%1").arg(id));
    if (auto *terminalPage = m_terminalPageHash.value(terminalUrl.toString(QUrl::FullyEncoded))) terminalPage->close();
}

void TerminalModule::terminalWrite(const QString &id, const QByteArray &data) const {
    QUrl terminalUrl{};
    terminalUrl.setScheme("terminal");
    terminalUrl.setPath(QStringLiteral("/lua/%1").arg(id));
    if (auto *terminalPage = m_terminalPageHash.value(terminalUrl.toString(QUrl::FullyEncoded))) terminalPage->write(data);
}
