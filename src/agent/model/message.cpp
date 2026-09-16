#include "agent/model/message.h"

#include <QFileInfo>

#include "util/uniCast.h"

namespace Conversation {
    Message::Message(const QString &id, const QString &role, const QList<QUrl> &attachments, QObject *parent)
        : QObject(parent), m_id(id), m_role(role) {
        for (const auto &attachment: attachments) {
            m_attachments.append(QVariantMap{
                {"attachmentUrl", attachment},
                {"fileName", QFileInfo(attachment.toLocalFile()).fileName()},
                {"iconSource", uni_cast<QFileIcon>(attachment).value}
            });
        }
    }

    QString Message::typeGet() const {
        return "chat";
    }

    QString Message::idGet() const {
        return m_id;
    }

    QString Message::roleGet() const {
        return m_role;
    }

    QString Message::contentGet() const {
        return m_content;
    }

    QVariantList Message::attachmentsGet() const {
        return m_attachments;
    }

    void Message::roleSet(const QString &role) {
        if (m_role == role) return;
        m_role = role;
        emit changeRole();
    }

    void Message::contentAppend(const QString &content) {
        m_content += content;
        emit changeContent();
    }

    void Message::contentReset() {
        m_content.clear();
        emit changeContent();
    }
}
