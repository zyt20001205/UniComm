#ifndef UNICOMM_WATCHMODULE_H
#define UNICOMM_WATCHMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QStandardItemModel>
#include <QtQmlIntegration/qqmlintegration.h>

class QQuickWidget;

class WatchModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("WatchModule is created by C++.")
    Q_PROPERTY(QStandardItemModel* model READ modelGet CONSTANT)

public:
    explicit WatchModule();

    ~WatchModule() override;

    void propertySet();

    void watchConfigSave() const;

    [[nodiscard]] QStandardItemModel *modelGet() const { return m_model; }

    [[nodiscard]] QMultiHash<QUrl, QString> watchExpressionsGet() const;

    Q_INVOKABLE void watchExpressionAdd(const QUrl &documentUrl, const QString &expression);

    Q_INVOKABLE void watchExpressionDelete(int row);

    void watchValuesReload() const;

    void watchValuesSet(const QString &threadId, const QUrl &documentUrl, const QVariantHash &results) const;

    enum Role {
        DocumentUrlRole = Qt::UserRole + 1,
        ResultRole
    };

    Q_ENUM(Role)

signals:
    void addWatch(const QUrl &documentUrl, const QString &expression);

private:
    QQuickWidget *m_widget{};
    QStandardItemModel *m_model{};
};

#endif //UNICOMM_WATCHMODULE_H
