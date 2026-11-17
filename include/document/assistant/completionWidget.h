#ifndef UNICOMM_COMPLETIONPOPUP_H
#define UNICOMM_COMPLETIONPOPUP_H

#include <QFont>
#include <QSizeF>
#include <QVariant>

class CompletionWidget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList model READ modelGet NOTIFY modelChanged)
    Q_PROPERTY(QVariantList detailModel READ detailModelGet NOTIFY detailModelChanged)
    Q_PROPERTY(qreal rowHeight READ rowHeightGet NOTIFY sizesChanged)
    Q_PROPERTY(QSizeF contentSize READ contentSizeGet NOTIFY sizesChanged)
    Q_PROPERTY(QSizeF detailContentSize READ detailContentSizeGet NOTIFY sizesChanged)

public:
    explicit CompletionWidget(QObject *parent = nullptr);

    ~CompletionWidget() override = default;

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] QVariantList modelGet() const {
        return m_model;
    }

    [[nodiscard]] QVariantList detailModelGet() const {
        return m_detailModel;
    }

    [[nodiscard]] qreal rowHeightGet() const {
        return m_rowHeight;
    }

    [[nodiscard]] QSizeF contentSizeGet() const {
        return m_contentSize;
    }

    [[nodiscard]] QSizeF detailContentSizeGet() const {
        return m_detailContentSize;
    }

    void fontSet(const QString &family, int pointSize);

    [[nodiscard]] bool isVisible() const;

    void completionShow(const QVariantHash &completionSession, const QJsonArray &items);

    Q_INVOKABLE void completionHide();

    void completionPrev() const;

    void completionNext() const;

    Q_INVOKABLE void detailReload(int index);

    Q_INVOKABLE void textReplace();

signals:
    void modelChanged();

    void detailModelChanged();

    void sizesChanged();

    void appendLog(int type, const QString &prefix, const QString &message);

    void setIndex(const QUrl &documentUrl, int line, int character);

    void setText(const QUrl &documentUrl, const QString &text, int startLine, int startCharacter, int endLine, int endCharacter);

    void addChar(const QUrl &documentUrl, QChar character);

    void showPosition(const QVariantMap &positionSession);

private:
    void placeholderExpand(const QString &placeholder, QVariantList &model) const;

    void sizesRefresh();

    QObject *m_tooltip{};
    QObject *m_listView{};
    QVariantHash m_completionSession{};
    QSet<QString > m_placeholderSet{};
    QVariantList m_model{};
    QVariantList m_detailModel{};
    QFont m_font{};
    qreal m_rowHeight{24};
    QSizeF m_contentSize{0, 0};
    QSizeF m_detailContentSize{0, 0};

    enum CompletionMode {
        Full,
        Simple
    };
};

#endif //UNICOMM_COMPLETIONPOPUP_H
