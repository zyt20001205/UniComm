#include "mainWindow/kddwCustom.h"

#include <QPainter>

#include "document/documentModule.h"
#include "document/page/documentPage.h"
#include "globals.h"
#include "core/globalManager.h"
#include "terminal/terminalModule.h"
#include "terminal/terminalPage.h"

namespace {
int f_theme{};
}

KDDockWidgets::Core::DockWidget *dockWidgetFactory(const QString &uniqueName) {
    const QUrl url(uniqueName);
    // file
    if (url.isLocalFile()) {
        if (auto *page = g_globalManager->documentModuleGet()->documentConstruct(url)) return page->dockWidget();
    }
    // terminal
    if (url.scheme() == QStringLiteral("terminal")) {
        if (auto *page = g_globalManager->terminalModuleGet()->terminalConstruct(url)) return page->dockWidget();
    }
    return nullptr;
}

CustomWidgetFactory::CustomWidgetFactory() {
    f_theme = g_mainConfig["theme"].toInt();
}

KDDockWidgets::Core::View * CustomWidgetFactory::createStack(KDDockWidgets::Core::Stack *controller, KDDockWidgets::Core::View *parent) const {
    return new MyStack(controller, KDDockWidgets::QtCommon::View_qt::asQWidget(parent));
}

KDDockWidgets::Core::View *CustomWidgetFactory::createTabBar(KDDockWidgets::Core::TabBar *controller, KDDockWidgets::Core::View *parent) const {
    return new MyTabBar(controller, KDDockWidgets::QtCommon::View_qt::asQWidget(parent));
}

KDDockWidgets::Core::View *CustomWidgetFactory::createSeparator(KDDockWidgets::Core::Separator *controller, KDDockWidgets::Core::View *parent) const {
    return new MySeparator(controller, parent);
}

// QString CustomWidgetFactory::classicIndicatorsPath() const {
// }

MyStack::MyStack(KDDockWidgets::Core::Stack *controller, QWidget *parent)
    : Stack(controller, parent) {
}

void MyStack::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    if (f_theme == Theme::Light) p.fillRect(QWidget::rect(), "#ffffff");
    else p.fillRect(QWidget::rect(), "#292929");
}

MyTabBar::MyTabBar(KDDockWidgets::Core::TabBar *controller, QWidget *parent)
    : TabBar(controller, parent) {
    setExpanding(false);
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
}

QSize MyTabBar::tabSizeHint(const int index) const {
    auto size = TabBar::tabSizeHint(index);
    size.setWidth(qMin(size.width(), 240));
    return size;
}

MySeparator::MySeparator(KDDockWidgets::Core::Separator *controller, KDDockWidgets::Core::View *parent)
    : Separator(controller, parent) {
}

void MySeparator::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    if (f_theme == Theme::Light) p.fillRect(QWidget::rect(), "#ffffff");
    else p.fillRect(QWidget::rect(), "#292929");

    auto line = QWidget::rect();
    if (line.width() > line.height()) {
        line.setHeight(1);
        line.moveTop((height() - line.height()) / 2);
    } else {
        line.setWidth(1);
        line.moveLeft((width() - line.width()) / 2);
    }
    if (f_theme == Theme::Light) p.fillRect(line, "#d1d1d1");
    else p.fillRect(line, "#666666");
}
