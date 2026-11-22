#ifndef UNICOMM_EDITORWIDGET_H
#define UNICOMM_EDITORWIDGET_H

#include "document/module/scintillaWidget.h"

#include <QJsonObject>
#include <QWidget>

#include "types/logTypes.h"

class SearchWidget;

class EditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit EditorWidget(const QJsonObject &documentConfig, const QUrl &documentUrl, QWidget *parent = nullptr);

    ~EditorWidget() override = default;

    void propertySet();

    [[nodiscard]] ScintillaWidget *handler() const { return m_scintillaWidget; }

    [[nodiscard]] virtual QString documentSave();

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void appendLog(LogType::Value type, const QString &prefix, const QString &message);

    void changeSavepoint(bool status);

    void changeSelection(const QHash<QString, int> &selection);

    void changeContent();

    void gotoDocument();

    void showPermissionDialog(const QUrl &documentUrl) const;

protected:
    virtual void shortcutInit();

    virtual void selectionChange();

    virtual void contentChange();

    virtual bool symbolPair(QChar ch);

    void miscInit() const;

    virtual void indicatorInit() const;

    virtual void marginInit() const;

    virtual void markerInit() const;

    void styleInit() const;

    virtual void lexerInit() const;

    void searchShow() const;

    void replaceShow() const;

    QUrl m_documentUrl{};
    ScintillaWidget *m_scintillaWidget{};
    QHash<QString, int> m_selection{};

    QTimer *m_selectionTimer{};
    QTimer *m_contentTimer{};

private:
    struct SearchState {
        int current{};
        QList<ScintillaWidget::PositionRange> matches{};
    };

    void documentOpen() const;

    void permissionSet() const;

    // private: search
    void searchRequest(const QString &text);

    void searchResponse();

    void searchPrev();

    void searchNext();

    void searchClear();

    void textReplace(const QString &text);

    void allReplace(const QString &text);

    QJsonObject m_config{};
    SearchWidget *m_searchWidget{};

    QHash<QChar, QChar> m_pair{};
    SearchState m_search{};
};

#endif //UNICOMM_EDITORWIDGET_H
