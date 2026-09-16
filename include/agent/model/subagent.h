#ifndef UNICOMM_SUBAGENT_H
#define UNICOMM_SUBAGENT_H

#include <QObject>

namespace Conversation {
    class Subagent final : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString runtimeId READ runtimeIdGet CONSTANT)
        Q_PROPERTY(QString role READ roleGet CONSTANT)
        Q_PROPERTY(QString activity READ activityGet NOTIFY changeActivity)

    public:
        Subagent(const QString &runtimeId, const QString &role, const QString &activity, QObject *parent = nullptr);

        [[nodiscard]] QString runtimeIdGet() const;

        [[nodiscard]] QString roleGet() const;

        [[nodiscard]] QString activityGet() const;

        void activitySet(const QString &activity);

    signals:
        void changeActivity();

    private:
        QString m_runtimeId{};
        QString m_role{};
        QString m_activity{};
    };
}

#endif //UNICOMM_SUBAGENT_H
