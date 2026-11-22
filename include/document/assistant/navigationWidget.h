#ifndef UNICOMM_NAVIGATIONWIDGET_H
#define UNICOMM_NAVIGATIONWIDGET_H

#include <QFont>
#include <QSizeF>
#include <QVariant>

#include "types/scintillaTypes.h"

class NavigationWidget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList model READ modelGet NOTIFY modelChanged)
    Q_PROPERTY(QFont font MEMBER m_font NOTIFY fontChanged)
    Q_PROPERTY(bool visible MEMBER m_visible)
    Q_PROPERTY(QString detailText READ detailTextGet NOTIFY detailTextChanged)
    Q_PROPERTY(qreal rowHeight READ rowHeightGet NOTIFY sizesChanged)
    Q_PROPERTY(QSizeF contentSize READ contentSizeGet NOTIFY sizesChanged)

public:
    explicit NavigationWidget(QObject *parent = nullptr);

    ~NavigationWidget() override = default;

    void propertySet();

    [[nodiscard]] QVariantList modelGet() const {
        return m_model;
    }

    [[nodiscard]] QString detailTextGet() const {
        return m_detailText;
    }

    [[nodiscard]] qreal rowHeightGet() const {
        return m_rowHeight;
    }

    [[nodiscard]] QSizeF contentSizeGet() const {
        return m_contentSize;
    }

    void fontSet(const QString &family, int pointSize);

    [[nodiscard]] bool isVisible() const { return m_visible; }

    void navigationShow(const QVariantHash &navigationSession, const QJsonArray &navigations);

    Q_INVOKABLE void navigationHide();

    void navigationPrev() { emit selectPrevious(); }

    void navigationNext() { emit selectNext(); }

    void navigationAccept() { emit acceptSelection(); }

    Q_INVOKABLE void detailReload(int index);

    Q_INVOKABLE void indicatorInsert(int index);

signals:
    void fontChanged();

    void showNavigation(const QPoint &position);

    void hideNavigation();

    void selectPrevious();

    void selectNext();

    void acceptSelection();

    void modelChanged();

    void detailTextChanged();

    void sizesChanged();

    void setIndex(const QUrl &documentUrl, int startLine, int startCharacter);

    void getText(const QUrl &documentUrl, int startLine, int startCharacter, int endLine, int endCharacter);

    void insertIndicator(const QUrl &documentUrl, ScintillaIndicator::Value type, int startLine, int startCharacter, int endLine, int endCharacter, int time);

    void recordNavigation(const QUrl &documentUrl, int line, int character);

private:
    void sizesRefresh();

    bool m_visible{};
    QVariantList m_model{};
    QString m_detailText{};
    QFont m_font{};
    qreal m_rowHeight{28};
    QSizeF m_contentSize{0, 0};
};

#endif //UNICOMM_NAVIGATIONWIDGET_H
