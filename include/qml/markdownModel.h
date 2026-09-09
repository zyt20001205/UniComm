#ifndef UNICOMM_MARKDOWNMODEL_H
#define UNICOMM_MARKDOWNMODEL_H

#include <QAbstractTableModel>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>

#include "util/uniCast.h"

class MarkdownTableModel final : public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(int rows READ rowsGet NOTIFY changeTable)
    Q_PROPERTY(int columns READ columnsGet NOTIFY changeTable)

public:
    enum Role {
        AlignmentRole = Qt::UserRole + 1
    };

    explicit MarkdownTableModel(const MarkdownTable &table, QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;

    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int rowsGet() const;

    [[nodiscard]] int columnsGet() const;

    void tableSet(const MarkdownTable &table);

signals:
    void changeTable();

private:
    MarkdownTable m_table{};
};

class MarkdownModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString source READ sourceGet WRITE sourceSet NOTIFY changeSource)

public:
    enum Type {
        Markdown = MarkdownBlock::Type::Markdown,
        Code = MarkdownBlock::Type::Code,
        Table = MarkdownBlock::Type::Table
    };
    Q_ENUM(Type)

    enum Role {
        TypeRole = Qt::UserRole + 1,
        ContentRole,
        LanguageRole,
        TableModelRole
    };

    explicit MarkdownModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString sourceGet() const;

    void sourceSet(const QString &source);

    Q_INVOKABLE void flush();

signals:
    void changeSource();

private:
    void update();

    QString m_source{};
    QList<MarkdownBlock> m_blocks{};
    QList<MarkdownTableModel *> m_tableModels{};
    QTimer m_timer{};
};

#endif //UNICOMM_MARKDOWNMODEL_H
