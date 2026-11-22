#ifndef UNICOMM_SIGNATUREWIDGET_H
#define UNICOMM_SIGNATUREWIDGET_H

#include <QFont>
#include <QUrl>
#include <QVariantHash>

class SignatureWidget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QFont font MEMBER m_font NOTIFY fontChanged)
    Q_PROPERTY(QString text READ textGet NOTIFY textChanged)
    Q_PROPERTY(bool visible MEMBER m_visible)

public:
    explicit SignatureWidget(QObject *parent = nullptr);

    ~SignatureWidget() override = default;

    void propertySet();

    [[nodiscard]] QString textGet() const {
        return m_text;
    }

    void fontSet(const QString &family, int pointSize);

    [[nodiscard]] bool isVisible() const {
        return m_visible;
    }

    void signatureShow(const QVariantHash &signatureSession, const QJsonArray &signatures);

    void signatureHide();

    void signatureNext();

signals:
    void fontChanged();

    void textChanged();

    void showSignature(const QPoint &position, bool reposition);

    void hideSignature();

    void setTextSelected(const QUrl &documentUrl, const QString &text);

    void addChar(const QUrl &documentUrl, QChar character);

private:
    QFont m_font{};
    QString m_text{};
    bool m_visible{};
    QVariantHash m_signatureSession{};
};

#endif //UNICOMM_SIGNATUREWIDGET_H
