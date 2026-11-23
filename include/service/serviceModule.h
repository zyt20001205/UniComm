#ifndef UNICOMM_SERVICEMODULE_H
#define UNICOMM_SERVICEMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QtQmlIntegration/qqmlintegration.h>

class GitService;
class GrepService;
class LspManager;
class McpService;
class NetworkService;
class QQuickWidget;

Q_MOC_INCLUDE("service/git/gitService.h")

class ServiceModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ServiceModule is created by C++.")
    Q_PROPERTY(GitService *git READ gitServiceGet CONSTANT)
    Q_PROPERTY(QVariantList services READ servicesGet CONSTANT)

public:
    explicit ServiceModule();

    void propertySet();

    [[nodiscard]] GitService *gitServiceGet() const {
        return m_gitService;
    }

    [[nodiscard]] GrepService *grepServiceGet() const {
        return m_grepService;
    }

    [[nodiscard]] LspManager *lspManagerGet() const {
        return m_lspManager;
    }

    [[nodiscard]] NetworkService *networkServiceGet() const {
        return m_networkService;
    }

    [[nodiscard]] QVariantList servicesGet() const;

private:
    GitService *m_gitService{};
    GrepService *m_grepService{};
    LspManager *m_lspManager{};
    McpService *m_mcpService{};
    NetworkService *m_networkService{};
    QQuickWidget *m_widget{};
};

#endif //UNICOMM_SERVICEMODULE_H
