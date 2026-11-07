#ifndef UNICOMM_UI_DATABASE_H
#define UNICOMM_UI_DATABASE_H

#include <QHash>
#include <QString>
#include <sol/table.hpp>

#include "api/ui/window.h"

class QStandardItemModel;

class UiDatabase final : public UiWindow {
public:
    UiDatabase(const sol::optional<std::string> &title, const sol::table &keys, Ui *owner);

    void set(const std::string &key, const sol::object &value) const;

    void close();

protected:
    void init(QQuickView &window) override;

private:
    QHash<QString, int> m_keys{};
    QStandardItemModel *m_model{}; // Owned by the window.
};

#endif //UNICOMM_UI_DATABASE_H
