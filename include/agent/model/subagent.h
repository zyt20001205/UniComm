#ifndef UNICOMM_SUBAGENT_H
#define UNICOMM_SUBAGENT_H

#include <QObject>
#include <QVariantList>

namespace Conversation {
    class Subagent final : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString type READ typeGet CONSTANT)
        Q_PROPERTY(QVariantList actions READ actionsGet NOTIFY changeActions)
        Q_PROPERTY(int count READ countGet NOTIFY changeActions)
        Q_PROPERTY(bool expanded READ expandedGet WRITE expandedSet NOTIFY changeExpanded)

    public:
        explicit Subagent(QObject *parent = nullptr);

        [[nodiscard]] QString typeGet() const;

        [[nodiscard]] QVariantList actionsGet() const;

        [[nodiscard]] int countGet() const;

        [[nodiscard]] bool expandedGet() const;

        void actionAppend(const QString &activity);

        void expandedSet(bool expanded);

    signals:
        void changeActions();

        void changeExpanded();

    private:
        QVariantList m_actions{};
        bool m_expanded{false};
    };
}

#endif //UNICOMM_SUBAGENT_H
