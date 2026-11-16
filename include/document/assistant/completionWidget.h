#ifndef UNICOMM_COMPLETIONPOPUP_H
#define UNICOMM_COMPLETIONPOPUP_H

#include <QStandardItemModel>

class CompletionWidget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel* model READ modelGet CONSTANT)
    Q_PROPERTY(QStandardItemModel* detailModel READ detailModelGet CONSTANT)

public:
    explicit CompletionWidget(QObject *parent = nullptr);

    ~CompletionWidget() override = default;

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] QStandardItemModel *modelGet() const {
        return m_model;
    }

    [[nodiscard]] QStandardItemModel *detailModelGet() const {
        return m_detailModel;
    }

    void fontSet(const QString &family, int pointSize) const;

    [[nodiscard]] bool isVisible() const;

    void completionShow(const QVariantHash &completionSession, const QJsonArray &items);

    Q_INVOKABLE void completionHide() const;

    void completionPrev() const;

    void completionNext() const;

    Q_INVOKABLE void detailReload(int index) const;

    Q_INVOKABLE void textReplace();

signals:
    void appendLog(int type, const QString &prefix, const QString &message);

    void setIndex(const QUrl &documentUrl, int line, int character);

    void setText(const QUrl &documentUrl, const QString &text, int startLine, int startCharacter, int endLine, int endCharacter);

    void addChar(const QUrl &documentUrl, QChar character);

    void showPosition(const QVariantMap &positionSession);

private:
    void placeholderExpand(const QString &placeholder) const;

    enum Role {
        KindRole = Qt::UserRole + 1,
        IconColorRole
    };

    QObject *m_tooltip{};
    QObject *m_tableView{};
    QVariantHash m_completionSession{};
    QSet<QString > m_placeholderSet{};
    QStandardItemModel *m_model{};
    QStandardItemModel *m_detailModel{};

    enum CompletionKind {
        Text = 1,
        Method,
        Function,
        Constructor,
        Field,
        Variable,
        Class,
        Interface,
        Module,
        Property,
        Unit,
        Value,
        Enum,
        Keyword,
        Snippet,
        Color,
        File,
        Reference,
        Folder,
        EnumMember,
        Constant,
        Struct,
        Event,
        Operator,
        TypeParameter
    };

    enum CompletionMode {
        Full,
        Simple
    };
};

#endif //UNICOMM_COMPLETIONPOPUP_H
