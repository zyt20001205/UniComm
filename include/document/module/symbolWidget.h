#ifndef UNICOMM_SYMBOLWIDGET_H
#define UNICOMM_SYMBOLWIDGET_H

#include <QQuickWidget>

class SymbolWidget final : public QQuickWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList model READ modelGet NOTIFY modelChanged)

public:
    explicit SymbolWidget(QWidget *parent = nullptr);

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] QVariantList modelGet() const {
        return m_model;
    }

    void documentSymbolShow(const QJsonArray &result, int line, int character);

    Q_INVOKABLE void indicatorFill(const QVariantHash &position);

signals:
    void modelChanged();

    void appendLog(int type, const QString &prefix, const QString &message);

    void setFocus(bool status);

    void setIndex(int startLine, int startCharacter);

    void fillIndicator(int type, int startLine, int startCharacter, int endLine, int endCharacter, int time);

private:
    QVariantList symbolParse(const QJsonArray &result, int line, int character);

    QVariantList m_model{};
};

#endif //UNICOMM_SYMBOLWIDGET_H
