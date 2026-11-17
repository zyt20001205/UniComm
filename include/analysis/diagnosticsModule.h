#ifndef UNICOMM_DIAGNOSTICS_H
#define UNICOMM_DIAGNOSTICS_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

class QJsonArray;
class QQuickWidget;
class QStandardItemModel;

class DiagnosticsModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("DiagnosticsModule is created by C++.")
    Q_PROPERTY(QVariantList diagnostics READ diagnosticsGet NOTIFY changeDiagnostics)

public:
    explicit DiagnosticsModule();

    ~DiagnosticsModule() override;

    void propertySet();

    [[nodiscard]] QVariantList diagnosticsGet() const;

    void diagnosticsNotification(const QUrl &documentUrl, const QJsonArray &diagnostics);

    enum Col {
        Severity = 0,
        Source,
        Code,
        Data,
        Message,
        Count
    };

    Q_ENUM(Col)

    enum Level {
        Error = 1,
        Warning = 2,
        Information = 3,
        Hint = 4
    };

    Q_ENUM(Level)

signals:
    void changeDiagnostics();

    void setIndex(const QUrl &documentUrl, int startLine, int startCharacter);

    void fillIndicator(const QUrl &documentUrl, int type, int startLine, int startCharacter, int endLine, int endCharacter, int time);

private:
    static void diagnosticsSet(QStandardItemModel *model, const QUrl &documentUrl, const QJsonArray &diagnostics);

    QQuickWidget *m_widget{};
    QHash<QUrl, QStandardItemModel *> m_diagnosticsModelHash{};

    enum Role {
        SeverityRole = Qt::UserRole + 1,
        DocumentUrlRole,
        StartLineRole,
        StartCharacterRole,
        EndLineRole,
        EndCharacterRole,
        MessageRole
    };
};

#endif //UNICOMM_DIAGNOSTICS_H
