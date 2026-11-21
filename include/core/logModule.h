#ifndef UNICOMM_LOG_H
#define UNICOMM_LOG_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QFont>
#include <QQuickTextDocument>
#include <QtQmlIntegration/qqmlintegration.h>

#include "types/logTypes.h"

class QQuickWidget;

class LogModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("LogModule is created by C++.")
    Q_PROPERTY(bool timestamp MEMBER m_timestamp NOTIFY timestampChanged)
    Q_PROPERTY(bool wrap MEMBER m_wrap NOTIFY wrapChanged)
    Q_PROPERTY(bool pause MEMBER m_pause NOTIFY pauseChanged)
    Q_PROPERTY(int collapseThreshold MEMBER m_collapseThreshold NOTIFY collapseThresholdChanged)
    Q_PROPERTY(int lineLimit MEMBER m_lineLimit NOTIFY lineLimitChanged)
    Q_PROPERTY(QFont logFont MEMBER m_font CONSTANT)

public:
    explicit LogModule();

    ~LogModule() override;

    void propertySet();

    void logConfigSave() const;

    void logAppend(LogType::Value type, const QString &prefix, const QString &message);

    Q_INVOKABLE void documentSet(const QQuickTextDocument *document);

    Q_INVOKABLE void logSave(const QUrl &fileUrl) const;

    Q_INVOKABLE QString linkTextGet(int position) const;

signals:
    void timestampChanged();

    void wrapChanged();

    void pauseChanged();

    void collapseThresholdChanged();

    void lineLimitChanged(int limit);

    void appendText(const QString &text);

private:
    bool m_timestamp{};
    bool m_wrap{};
    bool m_pause{};
    int m_collapseThreshold{200};
    int m_lineLimit{1000};
    QFont m_font{};
    QQuickWidget *m_widget{};
    QTextDocument *m_textDocument{};
    QString m_errorFore{};
    QString m_warningFore{};
    QString m_infoFore{};
};

#endif //UNICOMM_LOG_H
