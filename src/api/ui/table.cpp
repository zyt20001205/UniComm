#include "api/ui/table.h"

#include <QApplication>
#include <QQuickView>
#include <QSaveFile>
#include <QTextStream>
#include <sol/error.hpp>

#include <vector>

#include "util/uniCast.h"

// public
UiTable::UiTable(const sol::optional<std::string> &title, const sol::table &keys, Ui *owner)
    : UiWindow(title, owner) {
    const auto names = keys.as<sol::as_table_t<std::vector<std::string> > >();
    for (const auto &key: names) {
        const auto name = QString::fromStdString(key);
        if (m_keys.contains(name)) throw sol::error("duplicate table column: " + key);
        m_keys.insert(name, static_cast<int>(m_keys.size()));
    }
}

void UiTable::append(const sol::table &values) {
    QList<QPair<int, QString>> cells{};
    for (const auto &[key, value]: values) {
        if (key.get_type() != sol::type::string) throw sol::error("table column names must be strings");
        const auto name = key.as<std::string>();
        const int column = m_keys.value(QString::fromStdString(name), -1);
        if (column == -1) throw sol::error("unknown table column: " + name);
        cells.emplaceBack(column, uni_cast<QString>(value));
    }

    QString exception{};
    QMetaObject::invokeMethod(qApp, [this, &cells]() -> QString {
        if (!m_window || !m_window->isVisible()) return "table window is closed";
        for (const auto &[column, value]: cells) {
            m_model->setItem(m_nextRows[column]++, column, new QStandardItem(value)); // NOLINT
        }
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());
}

void UiTable::_export(const std::string &path) const {
    const auto url = uni_cast<QUrl>(LPath(QString::fromStdString(path)));
    QString exception{};
    QMetaObject::invokeMethod(qApp, [this, &url]() -> QString {
        if (!m_window || !m_window->isVisible()) return "table window is closed";
        return m_model->_export(url);
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());
}

void UiTable::close() {
    QMetaObject::invokeMethod(qApp, [this] {
        if (!m_window) return;
        m_window->close();
        m_window.clear();
        m_model.clear();
    }, Qt::BlockingQueuedConnection);
}

// protected
void UiTable::init(QQuickView &window) {
    QStringList keys(m_keys.size(), QString{});
    for (auto it = m_keys.cbegin(); it != m_keys.cend(); ++it) keys[it.value()] = it.key();
    m_nextRows.fill(0, keys.size());
    m_model = new UiTableModel(0, static_cast<int>(keys.size()), &window); // NOLINT
    m_model->setHorizontalHeaderLabels(keys);
    window.resize(720, 440);
    window.setInitialProperties({{"dataModel", QVariant::fromValue(m_model.data())}});
    window.setSource(QUrl("qrc:/qml/api/ui/table.qml"));
}

// public
QString UiTableModel::_export(const QUrl &url) const {
    QSaveFile file(url.toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) return file.errorString();

    QTextStream out(&file);
    for (int row = -1; row < rowCount(); ++row) {
        for (int column = 0; column < columnCount(); ++column) {
            if (column != 0) out << ',';
            auto field = row < 0 ? headerData(column, Qt::Horizontal).toString() : index(row, column).data().toString();
            if (field.contains(',') || field.contains('"') || field.contains('\n') || field.contains('\r')) {
                out << '"' << field.replace('"', "\"\"") << '"';
            } else {
                out << field;
            }
        }
        out << "\r\n";
    }
    out.flush();
    if (out.status() != QTextStream::Ok || !file.commit()) return file.errorString();
    return {};
}
