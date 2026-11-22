#ifndef UNICOMM_RESOLVEWIDGET_H
#define UNICOMM_RESOLVEWIDGET_H

#include <QQuickWidget>

class ResolveWidget final : public QQuickWidget {
    Q_OBJECT

public:
    explicit ResolveWidget(QWidget *parent = nullptr);

    void propertySet();

    Q_INVOKABLE void propertyGet(const QVariantMap &objects);

    void resolveStat(int conflicts) const;

    Q_INVOKABLE void resolveFinish();

signals:
    void finishResolve();

private:
    QObject *m_root{};
    QObject *m_resolveStatLabel{};
    QObject *m_resolveFinishButton{};
};

#endif //UNICOMM_RESOLVEWIDGET_H
