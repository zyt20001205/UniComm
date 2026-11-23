#ifndef UNICOMM_SYMBOLWIDGET_H
#define UNICOMM_SYMBOLWIDGET_H

#include <QJsonArray>
#include <QQuickWidget>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/scintillaTypes.h"

class SymbolWidget : public QQuickWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("SymbolWidget is created by C++.")
    Q_PROPERTY(QVariantList model READ modelGet NOTIFY modelChanged)

public:
    explicit SymbolWidget(QWidget *parent = nullptr);

    void propertySet();

    [[nodiscard]] QVariantList modelGet() const {
        return m_model;
    }

    void documentSymbolSet(const QJsonArray &result);

    void documentSymbolShow(int line, int character);

    Q_INVOKABLE void indicatorFill(const QVariantHash &position);

signals:
    void modelChanged();

    void setFocus(bool status);

    void setIndex(int startLine, int startCharacter);

    void fillIndicator(ScintillaIndicator::Value type, int startLine, int startCharacter, int endLine, int endCharacter, int time);

private:
    static QVariantList symbolParse(const QJsonArray &result, int line, int character);

    QVariantList m_model{};
    QJsonArray m_symbol{};
};

#endif //UNICOMM_SYMBOLWIDGET_H
