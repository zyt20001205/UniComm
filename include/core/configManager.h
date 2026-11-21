#ifndef UNICOMM_CONFIG_H
#define UNICOMM_CONFIG_H

#include <QObject>
#include <QUrl>

#include "types/logTypes.h"

class ConfigManager final : public QObject {
    Q_OBJECT

public:
    explicit ConfigManager(QWidget *parent = nullptr);

    ~ConfigManager() override = default;

    static int mainConfigLoad();

    static void workspaceInit();

    void workspaceConfigSave(const QUrl &configUrl);

signals:
    void appendLog(LogType::Value type, const QString &prefix, const QString &message);

    void openWorkspace();

    void quit();

private:
    static void mainConfigGenerate();
};

#endif //UNICOMM_CONFIG_H
