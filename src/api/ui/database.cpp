#include "api/ui/database.h"

#include <QApplication>
#include <QQuickView>
#include <QStandardItemModel>
#include <sol/error.hpp>

#include <vector>

#include "util/uniCast.h"

// public
UiDatabase::UiDatabase(const sol::optional<std::string> &title, const sol::table &keys, Ui *owner)
    : UiWindow(title, owner) {
    const auto names = keys.as<sol::as_table_t<std::vector<std::string> > >();
    for (const auto &key: names) {
        const auto name = QString::fromStdString(key);
        if (m_keys.contains(name)) throw sol::error("duplicate database key: " + key);
        m_keys.insert(name, static_cast<int>(m_keys.size()));
    }
}

void UiDatabase::set(const std::string &key, const sol::object &value) const {
    const int row = m_keys.value(QString::fromStdString(key), -1);
    if (row == -1) throw sol::error("unknown database key: " + key);
    const auto text = uni_cast<QString>(value);

    QString exception{};
    QMetaObject::invokeMethod(qApp, [this, row, &text]() -> QString {
        if (!m_window || !m_window->isVisible()) return "database window is closed";
        m_model->item(row, 1)->setText(text);
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());
}

void UiDatabase::close() {
    QMetaObject::invokeMethod(qApp, [this] {
        if (!m_window) return;
        m_window->close();
        m_window.clear();
        m_model = nullptr;
    }, Qt::BlockingQueuedConnection);
}

// private
void UiDatabase::init(QQuickView &window) {
    m_model = new QStandardItemModel(static_cast<int>(m_keys.size()), 2, &window); // NOLINT
    for (auto it = m_keys.cbegin(); it != m_keys.cend(); ++it) {
        m_model->setItem(it.value(), 0, new QStandardItem(it.key())); // NOLINT
        m_model->setItem(it.value(), 1, new QStandardItem(QString{})); // NOLINT
    }
    window.resize(360, 560);
    window.setInitialProperties({{"dataModel", QVariant::fromValue(m_model)}});
    window.setSource(QUrl("qrc:/qml/api/ui/database.qml"));
}
