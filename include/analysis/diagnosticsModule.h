#ifndef UNICOMM_DIAGNOSTICS_H
#define UNICOMM_DIAGNOSTICS_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QStandardItemModel>
#include <QUrl>

class QJsonArray;
class QQuickWidget;
class DiagnosticsModel;

class DiagnosticsModule final : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    Q_PROPERTY(QVariantList diagnostics READ diagnosticsGet NOTIFY changeDiagnostics)

public:
    explicit DiagnosticsModule();

    ~DiagnosticsModule() override;

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] QVariantList diagnosticsGet() const;

    void diagnosticsNotification(const QUrl &documentUrl, const QJsonArray &diagnostics);

    Q_INVOKABLE void indicatorFill(const QVariantHash &position);

signals:
    void changeDiagnostics();

    void setIndex(const QUrl &documentUrl, int startLine, int startCharacter);

    void fillIndicator(const QUrl &documentUrl, int type, int startLine, int startCharacter, int endLine, int endCharacter, int time);

private:
    QQuickWidget *m_widget{};
    QHash<QUrl, DiagnosticsModel *> m_diagnosticsModelHash{};
};

class DiagnosticsModel final : public QStandardItemModel {
    Q_OBJECT

public:
    explicit DiagnosticsModel(QObject *parent = nullptr);

    enum Role {
        SeverityRole = Qt::UserRole + 1,
        PositionRole,
        MessageRole
    };

    void diagnosticsSet(const QUrl &documentUrl, const QJsonArray &diagnostics);

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

private:
    enum {
        LEVEL_ERROR = 1,
        LEVEL_WARNING,
        LEVEL_INFO,
        LEVEL_HINT
    };
};

#endif //UNICOMM_DIAGNOSTICS_H
