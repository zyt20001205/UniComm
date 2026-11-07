#include "api/ui/input.h"

#include <QQuickItem>
#include <QQuickView>

// public
UiInput::UiInput(const sol::optional<std::string> &title, const std::string &text, const sol::optional<std::string> &value, Ui *owner)
    : UiWindow(title, owner),
      m_text(QString::fromStdString(text)),
      m_value(QString::fromStdString(value.value_or(""))) {
}

// private
void UiInput::init(QQuickView &window) {
    window.setFlags(Qt::Dialog);
    window.setInitialProperties({{"text", m_text}, {"value", m_value}});
    window.setSource(QUrl("qrc:/qml/api/ui/input.qml"));
    QObject::connect(&window, &QQuickWindow::closing, &window, [this, &window] {
        if (const auto *root = window.rootObject(); root && root->property("accepted").toBool()) m_result = root->property("value").toString().toStdString();
    });
}
