#ifndef UNICOMM_UI_H
#define UNICOMM_UI_H

#include <QObject>
#include <sol/table.hpp>

#include <string>

class UI final : public QObject {
    Q_OBJECT

public:
    explicit UI(QObject *parent = nullptr);

    [[nodiscard]] bool message(const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button) const;

signals:
    void close() const;
};

#endif //UNICOMM_UI_H
