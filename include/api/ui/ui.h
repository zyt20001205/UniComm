#ifndef UNICOMM_UI_H
#define UNICOMM_UI_H

#include <QObject>
#include <sol/table.hpp>

#include <memory>
#include <string>

class UiDatabase;
class UiImage;

class Ui final : public QObject {
    Q_OBJECT

public:
    explicit Ui(QObject *parent = nullptr);

    [[nodiscard]] bool message(const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button);

    [[nodiscard]] sol::optional<std::string> input(const sol::optional<std::string> &title, const std::string &text, const sol::optional<std::string> &value);

    [[nodiscard]] sol::optional<std::string> file(const sol::optional<std::string> &title, const sol::optional<std::string> &mode) const;

    [[nodiscard]] std::unique_ptr<UiImage> image(const sol::optional<std::string> &title, const sol::optional<std::string> &source);

    [[nodiscard]] std::unique_ptr<UiDatabase> database(const sol::optional<std::string> &title, const sol::table &keys);

    void windowOpened();

    void windowDestroyed();

    void wait() const;

signals:
    void exit() const;

    void close() const;

private:
    int m_windowCount = 0;
};

#endif //UNICOMM_UI_H
