#ifndef UNICOMM_DEBUG_H
#define UNICOMM_DEBUG_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QHash>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/debugTypes.h"
#include "types/scintillaTypes.h"

class QQuickWidget;

class DebugModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("DebugModule is created by C++.")
    Q_PROPERTY(QStringList threads READ threadsGet NOTIFY threadsChanged)
    Q_PROPERTY(QString currentThread READ currentThreadGet WRITE currentThreadSet NOTIFY currentThreadChanged)
    Q_PROPERTY(QVariantList callStack READ callStackGet NOTIFY callStackChanged)

public:
    explicit DebugModule();

    ~DebugModule() override;

    void propertySet();

    [[nodiscard]] QStringList threadsGet() const { return m_threads; }

    [[nodiscard]] QString currentThreadGet() const { return m_currentThread; }

    void currentThreadSet(const QString &threadId);

    [[nodiscard]] QVariantList callStackGet() const { return m_callStacks.value(m_currentThread); }

    void debugStart(const QString &threadId);

    void debugStop(const QString &threadId);

    Q_INVOKABLE void stateSet(Debug::Value state);

    void callStackSet(const QString &threadId, const QVariantList &frames);

signals:
    void threadsChanged();

    void currentThreadChanged();

    void callStackChanged();

    void getIndex();

    void addMarker(const QUrl &documentUrl, ScintillaMarker::Value type, int line, int time);

    void setState(const QString &threadId, Debug::Value state);

private:
    QQuickWidget *m_widget{};
    QStringList m_threads{};
    QString m_currentThread{};
    QHash<QString, QVariantList> m_callStacks{};
};

#endif //UNICOMM_DEBUG_H
