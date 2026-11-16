#ifndef UNICOMM_SERVICEMODULE_H
#define UNICOMM_SERVICEMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>

class GrepService;
class LspManager;
class McpService;
class NetworkService;
class QQuickWidget;

class ServiceModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList services READ servicesGet CONSTANT)

public:
    explicit ServiceModule();

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] LspManager *lspManagerGet() const {
        return m_lspManager;
    }

    [[nodiscard]] GrepService *grepServiceGet() const {
        return m_grepService;
    }

    [[nodiscard]] NetworkService *networkServiceGet() const {
        return m_networkService;
    }

    [[nodiscard]] QVariantList servicesGet() const;

private:
    LspManager *m_lspManager{};
    McpService *m_mcpService{};
    GrepService *m_grepService{};
    NetworkService *m_networkService{};
    QQuickWidget *m_widget{};
};

#endif //UNICOMM_SERVICEMODULE_H
