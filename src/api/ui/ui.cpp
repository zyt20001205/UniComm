#include "api/ui/ui.h"

#include <QFileDialog>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QThread>
#include <sol/error.hpp>

#include "globals.h"
#include "core/globalManager.h"

// public
Ui::Ui(QObject *parent)
    : QObject(parent) {
}

bool Ui::message(const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button) const {
    QEventLoop loop{};
    QString exception{};
    bool accepted = false;
    auto *worker = QThread::currentThread();
    QStringList buttons{"ok"};
    if (button.has_value()) {
        buttons.clear();
        for (const auto &value: button.value() | std::views::values) {
            const auto name = QString::fromStdString(value.as<std::string>());
            if (name != "ok" && name != "cancel") throw sol::error("button must be ok or cancel");
            buttons.append(name);
        }
    }
    if (buttons.isEmpty()) throw sol::error("button must not be empty");

    QMetaObject::invokeMethod(qApp, [&accepted, &loop, this, &title, &text, &buttons, worker]() -> QString {
        if (worker->isInterruptionRequested()) return "terminated";

        auto *window = init(title);
        window->setFlags(Qt::Dialog);
        window->setInitialProperties({{"text", QString::fromStdString(text)}, {"button", buttons}});
        window->setSource(QUrl("qrc:/qml/api/ui/message.qml"));
        if (window->status() == QQuickView::Error) {
            const auto error = window->errors().first().toString();
            delete window;
            return error;
        }

        connect(window, &QQuickWindow::closing, window, [&accepted, window] {
            accepted = window->rootObject()->property("accepted").toBool();
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

sol::optional<std::string> Ui::input(const sol::optional<std::string> &title, const std::string &text, const sol::optional<std::string> &value) const {
    QEventLoop loop{};
    QString exception{};
    sol::optional<std::string> result{};
    auto *worker = QThread::currentThread();

    QMetaObject::invokeMethod(qApp, [&result, &loop, this, &title, &text, &value, worker]() -> QString {
        if (worker->isInterruptionRequested()) return "terminated";

        auto *window = init(title);
        window->setFlags(Qt::Dialog);
        window->setInitialProperties({{"text", QString::fromStdString(text)}, {"value", QString::fromStdString(value.value_or(""))}});
        window->setSource(QUrl("qrc:/qml/api/ui/input.qml"));
        if (window->status() == QQuickView::Error) {
            const auto error = window->errors().first().toString();
            delete window;
            return error;
        }

        connect(window, &QQuickWindow::closing, window, [&result, window] {
            const auto *root = window->rootObject();
            if (root->property("accepted").toBool()) result = root->property("value").toString().toStdString();
        });
        // Keep an early close queued until the worker enters its event loop.
        connect(window, &QObject::destroyed, &loop, &QEventLoop::quit, Qt::QueuedConnection);
        window->show();
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());

    loop.exec();
    if (worker->isInterruptionRequested()) throw sol::error("terminated");
    return result;
}

sol::optional<std::string> Ui::file(const sol::optional<std::string> &title, const sol::optional<std::string> &mode) const {
    const auto _mode = mode.value_or("open");
    if (_mode != "open" && _mode != "save") throw sol::error("mode must be open or save");

    QEventLoop loop{};
    QString exception{};
    sol::optional<std::string> result{};
    auto *worker = QThread::currentThread();

    QMetaObject::invokeMethod(qApp, [&result, &loop, this, &title, &_mode, worker]() -> QString {
        if (worker->isInterruptionRequested()) return "terminated";

        const auto _title = title.has_value() ? QString::fromStdString(title.value()) : QString("0x%1").arg(reinterpret_cast<quintptr>(thread()), 0, 16);
        auto *dialog = new QFileDialog(g_mainWindow, _title, g_workspaceUrl.toLocalFile()); // NOLINT
        if (_mode == "save") {
            dialog->setAcceptMode(QFileDialog::AcceptSave);
            dialog->setFileMode(QFileDialog::AnyFile);
        } else {
            dialog->setAcceptMode(QFileDialog::AcceptOpen);
            dialog->setFileMode(QFileDialog::ExistingFile);
        }

        connect(this, &Ui::close, dialog, &QDialog::reject);
        connect(this, &QObject::destroyed, dialog, &QObject::deleteLater);
        connect(dialog, &QDialog::accepted, dialog, [&result, dialog] {
            result = dialog->selectedFiles().first().toStdString();
        });
        connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
        // Keep an early close queued until the worker enters its event loop.
        connect(dialog, &QObject::destroyed, &loop, &QEventLoop::quit, Qt::QueuedConnection);
        dialog->open();
        return {};
    }, Qt::BlockingQueuedConnection, &exception);
    if (!exception.isEmpty()) throw sol::error(exception.toStdString());

    loop.exec();
    if (worker->isInterruptionRequested()) throw sol::error("terminated");
    return result;
}

// private
QQuickView *Ui::init(const sol::optional<std::string> &title) const {
    auto *window = new QQuickView(); // NOLINT
    const auto _title = title.has_value() ? QString::fromStdString(title.value()) : QString("0x%1").arg(reinterpret_cast<quintptr>(thread()), 0, 16);
    window->setTitle(_title);
    window->setTransientParent(g_mainWindow->windowHandle());
    window->rootContext()->setContextProperty("global", g_globalManager);
    window->setResizeMode(QQuickView::SizeRootObjectToView);
    connect(this, &Ui::close, window, &QWindow::close);
    connect(this, &QObject::destroyed, window, &QObject::deleteLater);
    connect(window, &QQuickWindow::closing, window, &QObject::deleteLater);
    return window;
}
