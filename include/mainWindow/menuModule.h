#ifndef UNICOMM_MENUMODULE_H
#define UNICOMM_MENUMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QQuickWidget>
#include <QtQmlIntegration/qqmlintegration.h>

class MenuModule : public QQuickWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("MenuModule is created by C++.")

public:
    explicit MenuModule(QWidget *parent = nullptr);

    ~MenuModule() override;

    void propertySet();

    Q_INVOKABLE [[nodiscard]] QAction *toggleActionGet(KDDockWidgets::QtWidgets::DockWidget *dock) const;
};

#endif //UNICOMM_MENUMODULE_H
