#include "api/ui/ui.h"

#include <QFileDialog>
#include <QThread>
#include <sol/error.hpp>

#include "globals.h"
#include "api/ui/database.h"
#include "api/ui/image.h"
#include "api/ui/input.h"
#include "api/ui/message.h"

// public
Ui::Ui(QObject *parent)
    : QObject(parent) {
}

bool Ui::message(const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button) {
    UiMessage message(title, text, button, this);
    message.open(true);
    return message.result();
}

sol::optional<std::string> Ui::input(const sol::optional<std::string> &title, const std::string &text, const sol::optional<std::string> &value) {
    UiInput input(title, text, value, this);
    input.open(true);
    return input.result();
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

        connect(this, &Ui::exit, dialog, &QDialog::reject);
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

std::unique_ptr<UiImage> Ui::image(const sol::optional<std::string> &title, const sol::optional<std::string> &source) {
    auto image = std::make_unique<UiImage>(title, source, this);
    image->open(false);
    return image;
}

std::unique_ptr<UiDatabase> Ui::database(const sol::optional<std::string> &title, const sol::table &keys) {
    auto database = std::make_unique<UiDatabase>(title, keys, this);
    database->open(false);
    return database;
}

void Ui::windowOpened() {
    ++m_windowCount;
}

void Ui::windowDestroyed() {
    if (--m_windowCount == 0) emit close();
}

void Ui::wait() const {
    if (m_windowCount == 0 || QThread::currentThread()->isInterruptionRequested()) return;
    QEventLoop loop{};
    connect(this, &Ui::close, &loop, &QEventLoop::quit);
    loop.exec();
}
