#include "debug/breakpointModule.h"

#include <QQmlContext>
#include <QQuickWidget>
#include <QStandardItemModel>

#include "globals.h"
#include "core/globalManager.h"
#include "types/scintillaTypes.h"

// public
BreakpointModule::BreakpointModule()
    : DockWidget("Breakpoint"),
      m_widget(new QQuickWidget()),
      m_standardItemModel(new BreakpointModel(this)) {
    setWidget(m_widget);
    m_widget->installEventFilter(this);
    auto breakpointConfig = g_workspaceConfig["breakpointConfig"].toObject();
    for (const auto &key: breakpointConfig.keys()) {
        const QUrl url(key);
        const auto breakpointLineHash = breakpointConfig[key].toObject();
        for (auto it = breakpointLineHash.begin(); it != breakpointLineHash.end(); ++it) {
            const int line = it.key().toInt();
            const QVariantHash session = it.value().toObject().toVariantHash();
            breakpointInsert(url, line, session);
        }
    }
}

BreakpointModule::~BreakpointModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void BreakpointModule::propertySet(const QVariantHash &objects) {
    m_widget->rootContext()->setContextProperty("breakpointModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("lineMenu", qvariant_cast<QObject *>(objects["breakpointModuleLineMenu"]));
    m_widget->rootContext()->setContextProperty("fileMenu", qvariant_cast<QObject *>(objects["breakpointModuleFileMenu"]));
    m_widget->rootContext()->setContextProperty("rootMenu", qvariant_cast<QObject *>(objects["breakpointModuleRootMenu"]));
    m_widget->rootContext()->setContextProperty("standardItemModel", m_standardItemModel);

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/debug/breakpointModule.qml"));
}

void BreakpointModule::propertyGet(const QVariantMap &objects) {
    m_treeView = qvariant_cast<QObject *>(objects["treeView"]);
}

void BreakpointModule::breakpointConfigSave() {
    auto breakpointHash = QJsonObject();
    for (const auto &url: g_breakpoints.keys()) {
        auto breakpointLineHash = QJsonObject();
        for (auto it = g_breakpoints[url].begin(); it != g_breakpoints[url].end(); ++it) {
            const int line = it.key();
            const QVariantHash &session = it.value();
            breakpointLineHash.insert(QString::number(line), QJsonObject::fromVariantHash(session));
        }
        breakpointHash.insert(url.toString(), breakpointLineHash);
    }
    g_workspaceConfig["breakpointConfig"] = breakpointHash;
}

void BreakpointModule::breakpointInsert(const QUrl &documentUrl, const int line, const QVariantHash &session) {
    // update g_breakpoints
    g_breakpoints[documentUrl][line] = session;
    // update scintilla
    if (g_breakpoints[documentUrl][line]["enabled"].toBool()) {
        emit addMarker(documentUrl, ScintillaMarker::BreakpointEnabled, line - 1, -1);
    } else {
        emit addMarker(documentUrl, ScintillaMarker::BreakpointDisabled, line - 1, -1);
    }
    // update model
    auto *lineItem = new QStandardItem(QString::number(line)); // NOLINT
    lineItem->setData(documentUrl, BreakpointModel::DocumentUrlRole);
    lineItem->setData(session["enabled"].toBool(), BreakpointModel::EnableRole);
    lineItem->setData(session["condition"].toString(), BreakpointModel::ConditionRole);
    const auto *indent0 = m_standardItemModel->invisibleRootItem();
    for (int i = 0; i < indent0->rowCount(); ++i) {
        auto *indent1 = indent0->child(i);
        if (indent1->data(BreakpointModel::DocumentUrlRole).toUrl() == documentUrl) {
            for (int j = 0; j < indent1->rowCount(); ++j) {
                const auto *indent2 = indent1->child(j);
                if (line < indent2->text().toInt()) {
                    indent1->insertRow(j, lineItem);
                    return;
                }
            }
            indent1->appendRow(lineItem);
            return;
        }
    }
    auto *urlItem = new QStandardItem(documentUrl.fileName()); // NOLINT
    urlItem->setData(documentUrl, BreakpointModel::DocumentUrlRole);
    urlItem->appendRow(lineItem);
    m_standardItemModel->appendRow(urlItem);
}

void BreakpointModule::breakpointRemove(const QUrl &documentUrl, const int line) {
    // update scintilla
    emit deleteMarker(documentUrl, ScintillaMarker::BreakpointEnabled, line - 1);
    emit deleteMarker(documentUrl, ScintillaMarker::BreakpointDisabled, line - 1);
    // update g_breakpoints
    g_breakpoints[documentUrl].remove(line);
    if (g_breakpoints[documentUrl].isEmpty()) g_breakpoints.remove(documentUrl);
    // update model
    const auto *indent0 = m_standardItemModel->invisibleRootItem();
    for (int i = 0; i < indent0->rowCount(); ++i) {
        auto *indent1 = indent0->child(i);
        if (indent1->data(BreakpointModel::DocumentUrlRole).toUrl() == documentUrl) {
            for (int j = 0; j < indent1->rowCount(); ++j) {
                const auto *indent2 = indent1->child(j);
                if (line == indent2->text().toInt()) {
                    indent1->removeRow(j);
                    if (indent1->rowCount() == 0) {
                        m_standardItemModel->removeRow(i);
                    }
                    return;
                }
            }
        }
    }
}

void BreakpointModule::breakpointReload(const QUrl &documentUrl, const int line) {
    const auto session = g_breakpoints[documentUrl][line];
    breakpointRemove(documentUrl, line);
    breakpointInsert(documentUrl, line, session);
}

void BreakpointModule::documentOpen(const QUrl &documentUrl) {
    emit openDocument(documentUrl);
}

void BreakpointModule::markerAdd(const QUrl &documentUrl, const int line) {
    emit addMarker(documentUrl, ScintillaMarker::Hint, line - 1, 1000);
}

void BreakpointModule::breakpointDelete(const QUrl &documentUrl, const int line) {
    breakpointRemove(documentUrl, line);
}

void BreakpointModule::breakpointsDelete(const QUrl &documentUrl) {
    const auto *indent0 = m_standardItemModel->invisibleRootItem();
    for (int i = 0; i < indent0->rowCount(); ++i) {
        auto *indent1 = indent0->child(i);
        if (indent1->data(BreakpointModel::DocumentUrlRole).toUrl() == documentUrl) {
            for (int j = indent1->rowCount() - 1; j >= 0; --j) {
                const auto *indent2 = indent1->child(j);
                const auto line = indent2->text().toInt();
                breakpointDelete(documentUrl, line);
            }
        }
    }
}

void BreakpointModule::allDelete() {
    const auto *indent0 = m_standardItemModel->invisibleRootItem();
    for (int i = indent0->rowCount() - 1; i >= 0; --i) {
        const auto *indent1 = indent0->child(i);
        const auto documentUrl = indent1->data(BreakpointModel::DocumentUrlRole).toUrl();
        breakpointsDelete(documentUrl);
    }
}

bool BreakpointModule::enabledGet(const QUrl &documentUrl, const int line) {
    return g_breakpoints[documentUrl][line]["enabled"].toBool();
}

void BreakpointModule::enabledSet(const QUrl &documentUrl, const int line, const bool status) {
    g_breakpoints[documentUrl][line]["enabled"] = status;
}

QString BreakpointModule::conditionGet(const QUrl &documentUrl, const int line) {
    return g_breakpoints[documentUrl][line]["condition"].toString();
}

void BreakpointModule::conditionSet(const QUrl &documentUrl, const int line, const QString &condition) {
    g_breakpoints[documentUrl][line]["condition"] = condition;
}

bool BreakpointModule::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_widget) {
        if (event->type() == QEvent::FocusOut) {
            m_treeView->setProperty("selectedRow", -1);
        }
    }
    return DockWidget::eventFilter(watched, event);
}

// public
BreakpointModel::BreakpointModel(QObject *parent)
    : QStandardItemModel(parent) {
    connect(this, &QAbstractItemModel::rowsInserted, this, &BreakpointModel::emptyChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &BreakpointModel::emptyChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &BreakpointModel::emptyChanged);
}

QHash<int, QByteArray> BreakpointModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[DocumentUrlRole] = "documentUrl";
    roles[EnableRole] = "enable";
    roles[ConditionRole] = "condition";
    return roles;
}
