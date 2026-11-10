#ifndef UNICOMM_SERVICEMODULE_H
#define UNICOMM_SERVICEMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>

class LspManager;
class McpService;
class QQuickWidget;

class ServiceModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList services READ servicesGet CONSTANT)

public:
    explicit ServiceModule();

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] LspManager *lspManagerGet() const;

    [[nodiscard]] QVariantList servicesGet() const;

private:
    LspManager *m_lspManager{};
    McpService *m_mcpService{};
    QQuickWidget *m_widget{};
};

#endif //UNICOMM_SERVICEMODULE_H
