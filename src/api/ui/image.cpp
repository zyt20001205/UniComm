#include "api/ui/image.h"

#include <QApplication>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickView>
#include <sol/error.hpp>

#include "util/uniCast.h"

// public
class UiImage::DataProvider final : public QQuickImageProvider {
public:
    DataProvider() : QQuickImageProvider(Image) {
    }

    QImage requestImage(const QString &, QSize *size, const QSize &) override {
        if (size) *size = image.size();
        return image;
    }

    QImage image{};
};

// public
UiImage::UiImage(const sol::optional<std::string> &title, const sol::optional<std::string> &source, Ui *owner)
    : UiWindow(title, owner),
      m_source(source.has_value() ? uni_cast<QUrl>(LPath(QString::fromStdString(source.value()))) : QUrl{}) {
}

void UiImage::setData(const std::string &data) const {
    const auto image = QImage::fromData(QByteArray::fromStdString(data));
    if (image.isNull()) throw sol::error("invalid or unsupported image data");

    QString exception{};
    QMetaObject::invokeMethod(qApp, [this, &image]() -> QString {
        if (!m_window || !m_window->isVisible()) return "image window is closed";

        m_provider->image = image;
        auto *root = m_window->rootObject();
        root->setProperty("source", QUrl{});
        root->setProperty("source", QUrl("image://memory/frame"));
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());
}

void UiImage::setPath(const std::string &path) const {
    const auto url = uni_cast<QUrl>(LPath(QString::fromStdString(path)));
    QString exception{};
    QMetaObject::invokeMethod(qApp, [this, &url]() -> QString {
        if (!m_window || !m_window->isVisible()) return "image window is closed";

        auto *root = m_window->rootObject();
        root->setProperty("source", QUrl{});
        root->setProperty("source", url);
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());
}

void UiImage::close() {
    QMetaObject::invokeMethod(qApp, [this] {
        if (!m_window) return;
        m_window->close();
        m_window.clear();
        m_provider = nullptr;
    }, Qt::BlockingQueuedConnection);
}

// private
void UiImage::init(QQuickView &window) {
    m_provider = new DataProvider(); // NOLINT
    window.engine()->addImageProvider("memory", m_provider);
    window.resize(800, 600);
    window.setInitialProperties({{"source", m_source}, {"cache", false}});
    window.setSource(QUrl("qrc:/qml/util/ImageViewer.qml"));
}
