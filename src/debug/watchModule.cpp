#include "debug/watchModule.h"

#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "debug/debugModule.h"
#include "runtime/threadpoolModule.h"

// public
WatchModule::WatchModule()
    : DockWidget("Watch"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_model(new QStandardItemModel(this)) {
    setWidget(m_widget);
    m_model->setItemRoleNames({
        {Qt::DisplayRole, "display"},
        {DocumentUrlRole, "documentUrl"},
        {ResultRole, "result"}
    });
}

WatchModule::~WatchModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void WatchModule::propertySet() {
    const auto watches = g_workspaceConfig["watchConfig"].toArray();
    for (const auto &watch: watches) {
        const auto pair = watch.toArray();
        watchExpressionAdd(QUrl(pair[0].toString()), pair[1].toString());
    }
    m_widget->setInitialProperties({{"watchModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/debug/watchModule.qml"));
}

void WatchModule::watchConfigSave() const {
    QJsonArray watches{};
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const auto *item = m_model->item(row);
        watches.append(QJsonArray{item->data(DocumentUrlRole).toUrl().toString(), item->text()});
    }
    g_workspaceConfig["watchConfig"] = watches;
}

QMultiHash<QUrl, QString> WatchModule::watchExpressionsGet() const {
    QMultiHash<QUrl, QString> expressions{};
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const auto *item = m_model->item(row);
        const auto url = item->data(DocumentUrlRole).toUrl();
        if (!expressions.contains(url, item->text())) expressions.insert(url, item->text());
    }
    return expressions;
}

void WatchModule::watchExpressionAdd(const QUrl &documentUrl, const QString &expression) const {
    if (documentUrl.isEmpty() || expression.trimmed().isEmpty()) return;
    auto *item = new QStandardItem(expression.trimmed()); // NOLINT
    item->setData(documentUrl, DocumentUrlRole);
    item->setData(QVariantHash{}, ResultRole);
    m_model->appendRow(item);
    g_globalManager->threadpoolModuleGet()->watchExpressionsSet(watchExpressionsGet());
}

void WatchModule::watchExpressionDelete(const int row) const {
    if (!m_model->removeRow(row)) return;
    g_globalManager->threadpoolModuleGet()->watchExpressionsSet(watchExpressionsGet());
}

void WatchModule::watchExpressionSet(const int row, const QString &expression) const {
    auto *item = m_model->item(row);
    const auto text = expression.trimmed();
    if (!item || text.isEmpty() || text == item->text()) return;
    item->setData(QVariantHash{}, ResultRole);
    item->setText(text);
    auto *pool = g_globalManager->threadpoolModuleGet();
    pool->watchExpressionsSet(watchExpressionsGet());
    pool->watchValuesGet(g_globalManager->debugModuleGet()->currentThreadGet());
}

void WatchModule::watchValuesReload() const {
    for (int row = 0; row < m_model->rowCount(); ++row) m_model->item(row)->setData(QVariantHash{}, ResultRole);
    const auto *debug = g_globalManager->debugModuleGet();
    if (!debug->callStackGet().isEmpty()) g_globalManager->threadpoolModuleGet()->watchValuesGet(debug->currentThreadGet());
}

void WatchModule::watchValuesSet(const QString &threadId, const QUrl &documentUrl, const QVariantHash &results) const {
    if (threadId != g_globalManager->debugModuleGet()->currentThreadGet()) return;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        auto *item = m_model->item(row);
        if (item->data(DocumentUrlRole).toUrl() != documentUrl) continue;
        item->setData(results.value(item->text()).toHash(), ResultRole);
    }
}
