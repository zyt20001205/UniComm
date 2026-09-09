#ifndef UNICOMM_CONTEXTMODULE_H
#define UNICOMM_CONTEXTMODULE_H

#include "agent/module/sqlModule.h"

class ContextModule final : public QObject {
    Q_OBJECT

public:
    explicit ContextModule(const QJsonObject &config, QObject *parent = nullptr);

    [[nodiscard]] int compactThresholdGet() const;

    void compactThresholdSet(int threshold);

    [[nodiscard]] bool compactRequired(qint64 contextTokens, qint64 contextWindow) const;

    [[nodiscard]] QJsonArray contextBuild(const QString &system, const SqlModule::Conversation &conversation, const QList<SqlModule::Message> &history,
                                          const QList<SqlModule::Message> &turn, const QString &steering) const;

    [[nodiscard]] QJsonArray contextBuild(const QString &system, int mode, const QList<SqlModule::Message> &turn, const QString &steering) const;

    [[nodiscard]] QPair<QString, QJsonArray> compactBuild(const SqlModule::Conversation &conversation, const QList<SqlModule::Message> &history) const;

private:
    struct AttachmentMode {
        enum {
            Reference,
            Embed
        };
    };

    [[nodiscard]] static QString systemBuild(const QString &system, int mode, const QString &summary = {});

    [[nodiscard]] static QJsonObject messageBuild(const SqlModule::Message &message, int attachmentMode = AttachmentMode::Reference);

    QJsonObject m_config{};
};

#endif //UNICOMM_CONTEXTMODULE_H
