#include "core/fileModule.h"

#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QQmlComponent>

#include "globals.h"
#include "core/globalManager.h"
#include "document/documentModule.h"
#include "mainWindow/toastModule.h"
#include "util/uniCast.h"

// public
FileModule::FileModule(QObject *parent)
    : QObject(parent) {
}

void FileModule::propertySet() {
    QQmlComponent component(g_globalManager->engineGet(), QUrl("qrc:/qml/core/fileModule.qml"));
    if (auto *dialog = component.create()) dialog->setParent(this);
    else qWarning().noquote() << component.errorString();
}

void FileModule::fileOpenInExplorer(const QUrl &fileUrl) {
    const QString filePath = fileUrl.toLocalFile();
    const QFileInfo fileInfo(filePath);
    QStringList args;
#ifdef Q_OS_WIN
    const QString command = "explorer.exe";
    if (fileInfo.isFile()) {
        args << "/select," << QDir::toNativeSeparators(filePath);
    } else if (fileInfo.isDir()) {
        args << QDir::toNativeSeparators(filePath);
    }
#endif
    auto *process = new QProcess(this); // NOLINT
    connect(process, &QProcess::started, this, [this, process] {
        int taskId = -1;
        emit appendBackground(
            taskId,
            [process] { if (process->state() != QProcess::NotRunning) process->terminate(); },
            {});
        process->setProperty("taskId", taskId);
        emit refreshBackground(taskId, tr("Waiting for explorer..."));
    });
    connect(process, &QProcess::finished, this, [this, process] {
        emit removeBackground(process->property("taskId").toInt());
        process->deleteLater();
    });
    connect(process, &QProcess::errorOccurred, this, [process](const QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) process->deleteLater();
    });
    process->start(command, args);
}

void FileModule::fileOpenInApplication(const QUrl &fileUrl) {
    const QString filePath = fileUrl.toLocalFile();
    QStringList args;
#ifdef Q_OS_WIN
    const QString command = "explorer.exe";
    args << QDir::toNativeSeparators(filePath);
#endif
    auto *process = new QProcess(this); // NOLINT
    connect(process, &QProcess::started, this, [this, process] {
        int taskId = -1;
        emit appendBackground(
            taskId,
            [process] { if (process->state() != QProcess::NotRunning) process->terminate(); },
            {});
        process->setProperty("taskId", taskId);
        emit refreshBackground(taskId, tr("Waiting for application..."));
    });
    connect(process, &QProcess::finished, this, [this, process] {
        emit removeBackground(process->property("taskId").toInt());
        process->deleteLater();
    });
    connect(process, &QProcess::errorOccurred, this, [process](const QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) process->deleteLater();
    });
    process->start(command, args);
}

QVariantHash FileModule::fileInfo(const QUrl &fileUrl) {
    const auto &filePath = fileUrl.toLocalFile();
    const QFileInfo fileInfo(filePath);
    const auto &source = uni_cast<QFileIcon>(fileUrl);
    const QLocale locale{QLocale::C};
    QVariantHash infoSession = {
        {"source", source.value},
        {"baseName", fileInfo.baseName()},
        {"parentUrl", QUrl::fromLocalFile(fileInfo.absolutePath()).toString()},
        {"suffix", fileInfo.suffix()},
        {"absolutePath", fileInfo.absoluteFilePath()},
        {"size", locale.formattedDataSize(fileInfo.size())},
        {"birthTime", fileInfo.birthTime().toString("yyyy-MM-dd HH:mm:ss")},
        {"lastModified", fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss")},
        {"lastRead", fileInfo.lastRead().toString("yyyy-MM-dd HH:mm:ss")},
        {"readable", fileInfo.isReadable()},
        {"writable", fileInfo.isWritable()},
        {"hidden", fileInfo.isHidden()},
        {"directory", fileInfo.isDir()},
    };
    return infoSession;
}

bool FileModule::fileRename(const QUrl &sourceUrl, const QUrl &targetUrl) {
    auto *document = g_globalManager->documentModuleGet();
    const auto error = QFileInfo(sourceUrl.toLocalFile()).isDir()
                           ? document->directoryRename(sourceUrl, targetUrl)
                           : document->documentRename(sourceUrl, targetUrl);
    if (error.isEmpty()) return true;
    g_globalManager->toastModuleGet()->show(ToastLevel::Error, tr("Rename failed"), error);
    return false;
}

void FileModule::fileWritable(const QUrl &fileUrl, const bool status) {
    const auto filePath = fileUrl.toLocalFile();
    const QFileInfo fileInfo(filePath);
    auto permissions = fileInfo.permissions();
    if (status) permissions |= QFileDevice::WriteOwner | QFileDevice::WriteUser | QFileDevice::WriteGroup | QFileDevice::WriteOther;
    else permissions &= ~(QFileDevice::WriteOwner | QFileDevice::WriteUser | QFileDevice::WriteGroup | QFileDevice::WriteOther);
    QFile::setPermissions(filePath, permissions);
    emit setPermission(fileUrl);
}

void FileModule::copyToClipboard(const QString &text) const {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
    g_globalManager->toastModuleGet()->show(ToastLevel::Success, tr("Copied to clipboard"));
}

int FileModule::lineCountGet(const QUrl &documentUrl) {
    QFile file(documentUrl.toLocalFile());
    if (!QFileInfo(file).isFile() || !file.open(QIODevice::ReadOnly)) return -1;

    QTextStream in(&file);
    auto text = in.readAll();
    if (file.error() != QFileDevice::NoError) return -1;
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    return static_cast<int>(text.count('\n') + 1);
}

QString FileModule::linesGet(const QUrl &documentUrl, const int startLine, const int lineCount) {
    QFile file(documentUrl.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QTextStream in(&file);
    QStringList lines{};
    for (int line = 0; !in.atEnd(); ++line) {
        const auto text = in.readLine();
        if (line < startLine) continue;
        lines.append(text);
        if (lines.size() == lineCount) break;
    }
    return lines.join('\n');
}

QString FileModule::textGet(const QUrl &documentUrl, const int startLine, const int startCharacter, const int endLine, const int endCharacter) {
    // TODO: character not implemented
    QFile file(documentUrl.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream in(&file);
    if (startLine == -1) return in.readAll();
    QStringList lines{};
    while (!in.atEnd()) {
        const auto line = in.readLine();
        lines.append(line);
    }
    if (startLine < 0 || startLine >= lines.size() || endLine < startLine) return {};
    return lines.mid(startLine, qMin(endLine, static_cast<int>(lines.size() - 1)) - startLine + 1).join('\n');
}
