#ifndef UNICOMM_UI_H
#define UNICOMM_UI_H

#include <QObject>
#include <sol/table.hpp>

#include <string>

class QQuickView;

class Ui final : public QObject {
    Q_OBJECT

public:
    explicit Ui(QObject *parent = nullptr);

    [[nodiscard]] bool message(const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button) const;

    [[nodiscard]] sol::optional<std::string> input(const sol::optional<std::string> &title, const std::string &text, const sol::optional<std::string> &value) const;

    [[nodiscard]] sol::optional<std::string> file(const sol::optional<std::string> &title, const sol::optional<std::string> &mode) const;

signals:
    void close() const;

private:
    friend class UiImage;

    [[nodiscard]] QQuickView *init(const sol::optional<std::string> &title) const;
};

#endif //UNICOMM_UI_H
