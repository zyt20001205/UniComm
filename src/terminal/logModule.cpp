#include "terminal/logModule.h"

#include <QFileDialog>
#include <QSaveFile>
#include <QJsonArray>
#include <QPrinter>
#include <QQmlContext>
#include <QQuickTextDocument>
#include <QQuickWidget>
#include <QStandardPaths>
#include <QTextBlock>

#include "globals.h"
#include "core/globalManager.h"
#include "mainWindow/toastModule.h"

// public
LogModule::LogModule()
    : DockWidget("Log"),
      m_config(g_workspaceConfig["logConfig"].toObject()),
      m_widget(new QQuickWidget()),
      m_textDocument(new QTextDocument(this)),
      m_errorFore(g_globalManager->themeGet()["general"].toObject()["dangerFore3"].toString()),
      m_warningFore(g_globalManager->themeGet()["general"].toObject()["warningFore3"].toString()),
      m_infoFore(g_globalManager->themeGet()["general"].toObject()["fore"].toString()) {
    setWidget(m_widget);
}

LogModule::~LogModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void LogModule::propertySet(const QVariantHash &objects) {
    m_textView = qvariant_cast<QObject *>(objects["mainWindowTextView"]);
    m_toast = qvariant_cast<ToastModule *>(objects["mainWindowToast"]);
    const auto font = QFont(m_config["fontFamily"].toString(), m_config["fontSize"].toInt());
    m_textView->setProperty("font", font);

    m_widget->rootContext()->setContextProperty("logModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("mainLinkMenu", qvariant_cast<QObject *>(objects["mainWindowLinkMenu"]));
    m_widget->rootContext()->setContextProperty("mainToolTip", qvariant_cast<QObject *>(objects["mainWindowToolTip"]));
    m_widget->rootContext()->setContextProperty("heightDialog", qvariant_cast<QObject *>(objects["logModuleHeightDialog"]));

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/terminal/logModule.qml"));
}

void LogModule::propertyGet(const QVariantMap &objects) {
    // set timestamp
    auto *timestampButton = qvariant_cast<QObject *>(objects["timestampButton"]);
    timestampButton->setProperty("checked", m_config["timestamp"].toBool());
    // set wrap
    auto *wrapButton = qvariant_cast<QObject *>(objects["wrapButton"]);
    wrapButton->setProperty("checked", m_config["wrap"].toBool());
    // set font
    m_textArea = qvariant_cast<QObject *>(objects["textArea"]);
    const auto font = QFont(m_config["fontFamily"].toString(), m_config["fontSize"].toInt());
    m_textArea->setProperty("font", font);
    // set height
    const auto *quickTextDocument = qvariant_cast<QQuickTextDocument *>(m_textArea->property("textDocument"));
    m_textDocument = quickTextDocument->textDocument();
    m_textDocument->setMaximumBlockCount(m_config["height"].toInt());
}

void LogModule::logConfigSave() const {
    g_workspaceConfig["logConfig"] = m_config;
}

void LogModule::logFontReload(const QJsonObject &fontConfigLog) const {
    const auto logFont = QFont(fontConfigLog["fontFamily"].toString(), fontConfigLog["fontSize"].toInt());
    m_textArea->setProperty("font", logFont);
}

void LogModule::logFontSave(const QJsonObject &fontConfigLog) {
    m_config["fontFamily"] = fontConfigLog["fontFamily"].toString();
    m_config["fontSize"] = fontConfigLog["fontSize"].toInt();
}

QJsonArray LogModule::logGet(const int blockCount) const {
    int count{};
    // limit range
    if (blockCount == -1 || blockCount > 20) count = 20;
    // limit blockCount to available
    count = qMin(count, m_textDocument->blockCount());
    // pack to json array
    QJsonArray array{};
    for (int i = m_textDocument->blockCount() - count; i < m_textDocument->blockCount(); ++i) {
        const auto block = m_textDocument->findBlockByNumber(i);
        array.append(block.text());
    }
    return array;
}

void LogModule::logAppend(const int type, const QString &prefix, const QString &message) {
    auto _message = message;
    // check size
    const auto size = _message.size();
    if (size >= 200) {
        _message = QString::fromLatin1(
            _message.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
        _message = QString("[<a href='request.expand://reserved/%1'>%2 chars collapsed</a>]").arg(
            _message, QString::number(size));
    }
    // check level
    switch (type) {
        case LogLevel::Error: {
            _message = QString("<span style='color:%1'>%2 %3</span>").arg(m_errorFore, prefix, _message);
        }
        break;
        case LogLevel::Warning: {
            _message = QString("<span style='color:%1'>%2 %3</span>").arg(m_warningFore, prefix, _message);
        }
        break;
        case LogLevel::Info: {
            _message = QString("<span style='color:%1'>%2 %3</span>").arg(m_infoFore, prefix, _message);
        }
        break;
        case LogLevel::Transmit: {
            _message = QString("%1 %2").arg(prefix, _message);
        }
        break;
        case LogLevel::Receive: {
            _message = QString("%1 %2").arg(prefix, _message);
        }
        break;
        default: break;
    }
    // check timestamp
    if (m_config["timestamp"].toBool()) {
        const auto timestamp = QString("[%1] ").arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"));
        _message = timestamp + _message;
    }
    QMetaObject::invokeMethod(m_textArea, "append", Q_ARG(QString, _message));
}

void LogModule::timestampToggle(const bool status) {
    m_config["timestamp"] = status;
}

void LogModule::wrapToggle(const bool status) {
    m_config["wrap"] = status;
}

int LogModule::heightGet() {
    return m_config["height"].toInt();
}

void LogModule::heightSet(const QString &height) {
    m_config["height"] = height.toInt();
    m_textDocument->setMaximumBlockCount(height.toInt());
}

bool LogModule::logSaveCheck() const {
    QTextDocument document;
    document.setHtml(m_textArea->property("text").toString());
    if (document.toPlainText().isEmpty()) {
        m_toast->show(ToastLevel::Warning, tr("Nothing to export"), tr("Log is empty."));
        return false;
    }
    return true;
}

void LogModule::logSave(const QUrl &fileUrl) {
    const QString filePath = fileUrl.toLocalFile();
    QTextDocument document;
    document.setHtml(m_textArea->property("text").toString());
    if (filePath.endsWith(".txt", Qt::CaseInsensitive)) {
        bool saved = false;
        QSaveFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << document.toPlainText();
            stream.flush();
            if (stream.status() == QTextStream::Ok) saved = file.commit();
        }
        if (saved) {
            m_toast->show(ToastLevel::Success, tr("Log exported"), filePath, {
                {tr("Open"), [this, fileUrl] { emit openFileInApplication(fileUrl); }},
                {tr("Show in Explorer"), [this, fileUrl] { emit openFileInExplorer(fileUrl); }}
            });
        } else {
            m_toast->show(ToastLevel::Error, tr("Log export failed"), filePath);
        }
    }
}

void LogModule::linkClick(const QUrl &customUrl) const {
    // qDebug() << customUrl;
    const QString scheme = customUrl.scheme();
    if (scheme == "request.expand") {
        const QStringList arguments = customUrl.path().split('/');
        const auto data = QString::fromUtf8(
            QByteArray::fromBase64(arguments[1].toLatin1(), QByteArray::Base64UrlEncoding));
        m_textView->setProperty("position", QCursor::pos());
        m_textView->setProperty("data", data);
        QMetaObject::invokeMethod(m_textView, "open");
    }
}
