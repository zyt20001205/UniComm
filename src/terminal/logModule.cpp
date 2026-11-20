#include "terminal/logModule.h"

#include <QFileInfo>
#include <QPdfWriter>
#include <QSaveFile>
#include <QQuickWidget>

#include "globals.h"
#include "core/fileModule.h"
#include "core/globalManager.h"
#include "mainWindow/toastModule.h"

// public
LogModule::LogModule()
    : DockWidget("Log"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)),
      m_errorFore(g_globalManager->themeGet()["general"].toObject()["dangerFore3"].toString()),
      m_warningFore(g_globalManager->themeGet()["general"].toObject()["warningFore3"].toString()),
      m_infoFore(g_globalManager->themeGet()["general"].toObject()["fore"].toString()) {
    const auto config = g_workspaceConfig["logConfig"].toObject();
    m_timestamp = config["timestamp"].toBool();
    m_wrap = config["wrap"].toBool();
    m_collapseThreshold = config["collapseThreshold"].toInt(m_collapseThreshold);
    m_lineLimit = config["lineLimit"].toInt(config["height"].toInt(m_lineLimit));
    m_font = QFont(config["fontFamily"].toString(), config["fontSize"].toInt());
    setWidget(m_widget);
}

LogModule::~LogModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void LogModule::propertySet() {
    m_widget->setInitialProperties({{"logModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/terminal/logModule.qml"));
}

void LogModule::logConfigSave() const {
    auto config = g_workspaceConfig["logConfig"].toObject();
    config.remove("height");
    config["collapseThreshold"] = m_collapseThreshold;
    config["lineLimit"] = m_lineLimit;
    config["timestamp"] = m_timestamp;
    config["wrap"] = m_wrap;
    g_workspaceConfig["logConfig"] = config;
}

void LogModule::logAppend(const int type, const QString &prefix, const QString &message) {
    auto _message = message;
    // check size
    const auto size = _message.size();
    if (size >= m_collapseThreshold) {
        _message = QString::fromLatin1(_message.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
        _message = QString("<a href='request.expand://reserved/%1'>[%2 chars collapsed]</a>").arg(_message, QString::number(size));
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
    if (m_timestamp) {
        const auto timestamp = QString("[%1] ").arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"));
        _message = timestamp + _message;
    }
    emit appendText(_message);
}

void LogModule::documentSet(const QQuickTextDocument *document) {
    m_textDocument = document->textDocument();
    m_textDocument->setMaximumBlockCount(m_lineLimit);
    connect(this, &LogModule::lineLimitChanged, m_textDocument, &QTextDocument::setMaximumBlockCount);
}

void LogModule::logSave(const QUrl &fileUrl) const {
    auto *toast = g_globalManager->toastModuleGet();
    const QString filePath = fileUrl.toLocalFile();
    const auto suffix = QFileInfo(filePath).suffix().toLower();
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        toast->show(ToastLevel::Error, tr("Log export failed"), filePath);
        return;
    }

    if (suffix == "pdf") {
        QPdfWriter writer(&file);
        m_textDocument->print(&writer);
    } else if (suffix == "txt" || suffix == "html") {
        const auto data = (suffix == "txt" ? m_textDocument->toPlainText() : m_textDocument->toHtml()).toUtf8();
        if (file.write(data) != data.size()) file.cancelWriting();
    } else {
        file.cancelWriting();
    }
    if (file.commit()) {
        toast->show(ToastLevel::Success, tr("Log exported"), filePath, {
                        {.text = tr("Open"), .callback = [fileUrl] { g_globalManager->fileModuleGet()->fileOpenInApplication(fileUrl); }},
                        {.text = tr("Show in Explorer"), .callback = [fileUrl] { g_globalManager->fileModuleGet()->fileOpenInExplorer(fileUrl); }}
                    });
    } else {
        toast->show(ToastLevel::Error, tr("Log export failed"), filePath);
    }
}

QString LogModule::linkTextGet(const QUrl &url) {
    return QString::fromUtf8(QByteArray::fromBase64(url.path().mid(1).toLatin1(), QByteArray::Base64UrlEncoding));
}
