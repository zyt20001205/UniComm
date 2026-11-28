#ifndef UNICOMM_GITCOMMITMODULE_H
#define UNICOMM_GITCOMMITMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QStandardItemModel>
#include <QtQmlIntegration/qqmlintegration.h>

class GitService;
class QQuickWidget;

class GitCommitModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("GitCommitModule is created by C++.")
    Q_PROPERTY(QStandardItemModel* changes MEMBER m_changes CONSTANT)

public:
    explicit GitCommitModule();

    void propertySet();

    Q_INVOKABLE void commitShow();

    Q_INVOKABLE void commit(const QString &message, bool amend, bool push);

    Q_INVOKABLE void checkStateSet(const QModelIndex &index, bool checked) const;

    Q_INVOKABLE void diffOpen(const QModelIndex &index);

signals:
    void committed(bool push);

    void indexUpdated();

private:
    void indexUpdate();

    static void groupCheckStateUpdate(QStandardItem *group);

    enum Role { FileStatusRole = Qt::UserRole + 1, DocumentUrlRole, OriginalUrlRole, DirectoryRole, FileCountRole };

    GitService *const m_gitService;
    QQuickWidget *m_widget{};
    QStandardItemModel *m_changes{};
};

#endif //UNICOMM_GITCOMMITMODULE_H
