#ifndef UNICOMM_TERMINALPAGE_H
#define UNICOMM_TERMINALPAGE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QJsonObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/terminalTypes.h"

class ConptyWidget;
class QCloseEvent;
class QQuickWidget;
class TerminalWidget;
class VtermWidget;

class TerminalPage : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("TerminalPage is created by C++.")
    Q_MOC_INCLUDE("terminal/module/terminalWidget.h")
    Q_MOC_INCLUDE("terminal/module/vtermWidget.h")
    Q_PROPERTY(VtermWidget* vtermWidget MEMBER m_vtermWidget CONSTANT)

public:
    explicit TerminalPage(const QString &uniqueName, const QVariantHash &session, const QJsonObject &config, TerminalBackend::Value backend = TerminalBackend::Process);

    ~TerminalPage() override;

    void propertySet();

    Q_INVOKABLE void terminalWidgetSet(TerminalWidget *widget);

    void write(const QByteArray &data) const;

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void readTerminal(const QByteArray &data);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void start();

    void _resize(int rows, int cols);

    void stop() const;

    void titleSet(const QString &title);

    QJsonObject m_config{};
    QVariantHash m_session{};
    QQuickWidget *m_widget{};
    ConptyWidget *m_conptyWidget{};
    TerminalWidget *m_terminalWidget{};
    VtermWidget *m_vtermWidget{};
    int m_rows{24};
    int m_cols{80};
};

#endif //UNICOMM_TERMINALPAGE_H
