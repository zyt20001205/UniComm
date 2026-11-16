#ifndef UNICOMM_DWELLWIDGET_H
#define UNICOMM_DWELLWIDGET_H

#include <QColor>
#include <QJsonArray>
#include <QUrl>
#include <QVariant>
#include <QWidget>

class WebviewWidget;

class DwellWidget final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList codeActions READ codeActionsGet NOTIFY codeActionsChanged)
    Q_PROPERTY(QVariantList suggestions READ suggestionsGet NOTIFY suggestionsChanged)

public:
    explicit DwellWidget(QWidget *parent = nullptr);

    ~DwellWidget() override = default;

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] QVariantList codeActionsGet() const {
        return m_codeActions;
    }

    [[nodiscard]] QVariantList suggestionsGet() const {
        return m_suggestions;
    }

    void diagnosticSet(const QVariantHash &diagnosticSession, const QJsonArray &diagnostics, const QVariantList &typos);

    void hoverShow(const QVariantHash &hoverSession, const QString &message);

    void codeActionShow(const QUrl &documentUrl, const QJsonArray &result);

    Q_INVOKABLE void dwellHide();

    void linkClick(const QUrl &customUrl);

    Q_INVOKABLE void suggestionAccept(const QString &text);

    Q_INVOKABLE void codeActionAccept(const QJsonObject &codeAction);

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void codeActionsChanged();

    void suggestionsChanged();

    void textSet(const QUrl &documentUrl, const QString &text, int startLine, int startCharacter, int endLine, int endCharacter);

    void requestCodeAction(const QUrl &documentUrl, int startLine, int startCharacter, int endLine, int endCharacter);

protected:
    void hideEvent(QHideEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

private:
    QWidget *m_surface{};
    WebviewWidget *m_webview{};
    QObject *m_codeActionMenu{};
    QObject *m_suggestionMenu{};
    QVariantList m_codeActions{};
    QVariantList m_suggestions{};
    QString m_renderedText{};
    QJsonArray m_diagnostics{};
    QVariantList m_typos{};
    QColor m_background{};
    QColor m_foreground{};
    QPoint m_position{};
    quint64 m_sessionId{};
    bool m_sessionActive{};

    QUrl m_documentUrl{};
    int m_typoStartLine{};
    int m_typoEndLine{};
    int m_typoStartCharacter{};
    int m_typoEndCharacter{};
};

#endif //UNICOMM_DWELLWIDGET_H
