#include "debug/debugModule.h"

#include <QHeaderView>
#include <QInputDialog>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QStandardItemModel>
#include <QStringListModel>

#include "globals.h"
#include "core/globalManager.h"
#include "runtime/luaInterpreter.h"
#include "document/documentModule.h"
#include "types/scintillaTypes.h"

// public
DebugModule::DebugModule()
    : DockWidget("Debug"),
      m_widget(new QQuickWidget()),
      m_stringListModel(new QStringListModel(this)) {
    setWidget(m_widget);
}

DebugModule::~DebugModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void DebugModule::propertySet(const QVariantHash &objects) {
    m_errorDialog = qvariant_cast<QObject *>(objects["debugModuleErrorDialog"]);

    m_widget->rootContext()->setContextProperty("debugModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("mainToolTip", qvariant_cast<QObject *>(objects["mainWindowToolTip"]));
    m_widget->rootContext()->setContextProperty("stringListModel", m_stringListModel);
    m_widget->rootContext()->setContextProperty("standardItemModel", new QStandardItemModel());

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/debug/debugModule.qml"));
}

void DebugModule::propertyGet(const QVariantMap &objects) {
    m_threadComboBox = qvariant_cast<QObject *>(objects["threadComboBox"]);
}

void DebugModule::debugStart(const QString &threadId) const {
    QStringList threads = m_stringListModel->stringList();
    threads.append(threadId);
    m_stringListModel->setStringList(threads);
}

void DebugModule::debugStop(const QString &threadId) {
    // string list model
    QStringList threads = m_stringListModel->stringList();
    threads.removeOne(threadId);
    m_stringListModel->setStringList(threads);
    // standard item model
    m_callStackModelHash.remove(threadId);
}

QString DebugModule::threadGet() const {
    return m_threadComboBox->property("currentText").toString();
}

void DebugModule::stateSet(const QString &threadId, const int state) {
    const auto &currenThread = m_threadComboBox->property("currentText").toString();
    if (currenThread.isEmpty()) {
        QMetaObject::invokeMethod(m_errorDialog, "open");
        return;
    }
    if (state == Debug::RunToCursor) {
        emit getIndex();
        const QString &currentThreadId = currenThread;
        emit setState(currentThreadId, state);
    } else {
        emit setState(threadId, state);
    }
}

void DebugModule::callStackInsert(const QString &threadId, QStandardItemModel *callStackModel) {
    if (!m_callStackModelHash.contains(threadId)) {
        m_callStackModelHash.insert(threadId, callStackModel);
    } else {
        m_callStackModelHash[threadId] = callStackModel;
        m_widget->rootContext()->setContextProperty("standardItemModel", callStackModel);
    }
}

void DebugModule::callStackSwitch(const QString &threadId) const {
    if (threadId.isEmpty()) {
        m_widget->rootContext()->setContextProperty("standardItemModel", nullptr);
        return;
    }
    auto *callStackModel = m_callStackModelHash.value(threadId, nullptr);
    if (!callStackModel) {
        m_widget->rootContext()->setContextProperty("standardItemModel", nullptr);
        return;
    }
    m_widget->rootContext()->setContextProperty("standardItemModel", callStackModel);
}

void DebugModule::markerAdd(const QVariantHash &position) {
    emit addMarker(
        position["documentUrl"].toUrl(),
        ScintillaMarker::Hint,
        position["line"].toInt() - 1,
        1000);
}
