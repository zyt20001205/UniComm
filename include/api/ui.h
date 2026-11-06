#ifndef UNICOMM_UI_H
#define UNICOMM_UI_H

#include <QObject>
#include <QPointer>
#include <sol/table.hpp>

#include <string>

class QQuickView;

class UI final : public QObject {
    Q_OBJECT

public:
    explicit UI(QObject *parent = nullptr);

    [[nodiscard]] bool message(const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button) const;

    [[nodiscard]] sol::optional<std::string> input(const sol::optional<std::string> &title, const std::string &text, const sol::optional<std::string> &value) const;

    [[nodiscard]] sol::optional<std::string> file(const sol::optional<std::string> &title, const sol::optional<std::string> &mode) const;

signals:
    void close() const;

private:
    friend class ImageViewer;

    [[nodiscard]] QQuickView *init(const sol::optional<std::string> &title) const;
};

class ImageViewer final {
public:
    ImageViewer(const sol::optional<std::string> &title, const sol::optional<std::string> &source, const UI *owner);

    ~ImageViewer();

    void setData(const std::string &data) const;

    void setPath(const std::string &path) const;

    void close();

private:
    Q_DISABLE_COPY(ImageViewer)

    class DataProvider;

    QPointer<QQuickView> m_window{};
    DataProvider *m_provider{}; // Owned by the window's QML engine.
};

#endif //UNICOMM_UI_H
