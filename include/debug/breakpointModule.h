#ifndef UNICOMM_BREAKPOINTMODULE_H
#define UNICOMM_BREAKPOINTMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QReadWriteLock>
#include <QUrl>
#include <QVariant>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/scintillaTypes.h"

class QQuickWidget;

class BreakpointModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("BreakpointModule is created by C++.")
    Q_PROPERTY(QVariantList breakpoints READ breakpointsGet NOTIFY breakpointsChanged)

public:
    explicit BreakpointModule();

    ~BreakpointModule() override;

    void propertySet();

    void breakpointConfigSave() const;

    [[nodiscard]] QVariantList breakpointsGet() const;

    [[nodiscard]] QVariantHash breakpointGet(const QUrl &documentUrl, int line) const;

    void breakpointAdd(const QUrl &documentUrl, int line);

    Q_INVOKABLE void breakpointDelete(const QUrl &documentUrl, int line);

    Q_INVOKABLE void breakpointConfigure(const QUrl &documentUrl, int line, const QVariantHash &session);

    void breakpointToggle(const QUrl &documentUrl, int line);

signals:
    void breakpointsChanged();

    void showBreakpoint(const QUrl &documentUrl, int line);

    void setIndex(const QUrl &documentUrl, int line, int character);

    void addMarker(const QUrl &documentUrl, ScintillaMarker::Value type, int line, int time);

    void deleteMarker(const QUrl &documentUrl, ScintillaMarker::Value type, int line);

private:
    void breakpointShow(const QUrl &documentUrl, int line);

    QQuickWidget *m_widget{};
    mutable QReadWriteLock m_lock{};
    QHash<QUrl, QHash<int, QVariantHash>> m_breakpoints{};
};

#endif //UNICOMM_BREAKPOINTMODULE_H
