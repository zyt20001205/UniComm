#include "api/ui/message.h"

#include <QQuickItem>
#include <QQuickView>
#include <sol/error.hpp>

// public
UiMessage::UiMessage(const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button, Ui *owner)
    : UiWindow(title, owner),
      m_text(QString::fromStdString(text)) {
    if (button.has_value()) {
        m_buttons.clear();
        for (const auto &value: button.value() | std::views::values) {
            const auto name = QString::fromStdString(value.as<std::string>());
            if (name != "ok" && name != "cancel") throw sol::error("button must be ok or cancel");
            m_buttons.append(name);
        }
    }
    if (m_buttons.isEmpty()) throw sol::error("button must not be empty");
}

// private
void UiMessage::init(QQuickView &window) {
    window.setFlags(Qt::Dialog);
    window.setInitialProperties({{"text", m_text}, {"button", m_buttons}});
    window.setSource(QUrl("qrc:/qml/api/ui/message.qml"));
    QObject::connect(&window, &QQuickWindow::closing, &window, [this, &window] {
        if (const auto *root = window.rootObject()) m_result = root->property("accepted").toBool();
    });
}
