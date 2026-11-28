#ifndef UNICOMM_KDDWCUSTOM_H
#define UNICOMM_KDDWCUSTOM_H

#include <kddockwidgets/qtwidgets/Separator.h>
#include <kddockwidgets/qtwidgets/Stack.h>
#include <kddockwidgets/qtwidgets/TabBar.h>
#include <kddockwidgets/qtwidgets/ViewFactory.h>

#include <QString>

KDDockWidgets::Core::DockWidget *dockWidgetFactory(const QString &uniqueName);

class CustomWidgetFactory final : public KDDockWidgets::QtWidgets::ViewFactory {
    Q_OBJECT

public:
    explicit CustomWidgetFactory() = default;

    KDDockWidgets::Core::View *createStack(KDDockWidgets::Core::Stack *controller, KDDockWidgets::Core::View *parent) const override;

    KDDockWidgets::Core::View *createTabBar(KDDockWidgets::Core::TabBar *controller, KDDockWidgets::Core::View *parent) const override;

    KDDockWidgets::Core::View *createSeparator(KDDockWidgets::Core::Separator *controller, KDDockWidgets::Core::View *parent = nullptr) const override;

    // [[nodiscard]] QString classicIndicatorsPath() const override;
};

class MyStack final : public KDDockWidgets::QtWidgets::Stack {
public:
    explicit MyStack(KDDockWidgets::Core::Stack *controller, QWidget *parent);

    ~MyStack() override = default;

protected:
    void paintEvent(QPaintEvent *event) override;
};

class MyTabBar final : public KDDockWidgets::QtWidgets::TabBar {
public:
    explicit MyTabBar(KDDockWidgets::Core::TabBar *controller, QWidget *parent);

    ~MyTabBar() override = default;

protected:
    [[nodiscard]] QSize tabSizeHint(int index) const override;

    void initStyleOption(QStyleOptionTab *option, int index) const override;
};

class MySeparator final : public KDDockWidgets::QtWidgets::Separator {
public:
    explicit MySeparator(KDDockWidgets::Core::Separator *controller, KDDockWidgets::Core::View *parent);

    ~MySeparator() override = default;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif //UNICOMM_KDDWCUSTOM_H
