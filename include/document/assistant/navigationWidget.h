#ifndef UNICOMM_NAVIGATIONWIDGET_H
#define UNICOMM_NAVIGATIONWIDGET_H

#include <QFont>
#include <QSizeF>
#include <QVariant>

class NavigationWidget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList model READ modelGet NOTIFY modelChanged)
    Q_PROPERTY(QString detailText READ detailTextGet NOTIFY detailTextChanged)
    Q_PROPERTY(qreal rowHeight READ rowHeightGet NOTIFY sizesChanged)
    Q_PROPERTY(QSizeF contentSize READ contentSizeGet NOTIFY sizesChanged)

public:
    explicit NavigationWidget(QObject *parent = nullptr);

    ~NavigationWidget() override = default;

    void propertySet(const QVariantHash &objects);

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

    [[nodiscard]] bool isVisible() const;

    void navigationShow(const QVariantHash &navigationSession, const QJsonArray &navigations);

    Q_INVOKABLE void navigationHide();

    void navigationPrev() const;

    void navigationNext() const;

    Q_INVOKABLE void detailReload(int index);

    Q_INVOKABLE void indicatorInsert();

signals:
    void modelChanged();

    void detailTextChanged();

    void sizesChanged();

    void setIndex(const QUrl &documentUrl, int startLine, int startCharacter);

    void getText(const QUrl &documentUrl, int startLine, int startCharacter, int endLine, int endCharacter);

    void insertIndicator(const QUrl &documentUrl, int type, int startLine, int startCharacter, int endLine, int endCharacter, int time);

    void recordNavigation(const QUrl &documentUrl, int line, int character);

private:
    void sizesRefresh();

    QObject *m_tooltip{};
    QObject *m_listView{};
    QVariantList m_model{};
    QString m_detailText{};
    QFont m_font{};
    qreal m_rowHeight{24};
    QSizeF m_contentSize{0, 0};
};

#endif //UNICOMM_NAVIGATIONWIDGET_H
