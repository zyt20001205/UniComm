#ifndef UNICOMM_MESSAGE_H
#define UNICOMM_MESSAGE_H

#include <QUrl>
#include <QVariantList>

namespace Conversation {
    class Message final : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString type READ typeGet CONSTANT)
        Q_PROPERTY(QString id READ idGet CONSTANT)
        Q_PROPERTY(QString role READ roleGet NOTIFY changeRole)
        Q_PROPERTY(QString content READ contentGet NOTIFY changeContent)
        Q_PROPERTY(QVariantList attachments READ attachmentsGet CONSTANT)

    public:
        Message(const QString &id, const QString &role, const QList<QUrl> &attachments, QObject *parent = nullptr);

        [[nodiscard]] QString typeGet() const;

        [[nodiscard]] QString idGet() const;

        [[nodiscard]] QString roleGet() const;

        [[nodiscard]] QString contentGet() const;

        [[nodiscard]] QVariantList attachmentsGet() const;

        void roleSet(const QString &role);

        void contentAppend(const QString &content);

        void contentReset();

    signals:
        void changeRole();

        void changeContent();

    private:
        QString m_id{};
        QString m_role{};
        QString m_content{};
        QVariantList m_attachments{};
    };
}

#endif //UNICOMM_MESSAGE_H
