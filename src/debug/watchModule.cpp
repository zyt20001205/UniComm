#include "debug/watchModule.h"

#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "debug/debugModule.h"
#include "runtime/threadpoolModule.h"
#include "types/runtimeTypes.h"

// public
WatchModule::WatchModule()
    : DockWidget("Watch"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_model(new QStandardItemModel(0, 1, this)) {
    setWidget(m_widget);
    m_model->setItemRoleNames({
        {Qt::DisplayRole, "display"},
        {DocumentUrlRole, "documentUrl"},
        {ExpressionRole, "expression"},
        {ExpandRole, "expand"},
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

QHash<QUrl, QVariantHash> WatchModule::watchExpressionsGet() const {
    QHash<QUrl, QVariantHash> expressions{};
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const auto *item = m_model->item(row);
        const auto url = item->data(DocumentUrlRole).toUrl();
        auto &document = expressions[url];
        const auto path = "(" + item->text() + ")";
        auto open = document.value(path).value<QSet<QString>>();
        QList nodes{item};
        while (!nodes.isEmpty()) {
            const auto *node = nodes.takeLast();
            if (!node->data(ExpandRole).toBool()) continue;
            open.insert(node == item ? path : node->data(ExpressionRole).toString());
            for (int i = 0; i < node->rowCount(); ++i) nodes.append(node->child(i));
        }
        document.insert(path, QVariant::fromValue(open));
    }
    return expressions;
}

void WatchModule::watchExpressionAdd(const QUrl &documentUrl, const QString &expression) const {
    if (documentUrl.isEmpty() || expression.trimmed().isEmpty()) return;
    auto *item = new QStandardItem(expression.trimmed()); // NOLINT
    item->setColumnCount(1);
    item->setData(documentUrl, DocumentUrlRole);
    item->setData(item->text(), ExpressionRole);
    item->setData(false, ExpandRole);
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
    item->removeRows(0, item->rowCount());
    item->setData(false, ExpandRole);
    item->setData(text, ExpressionRole);
    item->setText(text);
    auto *pool = g_globalManager->threadpoolModuleGet();
    pool->watchExpressionsSet(watchExpressionsGet());
    pool->watchValuesGet(g_globalManager->debugModuleGet()->currentThreadGet());
}

void WatchModule::watchExpand(const QModelIndex &index, const bool expand) const {
    auto *item = m_model->itemFromIndex(index);
    if (!item || item->data(ExpandRole).toBool() == expand) return;
    item->setData(expand, ExpandRole);
    if (!expand) item->removeRows(0, item->rowCount());
    auto *pool = g_globalManager->threadpoolModuleGet();
    pool->watchExpressionsSet(watchExpressionsGet());
    if (expand) pool->watchValuesGet(g_globalManager->debugModuleGet()->currentThreadGet());
}

void WatchModule::watchValuesReload() const {
    QList nodes{m_model->invisibleRootItem()};
    while (!nodes.isEmpty()) {
        const auto *parent = nodes.takeLast();
        for (int row = 0; row < parent->rowCount(); ++row) {
            auto *item = parent->child(row);
            item->setData(QVariantHash{}, ResultRole);
            nodes.append(item);
        }
    }
    const auto *debug = g_globalManager->debugModuleGet();
    if (!debug->callStackGet().isEmpty()) g_globalManager->threadpoolModuleGet()->watchValuesGet(debug->currentThreadGet());
}

void WatchModule::watchValuesSet(const QString &threadId, const QUrl &documentUrl, const QVariantHash &results) const {
    if (threadId != g_globalManager->debugModuleGet()->currentThreadGet()) return;
    bool expansionChanged = false;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        auto *item = m_model->item(row);
        if (item->data(DocumentUrlRole).toUrl() != documentUrl) continue;
        watchValueSet(item, results.value("(" + item->text() + ")").toHash(), expansionChanged);
    }
    if (expansionChanged) g_globalManager->threadpoolModuleGet()->watchExpressionsSet(watchExpressionsGet());
}

// private
void WatchModule::watchValueSet(QStandardItem *item, QVariantHash result, bool &expansionChanged) const {
    auto children = result.take("children").toMap();
    if (item->data(ExpandRole).toBool() && result.value("type").toInt() != LuaType::Table) {
        item->setData(false, ExpandRole);
        expansionChanged = true;
    }
    item->setData(result, ResultRole);
    if (!item->data(ExpandRole).toBool()) children.clear();

    // update existing rows in place to preserve selection
    const bool hadChildren = item->hasChildren();
    for (int row = item->rowCount() - 1; row >= 0; --row) {
        auto *child = item->child(row);
        const auto path = child->data(ExpressionRole).toString();
        const auto entry = children.find(path.isEmpty() ? child->text() : path);
        if (entry == children.end()) {
            if (child->data(ExpandRole).toBool()) expansionChanged = true;
            item->removeRow(row);
        } else {
            auto value = entry.value().toHash();
            value.remove("display");
            value.remove("expression");
            watchValueSet(child, value, expansionChanged);
            children.erase(entry);
        }
    }
    for (const auto &entry: children) {
        auto value = entry.toHash();
        auto *child = new QStandardItem(value.take("display").toString());
        child->setColumnCount(1);
        child->setEditable(false);
        child->setData(false, ExpandRole);
        child->setData(value.take("expression"), ExpressionRole);
        item->appendRow(child);
        watchValueSet(child, value, expansionChanged);
    }
    if (!hadChildren && item->hasChildren()) emit expandWatch(item->child(0)->index());
}
