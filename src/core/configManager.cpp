#include "core/configManager.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>

#include "globals.h"

// public
ConfigManager::ConfigManager(QWidget *parent)
    : QObject(parent) {
    workspaceInit();
}

int ConfigManager::mainConfigLoad() {
    if (const auto rootPath = QDir::current().filePath("config.json"); !QFile::exists(rootPath)) {
        mainConfigGenerate();
        // logging
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        qDebug() << QString("[%1] %2").arg(timestamp, "main config not found");
    }
    auto mainFile = QFile(QDir::current().filePath("config.json"));
    if (!mainFile.open(QIODevice::ReadOnly | QIODevice::Text)) return 1;
    auto mainData = mainFile.readAll();
    mainFile.close();
    auto mainDoc = QJsonDocument::fromJson(mainData);
    auto mainConfig = mainDoc.object();
    const auto workspaceUrlStr = mainConfig["workspace"].toString();
    auto workspaceUrl = QUrl(workspaceUrlStr);
    if (workspaceUrlStr.isEmpty() || !QFileInfo::exists(workspaceUrl.toLocalFile())) {
        const auto workspaceDir = QFileDialog::getExistingDirectory(
            nullptr,
            tr("Open Workspace"),
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
        if (workspaceDir.isEmpty()) {
            return 1;
        }
        workspaceUrl = QUrl::fromLocalFile(workspaceDir);
    }
    mainConfig["version"] = QCoreApplication::applicationVersion();
    mainConfig["workspace"] = workspaceUrl.toString();
    mainDoc = QJsonDocument(mainConfig);
    if (!mainFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return 1;
    mainData = mainDoc.toJson(QJsonDocument::Indented);
    mainFile.write(mainData);
    mainFile.close();

    g_mainConfig = mainConfig;
    g_workspaceUrl = workspaceUrl;

    // logging
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2").arg(timestamp, "main config loaded");

    return 0;
}

void ConfigManager::workspaceInit() {
    const auto workspacePath = g_workspaceUrl.toLocalFile();
    const auto metadataDirPath = QDir(workspacePath).filePath(".unicomm");
    if (!QDir().mkpath(metadataDirPath)) return;
    // config.json
    const auto srcConfigPath = ":/config/config.json";
    const auto dstConfigPath = QDir(metadataDirPath).filePath("config.json");
    // copy if not found
    if (!QFile::exists(dstConfigPath)) {
        QFile::copy(srcConfigPath, dstConfigPath);
        QFile::setPermissions(dstConfigPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                             | QFileDevice::ReadUser | QFileDevice::WriteUser
                                             | QFileDevice::ReadGroup | QFileDevice::ReadOther);
        // logging
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        qDebug() << QString("[%1] .config.json copied").arg(timestamp);
    }
    // validation
    {
        QFile config(dstConfigPath);
        if (!config.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        const QByteArray jsonData = config.readAll();
        config.close();
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
        if (!jsonDoc.isObject()) return;
        const QJsonObject jsonObject = jsonDoc.object();
        // validation document config
        {
            QJsonObject documentConfig = jsonObject["documentConfig"].toObject();
            // remove invalid document url in breakpoint hash
            QJsonObject breakpointHash = documentConfig["breakpointHash"].toObject();
            QJsonObject validBreakpointHash;
            for (auto it = breakpointHash.begin(); it != breakpointHash.end(); ++it) {
                if (const auto documentUrl = QUrl(it.key()); QFileInfo::exists(documentUrl.toLocalFile())) {
                    validBreakpointHash.insert(it.key(), it.value());
                } else {
                    qDebug() << "invalid document url found in breakpoint hash:" << documentUrl;
                }
            }
            documentConfig["breakpointHash"] = validBreakpointHash;
            // write back
            jsonObject["documentConfig"] = documentConfig;
        }
        g_workspaceConfig = jsonObject;
        const QJsonDocument doc(jsonObject);
        if (!config.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return;
        config.write(doc.toJson(QJsonDocument::Indented));
        config.close();
    }
    // logging
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] config loaded").arg(timestamp);
}

void ConfigManager::workspaceConfigSave(const QUrl &configUrl) {
    QString configPath{};
    if (configUrl.isEmpty()) {
        const auto workspacePath = g_workspaceUrl.toLocalFile();
        configPath = QDir(workspacePath).filePath(".unicomm/config.json");
    } else {
        configPath = configUrl.toLocalFile();
    }
    if (QFile workspaceConfig(configPath); workspaceConfig.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        const auto fileUrl = QUrl::fromLocalFile(configPath);
        const QJsonDocument doc(g_workspaceConfig);
        workspaceConfig.write(doc.toJson(QJsonDocument::Indented));
        workspaceConfig.close();
        emit appendLog(LogType::Info, "workspace saved to", QString("<a href='%1'>%2</a>").arg(fileUrl.toString(), fileUrl.toString()));
    } else {
        emit appendLog(LogType::Info, "workspace save failed", "");
    }
}

// private
void ConfigManager::mainConfigGenerate() {
    if (QFile mainConfig(QDir::current().filePath("config.json")); mainConfig.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QJsonObject json{
            {"theme", 0},
            {"version", QCoreApplication::applicationVersion()},
            {"workspace", ""}
        };
        const QJsonDocument doc(json);
        mainConfig.write(doc.toJson(QJsonDocument::Indented));
        mainConfig.close();
    } else {
        // logging
        QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
        qDebug() << QString("[%1] %2").arg(timestamp, "main config generation failed");
    }
    // logging
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2").arg(timestamp, "main config generated");
}
