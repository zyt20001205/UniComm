#ifndef UNICOMM_POSITIONWIDGET_H
#define UNICOMM_POSITIONWIDGET_H

#include <QMap>
#include <QObject>

class QTimer;

class PositionWidget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool visible MEMBER m_visible NOTIFY visibleChanged)

public:
    explicit PositionWidget(QObject *parent = nullptr);

    ~PositionWidget() override = default;

    void propertySet();

    [[nodiscard]] bool isVisible() const { return m_visible; }

    void positionShow(const QVariantMap &positionSession);

    void positionHide();

    void textReplace();

signals:
    void visibleChanged();

    void showPosition();

    void hidePosition();

    void setPosition(const QPoint &position, const QString &text);

    void setText(const QUrl &documentUrl, const QString &text, int startLine, int startCharacter, int endLine, int endCharacter);

private:
    void positionUpdate();

    bool m_visible{};
    QString m_text{};
    QVariantMap m_positionSession{};
    QTimer *m_timer{};
};

#endif //UNICOMM_POSITIONWIDGET_H
