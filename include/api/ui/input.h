#ifndef UNICOMM_UI_INPUT_H
#define UNICOMM_UI_INPUT_H

#include <QString>

#include "api/ui/window.h"

class UiInput final : public UiWindow {
public:
    UiInput(const sol::optional<std::string> &title, const std::string &text, const sol::optional<std::string> &value, Ui *owner);

    [[nodiscard]] sol::optional<std::string> result() const { return m_result; }

private:
    void init(QQuickView &window) override;

    QString m_text{};
    QString m_value{};
    sol::optional<std::string> m_result{};
};

#endif //UNICOMM_UI_INPUT_H
