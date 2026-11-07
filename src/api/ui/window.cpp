#include "api/ui/window.h"

#include <QQmlContext>
#include <QQuickView>
#include <QThread>
#include <sol/error.hpp>

#include "globals.h"
#include "api/ui/ui.h"
#include "core/globalManager.h"

// protected
UiWindow::UiWindow(const sol::optional<std::string> &title, Ui *owner)
    : m_title(title),
      m_owner(owner) {
}

// public
void UiWindow::open(const bool blocking) {
    QEventLoop loop{};
    auto *worker = QThread::currentThread();
    QString exception{"failed to open window"};

    QMetaObject::invokeMethod(qApp, [&loop, this, worker, blocking]() -> QString {
        if (worker->isInterruptionRequested()) return "terminated";

        auto *window = new QQuickView(); // NOLINT
        const auto title = m_title.has_value() ? QString::fromStdString(m_title.value()) : QString("0x%1").arg(reinterpret_cast<quintptr>(m_owner->thread()), 0, 16);
        window->setTitle(title);
        window->setTransientParent(g_mainWindow->windowHandle());
        window->rootContext()->setContextProperty("global", g_globalManager);
        window->setResizeMode(QQuickView::SizeRootObjectToView);
        QObject::connect(m_owner, &Ui::exit, window, &QWindow::close);
        QObject::connect(m_owner, &QObject::destroyed, window, &QObject::deleteLater);
        QObject::connect(window, &QQuickWindow::closing, window, &QObject::deleteLater);

        init(*window);
        if (window->status() == QQuickView::Error) {
            const auto error = window->errors().first().toString();
            delete window;
            return error;
        }

        m_window = window;
        QObject::connect(window, &QObject::destroyed, m_owner, &Ui::windowDestroyed, Qt::QueuedConnection);
        if (blocking) QObject::connect(window, &QObject::destroyed, &loop, &QEventLoop::quit, Qt::QueuedConnection);
        window->show();
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());
    m_owner->windowOpened();

    if (!blocking) return;
    loop.exec();
    if (worker->isInterruptionRequested()) throw sol::error("terminated");
}
