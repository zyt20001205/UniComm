#ifndef UNICOMM_UI_PLOT_H
#define UNICOMM_UI_PLOT_H

#include <QStringList>
#include <sol/table.hpp>

#include "api/ui/window.h"

class UiPlot final : public UiWindow {
public:
    UiPlot(const sol::optional<std::string> &title, const sol::table &keys, Ui *owner);

    void append(double x, const sol::table &values) const;

    void close();

protected:
    void init(QQuickView &window) override;

private:
    QStringList m_keys{};
};

#endif //UNICOMM_UI_PLOT_H
