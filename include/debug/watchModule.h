#ifndef UNICOMM_WATCHMODULE_H
#define UNICOMM_WATCHMODULE_H

#include <QStandardItemModel>
#include <kddockwidgets/qtwidgets/views/DockWidget.h>

class QQuickWidget;

class WatchModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel* model READ modelGet CONSTANT)
    Q_PROPERTY(bool refresh READ refreshGet WRITE refreshSet)

public:
    explicit WatchModule();

    ~WatchModule() override;

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] QStandardItemModel *modelGet() const {
        return m_model;
    }

    [[nodiscard]] bool refreshGet() const {
        return m_refresh;
    }

    void refreshSet(const bool refresh) {
        m_refresh = refresh;
    }

    void watchConfigSave();

    Q_INVOKABLE void watchInsert(int index, const QUrl &documentUrl, const QString &expression);

    Q_INVOKABLE void watchRemove(int index);

    Q_INVOKABLE void watchRename(int index, const QUrl &documentUrl, const QString &expression);

    Q_INVOKABLE void watchSwap(int src, int dst) const;

    Q_INVOKABLE void watchClear(int index);

private:
    QQuickWidget *m_widget{};
    QQuickItem *m_item{};
    QStandardItemModel *m_model{};
    bool m_refresh = false;
};

#endif //UNICOMM_WATCHMODULE_H
