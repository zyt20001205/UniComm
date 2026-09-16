#ifndef UNICOMM_TOOL_H
#define UNICOMM_TOOL_H

#include <QVariantList>

namespace Conversation {
    class Message;

    class Tool final : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString type READ typeGet CONSTANT)
        Q_PROPERTY(QVariantList messages READ messagesGet NOTIFY changeMessages)
        Q_PROPERTY(int count READ countGet NOTIFY changeMessages)
        Q_PROPERTY(bool expanded READ expandedGet WRITE expandedSet NOTIFY changeExpanded)

    public:
        explicit Tool(QObject *parent = nullptr);

        [[nodiscard]] QString typeGet() const;

        [[nodiscard]] QVariantList messagesGet() const;

        [[nodiscard]] int countGet() const;

        [[nodiscard]] bool expandedGet() const;

        void messageAppend(Message *message);

        void expandedSet(bool expanded);

    signals:
        void changeMessages();

        void changeExpanded();

    private:
        QVariantList m_messages{};
        bool m_expanded{false};
    };
}

#endif //UNICOMM_TOOL_H
