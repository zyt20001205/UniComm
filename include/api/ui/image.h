#ifndef UNICOMM_UI_IMAGE_H
#define UNICOMM_UI_IMAGE_H

#include <QPointer>
#include <sol/optional.hpp>

#include <string>

class QQuickView;
class Ui;

class UiImage final {
public:
    UiImage(const sol::optional<std::string> &title, const sol::optional<std::string> &source, const Ui *owner);

    ~UiImage();

    void setData(const std::string &data) const;

    void setPath(const std::string &path) const;

    void close();

private:
    Q_DISABLE_COPY(UiImage)

    class DataProvider;

    QPointer<QQuickView> m_window{};
    DataProvider *m_provider{}; // Owned by the window's QML engine.
};

#endif //UNICOMM_UI_IMAGE_H
