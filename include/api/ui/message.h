#ifndef UNICOMM_UI_MESSAGE_H
#define UNICOMM_UI_MESSAGE_H

#include <QStringList>
#include <sol/table.hpp>

#include "api/ui/window.h"

class UiMessage final : public UiWindow {
public:
    UiMessage(const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button, Ui *owner);

    [[nodiscard]] bool result() const { return m_result; }

private:
    void init(QQuickView &window) override;

    QString m_text{};
    QStringList m_buttons{"ok"};
    bool m_result = false;
};

#endif //UNICOMM_UI_MESSAGE_H
