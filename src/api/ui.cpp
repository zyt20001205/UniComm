#include "api/ui.h"

#include <QQuickItem>
#include <QQuickView>
#include <QThread>
#include <sol/error.hpp>

#include "globals.h"

// public
UI::UI(QObject *parent)
    : QObject(parent) {
}

bool UI::message(const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button) const {
    QEventLoop loop{};
    QString exception{};
    bool accepted = false;
    auto *worker = QThread::currentThread();
    const auto _title = title.has_value() ? QString::fromStdString(title.value()) : QString("0x%1").arg(reinterpret_cast<quintptr>(worker), 0, 16);
    QStringList buttons{"ok"};
    if (button.has_value()) {
        buttons.clear();
        for (const auto &[key, value]: button.value()) {
            const auto name = QString::fromStdString(value.as<std::string>());
            if (name != "ok" && name != "cancel") throw sol::error("button must be ok or cancel");
            buttons.append(name);
        }
    }
    if (buttons.isEmpty()) throw sol::error("button must not be empty");

    QMetaObject::invokeMethod(qApp, [&accepted, &loop, this, &_title, &text, &buttons, worker]() -> QString {
        if (worker->isInterruptionRequested()) return "terminated";

        auto *window = new QQuickView(); // NOLINT
        window->setTitle(_title);
        window->setFlags(Qt::Dialog);
        window->setTransientParent(g_mainWindow->windowHandle());
        window->setResizeMode(QQuickView::SizeRootObjectToView);
        window->setInitialProperties({{"text", QString::fromStdString(text)}, {"button", buttons}});
        window->setSource(QUrl("qrc:/qml/api/ui/message.qml"));
        if (window->status() == QQuickView::Error) {
            const auto error = window->errors().first().toString();
            delete window;
            return error;
        }

        connect(this, &UI::close, window, &QWindow::close);
        connect(this, &QObject::destroyed, window, &QObject::deleteLater);
        connect(window, &QQuickWindow::closing, window, [&accepted, window] {
            accepted = window->rootObject()->property("accepted").toBool();
            window->deleteLater();
        });
        // Keep an early close queued until the worker enters its event loop.
        connect(window, &QObject::destroyed, &loop, &QEventLoop::quit, Qt::QueuedConnection);
        window->show();
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());

    loop.exec();
    if (worker->isInterruptionRequested()) throw sol::error("terminated");
    return accepted;
}
