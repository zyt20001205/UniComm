#ifndef UNICOMM_GITCONFIG_H
#define UNICOMM_GITCONFIG_H

#include <QQueue>
#include <QObject>
#include <QString>
#include <QVariantHash>

class QProcess;

class GitConfig final : public QObject {
    Q_OBJECT

public:
    explicit GitConfig(QObject *parent = nullptr);

    ~GitConfig() override = default;

    void localHttpProxyGet();

    void localHttpsProxyGet();

    void globalHttpProxyGet();

    void globalHttpsProxyGet();

    void gitProxySet(const QString &localHttpProxy, const QString &localHttpsProxy, const QString &globalHttpProxy, const QString &globalHttpsProxy);

signals:
    void showErrorDialog(const QString &title, const QString &text);

    void showProxyDialog(const QVariantHash &proxies);

private:
    void processEnqueue(int command, const QStringList &arguments);

    void processDequeue();

    void processFinished(int exitcode);

    void localHttpProxySet();

    void localHttpsProxySet();

    void globalHttpProxySet();

    void globalHttpsProxySet();

    QVariantHash m_proxies{};

    int m_command{};
    QProcess *m_process{};
    QQueue<QVariantHash> m_queue{};
    QString m_localHttpProxy{};
    QString m_localHttpsProxy{};
    QString m_globalHttpProxy{};
    QString m_globalHttpsProxy{};

    struct GitCommand {
        enum {
            Null,
            LocalHttpProxyGet,
            LocalHttpsProxyGet,
            GlobalHttpProxyGet,
            GlobalHttpsProxyGet,
            LocalHttpProxySet,
            LocalHttpsProxySet,
            GlobalHttpProxySet,
            GlobalHttpsProxySet
        };
    };
};

#endif //UNICOMM_GITCONFIG_H
