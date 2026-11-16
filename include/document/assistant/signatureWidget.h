#ifndef UNICOMM_SIGNATUREWIDGET_H
#define UNICOMM_SIGNATUREWIDGET_H

#include <QWidget>

class SignatureWidget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString text READ textGet NOTIFY textChanged)

public:
    explicit SignatureWidget(QObject *parent = nullptr);

    ~SignatureWidget() override = default;

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] QString textGet() const {
        return m_text;
    }

    void fontSet(const QString &family, int pointSize) const;

    [[nodiscard]] bool isVisible() const;

    void signatureShow(const QVariantHash &signatureSession, const QJsonArray &signatures);

    Q_INVOKABLE void signatureHide();

    void signatureNext();

signals:
    void textChanged();

    void setTextSelected(const QUrl &documentUrl, const QString &text);

    void addChar(const QUrl &documentUrl, QChar character);

private:
    QObject *m_tooltip{};
    QString m_text{};
    QVariantHash m_signatureSession{};
};

#endif //UNICOMM_SIGNATUREWIDGET_H
