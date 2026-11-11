#include "api/ui/plot.h"

#include <QApplication>
#include <QQuickItem>
#include <QQuickView>
#include <QVariantMap>
#include <sol/error.hpp>

#include <vector>

// public
UiPlot::UiPlot(const sol::optional<std::string> &title, const sol::table &keys, Ui *owner)
    : UiWindow(title, owner) {
    const auto names = keys.as<sol::as_table_t<std::vector<std::string> > >();
    for (const auto &key: names) {
        const auto name = QString::fromStdString(key);
        if (m_keys.contains(name)) throw sol::error("duplicate plot key: " + key);
        m_keys.append(name);
    }
}

void UiPlot::append(const double x, const sol::table &values) const {
    QVariantMap data{};
    for (const auto &[key, value]: values) {
        const auto name = key.as<std::string>();
        const auto _name = QString::fromStdString(name);
        if (!m_keys.contains(_name)) throw sol::error("unknown plot key: " + name);
        data.insert(_name, value.as<double>());
    }

    QString exception{};
    QMetaObject::invokeMethod(qApp, [this, x, &data]() -> QString {
        if (!m_window || !m_window->isVisible()) return "plot window is closed";
        QMetaObject::invokeMethod(m_window->rootObject(), "append", Q_ARG(QVariant, x), Q_ARG(QVariant, data));
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());
}

void UiPlot::close() {
    QMetaObject::invokeMethod(qApp, [this] {
        if (!m_window) return;
        m_window->close();
        m_window.clear();
    }, Qt::BlockingQueuedConnection);
}

// protected
void UiPlot::init(QQuickView &window) {
    window.resize(800, 500);
    window.setInitialProperties({{"keys", m_keys}});
    window.setSource(QUrl("qrc:/qml/api/ui/plot.qml"));
}
