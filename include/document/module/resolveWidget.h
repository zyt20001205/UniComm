#ifndef UNICOMM_RESOLVEWIDGET_H
#define UNICOMM_RESOLVEWIDGET_H

#include <QQuickWidget>

class ResolveWidget final : public QQuickWidget {
    Q_OBJECT
    Q_PROPERTY(int conflicts READ conflictsGet NOTIFY conflictsChanged)

public:
    explicit ResolveWidget(QWidget *parent = nullptr);

    void propertySet();

    [[nodiscard]] int conflictsGet() const { return m_conflicts; }

    void resolveStat(int conflicts);

    Q_INVOKABLE void resolveFinish();

signals:
    void conflictsChanged();

    void finishResolve();

private:
    int m_conflicts{};
};

#endif //UNICOMM_RESOLVEWIDGET_H
