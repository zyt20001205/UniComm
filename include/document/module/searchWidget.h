#ifndef UNICOMM_SEARCHWIDGET_H
#define UNICOMM_SEARCHWIDGET_H

#include <QQuickWidget>

class SearchWidget final : public QQuickWidget {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget *parent = nullptr);

    void propertySet();

    Q_INVOKABLE void propertyGet(const QVariantMap &objects);

    void searchShow(const QString &text);

    Q_INVOKABLE void searchFlagsSet(bool matchCase, bool wholeWord, bool wordStart, bool regExp);

    Q_INVOKABLE void searchRequest();

    void searchResponse(const QString &text) const;

    Q_INVOKABLE void searchPrev();

    Q_INVOKABLE void searchNext();

    void searchEnable(bool status) const;

    void replaceShow(const QString &text);

    Q_INVOKABLE void textReplace();

    Q_INVOKABLE void allReplace();

    void replaceEnable(bool status) const;

    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void setSearchFlags(bool matchCase, bool wholeWord, bool wordStart, bool regExp);

    void requestSearch(const QString &text);

    void prevSearch();

    void nextSearch();

    void replaceText(const QString &text);

    void replaceAll(const QString &text);

protected:
    void showEvent(QShowEvent *event) override;

    void hideEvent(QHideEvent *event) override;

private:
    QObject *m_searchBar{};
    QObject *m_searchTextField{};
    QObject *m_searchPrevButton{};
    QObject *m_searchNextButton{};
    QObject *m_searchStatLabel{};
    QObject *m_replaceBar{};
    QObject *m_replaceTextField{};
    QObject *m_replaceTextButton{};
    QObject *m_replaceAllButton{};
};

#endif //UNICOMM_SEARCHWIDGET_H
