#include "api/ui/image.h"

#include <QApplication>
#include <QMutex>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickView>
#include <QThread>
#include <sol/error.hpp>

#include "api/ui/ui.h"
#include "util/uniCast.h"

// public
class UiImage::DataProvider final : public QQuickImageProvider {
public:
    DataProvider() : QQuickImageProvider(Image) {
    }

    QImage requestImage(const QString &, QSize *size, const QSize &) override {
        const QMutexLocker locker(&mutex);
        if (size) *size = image.size();
        return image;
    }

    QMutex mutex{};
    QImage image{};
};

// public
UiImage::UiImage(const sol::optional<std::string> &title, const sol::optional<std::string> &source, const Ui *owner) {
    auto *worker = QThread::currentThread();
    const auto url = source.has_value() ? uni_cast<QUrl>(LPath(QString::fromStdString(source.value()))) : QUrl{};
    QString exception{};

    QMetaObject::invokeMethod(qApp, [this, &title, &url, owner, worker]() -> QString {
        if (worker->isInterruptionRequested()) return "terminated";

        auto *window = owner->init(title);
        auto *provider = new DataProvider(); // NOLINT
        window->engine()->addImageProvider("memory", provider);
        window->resize(800, 600);
        window->setInitialProperties({{"source", url}, {"cache", false}});
        window->setSource(QUrl("qrc:/qml/util/ImageViewer.qml"));
        if (window->status() == QQuickView::Error) {
            const auto error = window->errors().first().toString();
            delete window;
            return error;
        }

        m_window = window;
        m_provider = provider;
        window->show();
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());
}

UiImage::~UiImage() {
    close();
}

void UiImage::setData(const std::string &data) const {
    const auto image = QImage::fromData(QByteArray::fromStdString(data));
    if (image.isNull()) throw sol::error("invalid or unsupported image data");

    QString exception{};
    QMetaObject::invokeMethod(qApp, [this, &image]() -> QString {
        if (!m_window || !m_window->isVisible()) return "image window is closed";

        {
            const QMutexLocker locker(&m_provider->mutex);
            m_provider->image = image;
        }
        // QML may request the image synchronously, so release the lock first.
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
