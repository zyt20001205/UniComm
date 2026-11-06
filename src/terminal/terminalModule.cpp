#include "terminal/terminalModule.h"

#include <QJsonArray>
#include <QQmlContext>
#include <QQuickView>
#include <QUuid>

#include "globals.h"
#include "core/globalManager.h"
#include "runtime/threadpoolModule.h"
#include "terminal/logModule.h"
#include "terminal/terminalPage.h"

// public
TerminalModule::TerminalModule(QWidget *parent)
    : QObject(parent)
      , m_config(g_workspaceConfig["terminalConfig"].toObject()),
      m_manageWindow(new QQuickView()),
      m_terminalModel(new TerminalModel(this)) {
}

TerminalModule::~TerminalModule() {
    delete m_manageWindow;
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] terminal module destructed").arg(timestamp);
}

void TerminalModule::propertySet(const QVariantHash &objects) {
    m_toolTip = qvariant_cast<QObject *>(objects["mainWindowToolTip"]);
    m_contextMenu = qvariant_cast<QObject *>(objects["terminalModuleContextMenu"]);
    const auto &terminalMenu = qvariant_cast<QObject *>(objects["terminalModuleTerminalMenu"]);
    terminalMenu->setProperty("terminalModel", QVariant::fromValue(m_terminalModel));

    const auto terminals = m_config["terminals"];
    for (const auto &value: terminals.toArray()) {
        auto session = value.toObject().toVariantHash();
        const auto id = session.take("id").toString();
        if (id.isEmpty()) continue;

        auto *item = new QStandardItem(); // NOLINT
        item->setData(id, TerminalModel::IdRole);
        item->setData(session, TerminalModel::SessionRole);
        m_terminalModel->appendRow(item);
    }

    // manage window
    m_manageWindow->setTitle(tr("Manage Terminal"));
    m_manageWindow->setTransientParent(g_mainWindow->windowHandle());

    m_manageWindow->rootContext()->setContextProperty("terminalModule", this);
    m_manageWindow->rootContext()->setContextProperty("global", g_globalManager);
    m_manageWindow->rootContext()->setContextProperty("terminalModel", QVariant::fromValue(m_terminalModel));

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

void TerminalModule::terminalSave() {
    m_manageWindow->close();

    QJsonArray config{};
    for (int i = 0; i < m_terminalModel->rowCount(); ++i) {
        const auto *item = m_terminalModel->item(i, 0);
        const auto id = item->data(TerminalModel::IdRole).toString();
        auto session = item->data(TerminalModel::SessionRole).toHash();
        session["id"] = id;
        config.append(QJsonObject::fromVariantHash(session));
    }
    m_config["terminals"] = config;
}

int TerminalModule::terminalAdd() const {
    const int row = m_terminalModel->rowCount();
    auto *item = new QStandardItem(); // NOLINT
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto session = QVariantHash({
        {"name", "new"},
        {"program", ""},
        {"arguments", ""},
        {"workingDirectory", ""},
        {"environment", ""}
    });
    item->setData(id, TerminalModel::IdRole);
    item->setData(session, TerminalModel::SessionRole);
    m_terminalModel->appendRow(item);
    return row;
}

void TerminalModule::terminalDelete(const int index) const {
    if (index >= 0 && index < m_terminalModel->rowCount()) m_terminalModel->removeRow(index);
}

void TerminalModule::terminalSwap(const int src, const int dst) const {
    if (src < 0 || src >= m_terminalModel->rowCount() || dst < 0 || dst >= m_terminalModel->rowCount() || src == dst) return;
    const auto row = m_terminalModel->takeRow(src);
    m_terminalModel->insertRow(dst, row);
}

TerminalPage *TerminalModule::terminalConstruct(const QUrl &terminalUrl, const int backend) {
    const auto parts = terminalUrl.path().split('/', Qt::SkipEmptyParts);
    if (parts.size() != 2) return nullptr;

    const auto uniqueName = terminalUrl.toString(QUrl::FullyEncoded);
    if (auto *terminalPage = m_terminalPageHash.value(uniqueName)) return terminalPage;

    QVariantHash session{};
    QString terminalTitle{};
    switch (backend) {
        case TerminalPage::Backend::Conpty: {
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
        case TerminalPage::Backend::Lua: {
            const auto &id = parts.at(1);
            terminalTitle = id;
            break;
        }
    }

    auto *terminalPage = new TerminalPage(uniqueName, session, m_config, backend); // NOLINT
    if (backend == TerminalPage::Backend::Lua) {
        const auto &id = parts.at(1);
        connect(terminalPage, &TerminalPage::readTerminal, this, [id](const QByteArray &data) { g_threadpool->inputWrite(id, data); });
    }
    terminalPage->setTitle(terminalTitle);
    terminalPage->propertySet({
        {"mainWindowToolTip", QVariant::fromValue(m_toolTip)},
        {"terminalModuleContextMenu", QVariant::fromValue(m_contextMenu)}
    });

    m_terminalPageHash.insert(uniqueName, terminalPage);
    connect(terminalPage, &TerminalPage::destroyed, this, [this, uniqueName] { m_terminalPageHash.remove(uniqueName); });
    return terminalPage;
}

void TerminalModule::terminalOpen(const QString &id, const int backend) {
    QUrl terminalUrl{};
    terminalUrl.setScheme("terminal");
    switch (backend) {
        case TerminalPage::Backend::Conpty:
            terminalUrl.setPath(QStringLiteral("/%1/%2").arg(id, QUuid::createUuid().toString(QUuid::WithoutBraces)));
            break;
        case TerminalPage::Backend::Lua:
            terminalUrl.setPath(QStringLiteral("/lua/%1").arg(id));
            break;
    }
    auto *terminalPage = terminalConstruct(terminalUrl, backend);
    if (!terminalPage) return;
    g_log->addDockWidgetAsTab(terminalPage);
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

// public
QHash<int, QByteArray> TerminalModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[IdRole] = "id";
    roles[SessionRole] = "session";
    return roles;
}
