#include "qml/markdownModel.h"

MarkdownTableModel::MarkdownTableModel(const MarkdownTable &table, QObject *parent)
    : QAbstractTableModel(parent), m_table(table) {
}

int MarkdownTableModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_table.rows.size());
}

int MarkdownTableModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_table.header.size());
}

QVariant MarkdownTableModel::data(const QModelIndex &index, const int role) const {
    if (!index.isValid()) return {};
    switch (role) {
        case Qt::DisplayRole: return m_table.rows.at(index.row()).at(index.column());
        case AlignmentRole: return m_table.alignments.at(index.column());
        default: return {};
    }
}

QVariant MarkdownTableModel::headerData(const int section, const Qt::Orientation orientation, const int role) const {
    if (orientation != Qt::Horizontal) return {};
    switch (role) {
        case Qt::DisplayRole: return m_table.header.at(section);
        case AlignmentRole: return m_table.alignments.at(section);
        default: return {};
    }
}

QHash<int, QByteArray> MarkdownTableModel::roleNames() const {
    auto roles = QAbstractTableModel::roleNames();
    roles[AlignmentRole] = "alignment";
    return roles;
}

int MarkdownTableModel::rowsGet() const {
    return static_cast<int>(m_table.rows.size());
}

int MarkdownTableModel::columnsGet() const {
    return static_cast<int>(m_table.header.size());
}

void MarkdownTableModel::tableSet(const MarkdownTable &table) {
    if (m_table == table) return;
    beginResetModel();
    m_table = table;
    endResetModel();
    emit changeTable();
}

MarkdownModel::MarkdownModel(QObject *parent) : QAbstractListModel(parent) {
    m_timer.setInterval(32);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &MarkdownModel::update);
}

int MarkdownModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_blocks.size());
}

QVariant MarkdownModel::data(const QModelIndex &index, const int role) const {
    if (!index.isValid()) return {};
    const auto &block = m_blocks.at(index.row());
    switch (role) {
        case TypeRole: return block.type;
        case ContentRole: return block.content;
        case LanguageRole: return block.language;
        case TableModelRole: return QVariant::fromValue(static_cast<QObject *>(m_tableModels.at(index.row())));
        default: return {};
    }
}

QHash<int, QByteArray> MarkdownModel::roleNames() const {
    return {
        {TypeRole, "type"},
        {ContentRole, "content"},
        {LanguageRole, "language"},
        {TableModelRole, "tableModel"}
    };
}

QString MarkdownModel::sourceGet() const {
    return m_source;
}

void MarkdownModel::sourceSet(const QString &source) {
    if (m_source == source) return;
    m_source = source;
    emit changeSource();
    if (!m_timer.isActive()) m_timer.start();
}

void MarkdownModel::flush() {
    if (!m_timer.isActive()) return;
    m_timer.stop();
    update();
}

// private
void MarkdownModel::update() {
    auto blocks = uni_cast<QList<MarkdownBlock>>(m_source);
    auto structureChanged = blocks.size() != m_blocks.size();
    for (qsizetype index = 0; !structureChanged && index < blocks.size(); ++index) {
        structureChanged = blocks.at(index).type != m_blocks.at(index).type;
    }

    if (structureChanged) {
        beginResetModel();
        qDeleteAll(m_tableModels);
        m_blocks = std::move(blocks);
        m_tableModels.clear();
        for (const auto &block: m_blocks) {
            m_tableModels.append(block.type == MarkdownBlock::Type::Table ? new MarkdownTableModel(block.table, this) : nullptr);
        }
        endResetModel();
        return;
    }

    auto changed = false;
    for (qsizetype index = 0; index < blocks.size(); ++index) {
        if (blocks.at(index).content != m_blocks.at(index).content || blocks.at(index).language != m_blocks.at(index).language) changed = true;
        if (blocks.at(index).type == MarkdownBlock::Type::Table) m_tableModels.at(index)->tableSet(blocks.at(index).table);
    }
    if (!changed) return;
    m_blocks = std::move(blocks);
    emit dataChanged(index(0), index(rowCount({}) - 1), {ContentRole, LanguageRole});
}
