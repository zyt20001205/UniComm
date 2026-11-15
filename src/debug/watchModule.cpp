#include "debug/watchModule.h"

#include <QJsonArray>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QTimer>

#include "globals.h"
#include "core/globalManager.h"

// public
WatchModule::WatchModule()
    : DockWidget("Watch"),
      m_widget(new QQuickWidget()) {
    setWidget(m_widget);
    m_model = new QStandardItemModel(this);
    auto watchConfig = g_workspaceConfig["watchConfig"].toArray();
    for (const auto &value: watchConfig) {
        const auto pair = value.toArray();
        const auto documentUrl = QUrl(pair[0].toString());
        const QString expression = pair[1].toString();
        watchInsert(-1, documentUrl, expression);
    }
}

WatchModule::~WatchModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void WatchModule::propertySet(const QVariantHash &objects) {
    m_widget->rootContext()->setContextProperty("watchModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("mainToolTip", qvariant_cast<QObject *>(objects["mainWindowToolTip"]));
    m_widget->rootContext()->setContextProperty("expressionMenu", qvariant_cast<QObject *>(objects["watchModuleExpressionMenu"]));
    m_widget->rootContext()->setContextProperty("valueMenu", qvariant_cast<QObject *>(objects["watchModuleValueMenu"]));
    m_widget->rootContext()->setContextProperty("rootMenu", qvariant_cast<QObject *>(objects["watchModuleRootMenu"]));

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/debug/watchModule.qml"));
    m_item = m_widget->rootObject();
}

void WatchModule::watchConfigSave() {
    auto watchArray = QJsonArray();
    for (int i = 0; i < m_model->rowCount(); ++i) {
        const QString url = m_model->item(i, 0)->data(Qt::WhatsThisRole).toString();
        const QString expression = m_model->item(i, 0)->text();
        watchArray.append(QJsonArray({url, expression}));
    }
    g_workspaceConfig["watchConfig"] = watchArray;
}

void WatchModule::watchInsert(int index, const QUrl &documentUrl, const QString &expression) {
    index = m_model->rowCount();
    auto *expressionItem = new QStandardItem(expression); // NOLINT
    expressionItem->setData(documentUrl, Qt::WhatsThisRole);
    auto *valueItem = new QStandardItem("nil"); // NOLINT
    valueItem->setData("nil", Qt::WhatsThisRole);
    m_model->insertRow(index, {expressionItem, valueItem});
}

void WatchModule::watchRemove(const int index) {
    m_model->removeRow(index);
}

void WatchModule::watchRename(const int index, const QUrl &documentUrl, const QString &expression) {
    m_model->item(index, 0)->setData(documentUrl, Qt::WhatsThisRole);
    m_model->item(index, 0)->setText(expression);
}

void WatchModule::watchSwap(const int src, const int dst) const {
    const auto tmp = m_model->takeRow(src);
    m_model->insertRow(dst, tmp);
}

void WatchModule::watchClear(const int index) {
    if (index == -1) {
        for (int i = 0; i < m_model->rowCount(); ++i) {
            m_model->item(i, 1)->setText("");
        }
    } else {
        m_model->item(index, 1)->setText("");
    }
}
