#ifndef UNICOMM_GITCOMMITMODULE_H
#define UNICOMM_GITCOMMITMODULE_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QtQmlIntegration/qqmlintegration.h>

class GitCommitModule : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("GitCommitModule is created by C++.")

public:
    explicit GitCommitModule();

    Q_INVOKABLE void commitShow();
};

#endif //UNICOMM_GITCOMMITMODULE_H
