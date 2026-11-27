#ifndef UNICOMM_EXPLORER_H
#define UNICOMM_EXPLORER_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QFileSystemModel>
#include <QtQmlIntegration/qqmlintegration.h>

#include "service/git/gitService.h"
#include "types/logTypes.h"
#include "types/runtimeTypes.h"

class QQuickWidget;
class ExplorerModel;

class ExplorerModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ExplorerModule is created by C++.")
    Q_PROPERTY(QAbstractItemModel *model MEMBER m_model CONSTANT)

public:
    explicit ExplorerModule();

    ~ExplorerModule() override;

    void propertySet() const;

    void indexUpdate();

    Q_INVOKABLE void scriptRun(const QUrl &documentUrl);

    Q_INVOKABLE void scriptDebug(const QUrl &documentUrl);

    Q_INVOKABLE void documentOpen(const QUrl &documentUrl);

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void clearSelection();

    void appendLog(LogType::Value type, const QString &prefix, const QString &message);

    void openDocument(const QUrl &documentUrl);

    void startThread(const QUrl &documentUrl, RunMode::Value mode, void *closure = nullptr);

private:
    QQuickWidget *m_widget{};
    ExplorerModel *m_model{};
};

class ExplorerModel final : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;

    enum Role {
        SourceRole = FilePermissions + 1,
        IsDirRole,
        DocumentUrlRole,
        FileStatusRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    [[nodiscard]] int columnCount(const QModelIndex &parent) const override;

    void gitStatusSet(const QList<GitStatusResult::Entry> &files);

private:
    void gitStatusNotify(const QModelIndex &parent);

    QHash<QUrl, GitFileStatus::Value> m_documentStatus{};
};

#endif //UNICOMM_EXPLORER_H
