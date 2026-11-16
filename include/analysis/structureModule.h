#ifndef UNICOMM_STRUCTUREMODULE_H
#define UNICOMM_STRUCTUREMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QStandardItemModel>

class QQuickWidget;
class QStandardItem;
class QTreeView;

class StructureModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel* model READ modelGet CONSTANT)

public:
    explicit StructureModule();

    ~StructureModule() override;

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] QStandardItemModel *modelGet() const {
        return m_model;
    }

    Q_INVOKABLE void propertyGet(const QVariantMap &objects);

    void documentSymbolResponse(const QUrl &documentUrl, const QJsonArray &result);

    void documentFocus(const QUrl &documentUrl, const QVariantHash &session);

    Q_INVOKABLE void markerAdd(const QVariantHash &position);

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void appendLog(int type, const QString &prefix, const QString &message);

    void setFocus(const QUrl &documentUrl, bool status);

    void setIndex(const QUrl &documentUrl, int startLine, int startCharacter);

    void addMarker(const QUrl &documentUrl, int type, int line, int time);

private:
    enum Role {
        DetailRole = Qt::UserRole + 1,
        PositionRole,
        IconColorRole
    };

    void documentSymbolPublish(const QJsonArray &result, QStandardItem *parentItem);

    QUrl m_documentUrl{};
    QQuickWidget *m_widget{};
    QObject *m_treeView{};
    QStandardItemModel *m_model{};
    QHash<QUrl, QJsonArray> m_documentSymbolHash{};
};

#endif //UNICOMM_STRUCTUREMODULE_H
