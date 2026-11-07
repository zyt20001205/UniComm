#ifndef UNICOMM_UI_TABLE_H
#define UNICOMM_UI_TABLE_H

#include <QHash>
#include <QList>
#include <QStandardItemModel>
#include <QString>
#include <QUrl>
#include <sol/table.hpp>

#include "api/ui/window.h"

class UiTableModel;

class UiTable final : public UiWindow {
public:
    UiTable(const sol::optional<std::string> &title, const sol::table &keys, Ui *owner);

    void append(const sol::table &values);

    void _export(const std::string &path) const;

    void close();

protected:
    void init(QQuickView &window) override;

private:
    QHash<QString, int> m_keys{};
    QList<int> m_nextRows{};
    QPointer<UiTableModel> m_model{};
};

class UiTableModel final : public QStandardItemModel {
    Q_OBJECT

public:
    using QStandardItemModel::QStandardItemModel;

    Q_INVOKABLE [[nodiscard]] QString _export(const QUrl &url) const;
};

#endif //UNICOMM_UI_TABLE_H
