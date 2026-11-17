#include "document/page/conflictPage.h"

#include <QDir>
#include <QFileInfo>
#include <QShortcut>
#include <QGridLayout>

#include "globals.h"
#include "service/git/gitModule.h"
#include "core/globalManager.h"
#include "document/module/conflictWidget.h"
#include "document/module/resolveWidget.h"
#include "document/module/scintillaWidget.h"

// public
ConflictPage::ConflictPage(const QJsonObject &documentConfig, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_widget(new QWidget(this)),
      m_conflictWidget(new ConflictWidget(documentConfig, documentUrl, m_widget)),
      m_resolveWidget(new ResolveWidget(m_widget)) {
    setIcon(g_globalManager->themeGet()["mode"].toInt() == Theme::Light ? QIcon(":/icon/gitBranchConflictLight.svg") : QIcon(":/icon/gitBranchConflictDark.svg"));
    auto *layout = new QGridLayout(m_widget); // NOLINT
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_conflictWidget, 0, 0);
    layout->addWidget(m_resolveWidget, 0, 0, Qt::AlignBottom | Qt::AlignHCenter);
    setWidget(m_widget);
    connect(m_conflictWidget, &ConflictWidget::appendLog, this, &ConflictPage::appendLog);
    connect(m_conflictWidget, &ConflictWidget::changeSavepoint, this, &ConflictPage::savepointChange);
    connect(m_conflictWidget, &ConflictWidget::changeSelection, this, &ConflictPage::changeSelection);
    connect(m_conflictWidget, &ConflictWidget::statResolve, m_resolveWidget, &ResolveWidget::resolveStat);
    connect(m_resolveWidget, &ResolveWidget::finishResolve, this, &ConflictPage::resolveFinish);
}

void ConflictPage::propertySet(const QVariantHash &objects) const {
    m_conflictWidget->propertySet(QVariantHash{
        {"mainWindowToast", objects["mainWindowToast"]},
        {"mainWindowToolTip", objects["mainWindowToolTip"]},
        {"fileModulePropertyDialog", objects["fileModulePropertyDialog"]},
        {"documentModuleGotoDialog", objects["documentModuleGotoDialog"]}
    });
    m_resolveWidget->propertySet(QVariantHash{
    });
}

QString ConflictPage::documentSave() {
    return m_conflictWidget->documentSave();
}

// protected
bool ConflictPage::documentModified() const {
    return handler()->modifyGet();
}

// private
void ConflictPage::savepointChange(const bool status) {
    const QString pageName = title();
    if (status) {
        setTitle(pageName + "*");
    } else {
        setTitle(pageName.chopped(1));
    }
}

void ConflictPage::resolveFinish() {
    if (!documentSave().isEmpty()) return;
    connect(g_globalManager->gitModuleGet(), &GitModule::addFinish, this, [this] { emit reloadDocument(m_documentUrl.toLocalFile()); });
    g_globalManager->gitModuleGet()->gitAdd(m_documentUrl);
}
