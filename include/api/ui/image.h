#ifndef UNICOMM_UI_IMAGE_H
#define UNICOMM_UI_IMAGE_H

#include <QUrl>
#include <sol/optional.hpp>

#include <string>

#include "api/ui/window.h"

class UiImage final : public UiWindow {
public:
    UiImage(const sol::optional<std::string> &title, const sol::optional<std::string> &source, Ui *owner);

    void setData(const std::string &data) const;

    void setPath(const std::string &path) const;

    void close();

protected:
    void init(QQuickView &window) override;

private:
    class DataProvider;

    QUrl m_source{};
    DataProvider *m_provider{}; // Owned by the window's QML engine.
};

#endif //UNICOMM_UI_IMAGE_H
