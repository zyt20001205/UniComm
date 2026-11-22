#ifndef UNICOMM_COMPLETIONPOPUP_H
#define UNICOMM_COMPLETIONPOPUP_H

#include <QFont>
#include <QObject>
#include <QSizeF>
#include <QVariant>

class CompletionWidget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList model READ modelGet NOTIFY modelChanged)
    Q_PROPERTY(QFont font MEMBER m_font NOTIFY fontChanged)
    Q_PROPERTY(bool visible MEMBER m_visible)
    Q_PROPERTY(qreal rowHeight READ rowHeightGet NOTIFY sizesChanged)
    Q_PROPERTY(QSizeF contentSize READ contentSizeGet NOTIFY sizesChanged)

public:
    explicit CompletionWidget(QObject *parent = nullptr);

    ~CompletionWidget() override = default;

    void propertySet();

    [[nodiscard]] QVariantList modelGet() const {
        return m_model;
    }

    [[nodiscard]] qreal rowHeightGet() const {
        return m_rowHeight;
    }

    [[nodiscard]] QSizeF contentSizeGet() const {
        return m_contentSize;
    }

    void fontSet(const QString &family, int pointSize);

    [[nodiscard]] bool isVisible() const { return m_visible; }

    void completionShow(const QVariantHash &completionSession, const QJsonArray &items);

    Q_INVOKABLE void completionHide();

    void completionPrev() { emit selectPrevious(); }

    void completionNext() { emit selectNext(); }

    void completionAccept() { emit acceptSelection(); }

    Q_INVOKABLE void textReplace(int index);

signals:
    void fontChanged();

    void showCompletion(const QPoint &position);

    void hideCompletion();

    void selectPrevious();

    void selectNext();

    void acceptSelection();

    void modelChanged();

    void sizesChanged();

    void setIndex(const QUrl &documentUrl, int line, int character);

    void setText(const QUrl &documentUrl, const QString &text, int startLine, int startCharacter, int endLine, int endCharacter);

    void addChar(const QUrl &documentUrl, QChar character);

    void showPosition(const QVariantMap &positionSession);

private:
    void placeholderExpand(const QString &placeholder, QVariantList &model) const;

    void sizesRefresh();

    bool m_visible{};
    QVariantHash m_completionSession{};
    QSet<QString > m_placeholderSet{};
    QVariantList m_model{};
    QFont m_font{};
    qreal m_rowHeight{28};
    QSizeF m_contentSize{0, 0};

    enum CompletionMode {
        Full,
        Simple
    };
};

#endif //UNICOMM_COMPLETIONPOPUP_H
