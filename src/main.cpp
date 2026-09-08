#include "mainWindow/mainWindow.h"

#include <core/crashModule.h>
#include <kddockwidgets/Config.h>
#include <QLoggingCategory>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStyleFactory>

#include "core/configManager.h"
#include "mainWindow/kddwCustom.h"

int main(int argc, char *argv[]) {
    QLoggingCategory::setFilterRules("qt.qpa.mime.warning=false");
    // application style init
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("UniComm"));
    QApplication::setApplicationVersion(QStringLiteral("0.3.1"));
    QApplication::setWindowIcon(QIcon(":/icon/icon.ico"));
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QQuickStyle::setStyle("FluentWinUI3");
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
    if (CrashModule::reporterMode(QCoreApplication::arguments())) {
        return CrashModule::reporterExec(QCoreApplication::arguments());
    }
    // crash module init
    CrashModule crashModule;
    // kddw init
    KDDockWidgets::initFrontend(KDDockWidgets::FrontendType::QtWidgets);
    auto flags = KDDockWidgets::Config::self().flags();
    flags |= KDDockWidgets::Config::Flag_HideTitleBarWhenTabsVisible;
    flags |= KDDockWidgets::Config::Flag_AlwaysShowTabs;
    flags |= KDDockWidgets::Config::Flag_AllowReorderTabs;
    flags |= KDDockWidgets::Config::Flag_TabsHaveCloseButton;
    KDDockWidgets::Config::self().setFlags(flags);
    KDDockWidgets::Config::self().setSeparatorThickness(5);
    // config init
    if (ConfigManager::mainConfigLoad()) return 1;
    KDDockWidgets::Config::self().setViewFactory(new CustomWidgetFactory());
    KDDockWidgets::Config::self().setDockWidgetFactoryFunc(dockWidgetFactory);
    new MainWindow();
    // application exec
    return QApplication::exec();
}
