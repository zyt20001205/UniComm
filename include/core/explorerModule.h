#ifndef UNICOMM_EXPLORER_H
#define UNICOMM_EXPLORER_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QSortFilterProxyModel>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/logTypes.h"
#include "types/runtimeTypes.h"

class QAbstractProxyModel;
class QFileSystemModel;
class QFileSystemWatcher;
class QProcess;
class QQuickWidget;
class QStandardItemModel;

class SortFilterProxyModel;

class ExplorerModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ExplorerModule is created by C++.")

public:
    explicit ExplorerModule();

    ~ExplorerModule() override;

    void propertySet() const;

    void indexUpdate() const;

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
    void processFinished(int exitcode);

    QQuickWidget *m_widget{};
    QFileSystemModel *m_fileSystemModel{};
    SortFilterProxyModel *m_sortFilterProxyModel{};
    QProcess *m_process{};
    QHash<QUrl, QVariant> m_documentStatus{};
};

class SortFilterProxyModel final : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit SortFilterProxyModel(const QHash<QUrl, QVariant> *documentStatus, QObject *parent = nullptr);

    enum Role {
        SourceRole = Qt::UserRole + 4,
        IsDirRole,
        DocumentUrlRole,
        GitRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    const QHash<QUrl, QVariant> *m_documentStatus{};
};

#endif //UNICOMM_EXPLORER_H
