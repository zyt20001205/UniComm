#ifndef UNICOMM_STRUCTUREMODULE_H
#define UNICOMM_STRUCTUREMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QStandardItemModel>
#include <QtQmlIntegration/qqmlintegration.h>
#include <set>

#include "types/scintillaTypes.h"

class QQuickWidget;
class QStandardItem;

class StructureModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("StructureModule is created by C++.")
    Q_PROPERTY(QVariantHash models READ modelsGet NOTIFY changeModels)

public:
    explicit StructureModule();

    ~StructureModule() override;

    void propertySet();

    [[nodiscard]] QVariantHash modelsGet() const;

    void documentSymbolResponse(const QUrl &documentUrl, const QJsonArray &result);

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void changeModels();

    void clearSelection();

    void setFocus(const QUrl &documentUrl, bool status);

    void setIndex(const QUrl &documentUrl, int startLine, int startCharacter);

    void addMarker(const QUrl &documentUrl, ScintillaMarker::Value type, int line, int time);

private:
    void documentSymbolSet(const QJsonArray &result, QStandardItem *parentItem, std::set<int> &kinds);

    enum Role {
        KindRole = Qt::UserRole + 1,
        DetailRole,
        StartLineRole,
        StartCharacterRole
    };

    QQuickWidget *m_widget{};
    QHash<QUrl, QStandardItemModel *> m_documentSymbolModelHash{};
};

#endif //UNICOMM_STRUCTUREMODULE_H
