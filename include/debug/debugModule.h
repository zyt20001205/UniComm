#ifndef UNICOMM_DEBUG_H
#define UNICOMM_DEBUG_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QHash>
#include <QJsonArray>
#include <QSet>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/runtimeTypes.h"
#include "types/scintillaTypes.h"

class QQuickWidget;

class DebugModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("DebugModule is created by C++.")
    Q_PROPERTY(QString currentThread READ currentThreadGet WRITE currentThreadSet NOTIFY currentThreadChanged)
    Q_PROPERTY(QVariantList callStack READ callStackGet NOTIFY callStackChanged)

public:
    explicit DebugModule();

    ~DebugModule() override;

    void propertySet();

    [[nodiscard]] QJsonArray threadsGet() const;

    [[nodiscard]] QString currentThreadGet() const { return m_currentThread; }

    void currentThreadSet(const QString &threadId);

    [[nodiscard]] QVariantList callStackGet() const { return m_callStacks.value(m_currentThread); }

    Q_INVOKABLE void interpreterStateSet(InterpreterState::Value state);

    void callStackSet(const QString &threadId, const QVariantList &frames);

signals:
    void currentThreadChanged();

    void callStackChanged();

    void openDocument(const QUrl &documentUrl);

    void getIndex();

    void setIndex(const QUrl &documentUrl, int line, int character);

    void addMarker(const QUrl &documentUrl, ScintillaMarker::Value type, int line, int time);

    void deleteMarker(const QUrl &documentUrl, ScintillaMarker::Value type, int line);

    void clearEolAnnotation(const QUrl &documentUrl);

    void setEolAnnotation(const QUrl &documentUrl, int line, const QString &text);

    void setThreadStatus(const QString &threadId, ThreadStatus::Value status);

    void setInterpreterState(const QString &threadId, InterpreterState::Value state);

private:
    void debugRefresh();

    QQuickWidget *m_widget{};
    QString m_currentThread{};
    QHash<QString, QVariantList> m_callStacks{};
    QSet<QUrl> m_markedUrls{};
};

#endif //UNICOMM_DEBUG_H
