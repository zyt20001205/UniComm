#ifndef UNICOMM_WELCOMEPAGE_H
#define UNICOMM_WELCOMEPAGE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>

class QQuickWidget;

class WelcomePage final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT

public:
    explicit WelcomePage();

    ~WelcomePage() override = default;

    void propertySet();

    Q_INVOKABLE void workspaceOpen();

    Q_INVOKABLE void documentOpen(const QUrl &documentUrl);

signals:
    void openWorkspace();

    void openDocument(const QUrl &documentUrl);

private:
    QQuickWidget *m_welcomeWidget{};
};

#endif //UNICOMM_WELCOMEPAGE_H
