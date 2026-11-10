#ifndef UNICOMM_SERVICEMODULE_H
#define UNICOMM_SERVICEMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>

class LspService;
class QQuickWidget;

class ServiceModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList services READ servicesGet CONSTANT)

public:
    explicit ServiceModule();

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] LspService *lspServiceGet() const;

    [[nodiscard]] QVariantList servicesGet() const;

private:
    LspService *m_lspService{};
    const QVariantList m_services;
    QQuickWidget *m_widget{};
};

#endif //UNICOMM_SERVICEMODULE_H
