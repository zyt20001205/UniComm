#include "analysis/structureModule.h"

#include <QJsonArray>
#include <QQuickWidget>
#include <QStandardItemModel>

#include "globals.h"
#include "core/globalManager.h"

// public
StructureModule::StructureModule()
    : DockWidget("Structure"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)) {
    setWidget(m_widget);
    m_widget->installEventFilter(this);
}

StructureModule::~StructureModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void StructureModule::propertySet() {
    m_widget->setInitialProperties({{"structureModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/analysis/structureModule.qml"));
}

QVariantHash StructureModule::modelsGet() const {
    QVariantHash models;
    for (auto it = m_documentSymbolModelHash.cbegin(); it != m_documentSymbolModelHash.cend(); ++it)
        models.insert(it.key().toString(), QVariantHash{
            {"model", QVariant::fromValue<QAbstractItemModel *>(it.value())},
            {"kinds", it.value()->property("kinds")}
        });
    return models;
}

void StructureModule::documentSymbolResponse(const QUrl &documentUrl, const QJsonArray &result) {
    if (result.isEmpty()) {
        if (auto *model = m_documentSymbolModelHash.take(documentUrl)) {
            emit changeModels();
            model->deleteLater();
        }
        return;
    }

    auto *model = m_documentSymbolModelHash.value(documentUrl);
    if (model) {
        model->setRowCount(0);
    } else {
        model = new QStandardItemModel(this);
        model->setItemRoleNames({
            {KindRole, "kind"},
            {Qt::DisplayRole, "display"},
            {DetailRole, "detail"},
            {StartLineRole, "startLine"},
            {StartCharacterRole, "startCharacter"}
        });
        m_documentSymbolModelHash.insert(documentUrl, model);
    }

    std::set<int> kinds;
    documentSymbolSet(result, model->invisibleRootItem(), kinds);
    QVariantList kindList;
    for (const int kind: kinds) kindList.append(kind);
    model->setProperty("kinds", kindList);
    emit changeModels();
}

bool StructureModule::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_widget && event->type() == QEvent::FocusOut)
        emit clearSelection();
    return DockWidget::eventFilter(watched, event);
}

// private
void StructureModule::documentSymbolSet(const QJsonArray &result, QStandardItem *parentItem, std::set<int> &kinds) {
    for (const auto &value: result) {
        const auto symbol = value.toObject();
        const auto kind = symbol["kind"].toInt();
        kinds.insert(kind);
        const auto detail = symbol["detail"].toString().trimmed();
        const auto name = symbol["name"].toString();
        const auto range = symbol["range"].toObject();
        const auto start = range["start"].toObject();
        auto *item = new QStandardItem(name); // NOLINT
        item->setData(kind, KindRole);
        item->setData(detail, DetailRole);
        item->setData(start["line"].toInt(), StartLineRole);
        item->setData(start["character"].toInt(), StartCharacterRole);
        parentItem->appendRow(item);
        if (symbol.contains("children")) {
            QJsonArray children = symbol["children"].toArray();
            documentSymbolSet(children, item, kinds);
        }
    }
}
