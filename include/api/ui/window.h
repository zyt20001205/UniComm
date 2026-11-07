#ifndef UNICOMM_UI_WINDOW_H
#define UNICOMM_UI_WINDOW_H

#include <QPointer>
#include <sol/optional.hpp>

#include <string>

class QQuickView;
class Ui;

class UiWindow {
public:
    virtual ~UiWindow() = default;

    void open(bool blocking);

protected:
    UiWindow(const sol::optional<std::string> &title, Ui *owner);

    virtual void init(QQuickView &window) = 0;

    QPointer<QQuickView> m_window{};

private:
    sol::optional<std::string> m_title{};
    Ui *m_owner{};
};

#endif //UNICOMM_UI_WINDOW_H
