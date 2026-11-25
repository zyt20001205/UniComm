#include "document/page/conflictPage.h"

#include <QGridLayout>
#include <QLabel>
#include <QTextStream>

#include "globals.h"
#include "core/globalManager.h"
#include "document/module/conflictWidget.h"
#include "document/module/editorWidget.h"
#include "document/module/resolveWidget.h"
#include "document/module/scintillaWidget.h"
#include "service/git/gitModule.h"
#include "service/git/gitService.h"
#include "service/serviceModule.h"

// public
ConflictPage::ConflictPage(const QJsonObject &documentConfig, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_widget(new QWidget(this)),
      m_conflictWidget(new ConflictWidget(documentConfig, documentUrl, m_widget)),
      m_resolveWidget(new ResolveWidget(m_widget)) {
    setIcon(g_globalManager->themeGet()["mode"].toInt() == Theme::Light ? QIcon(":/icon/gitBranchConflictLight.svg") : QIcon(":/icon/gitBranchConflictDark.svg"));
    auto *layout = new QGridLayout(m_widget); // NOLINT
    layout->setContentsMargins(0, 0, 0, 0);
    auto *ours = new EditorWidget(documentConfig, {}, m_widget);
    auto *theirs = new EditorWidget(documentConfig, {}, m_widget);
    for (auto *editor : {ours, theirs}) {
        editor->propertySet();
        editor->handler()->readonlySet(true);
    }
    auto *oursLabel = new QLabel(tr("Ours"), m_widget);
    auto *theirsLabel = new QLabel(tr("Theirs"), m_widget);
    layout->addWidget(oursLabel, 0, 0);
    layout->addWidget(new QLabel(tr("Working Tree"), m_widget), 0, 1);
    layout->addWidget(theirsLabel, 0, 2);
    layout->addWidget(ours, 1, 0);
    layout->addWidget(m_conflictWidget, 1, 1);
    layout->addWidget(m_resolveWidget, 1, 1, Qt::AlignBottom | Qt::AlignHCenter);
    layout->addWidget(theirs, 1, 2);
    for (int column = 0; column < 3; ++column) layout->setColumnStretch(column, 1);
    setWidget(m_widget);
    connect(m_conflictWidget, &ConflictWidget::appendLog, this, &ConflictPage::appendLog);
    connect(m_conflictWidget, &ConflictWidget::changeSavepoint, this, &ConflictPage::savepointChange);
    connect(m_conflictWidget, &ConflictWidget::changeSelection, this, &ConflictPage::changeSelection);
    connect(m_conflictWidget, &EditorWidget::gotoDocument, this, &DocumentPage::gotoDocument);
    connect(m_conflictWidget, &EditorWidget::showPermissionDialog, this, &DocumentPage::showPermissionDialog);
    connect(m_conflictWidget, &ConflictWidget::statResolve, m_resolveWidget, &ResolveWidget::resolveStat);
    connect(m_resolveWidget, &ResolveWidget::finishResolve, this, &ConflictPage::resolveFinish);

    auto *git = g_globalManager->serviceModuleGet()->gitServiceGet();
    git->conflictGet(documentUrl).then(this, [this, ours, theirs, oursLabel, theirsLabel](const GitConflictResult &result) {
        if (!result.success) {
            emit appendLog(LogType::Error, tr("Git command failed"), result.text());
            return;
        }
        if (!result.base && !result.ours && !result.theirs) return;
        const auto display = [](const ScintillaWidget *editor, const QByteArray &contents) {
            editor->readonlySet(false);
            editor->textAppend(QTextStream(contents).readAll());
            editor->readonlySet(true);
        };
        if (result.ours) display(ours->handler(), *result.ours);
        else oursLabel->setText(tr("Ours (deleted)"));
        if (result.theirs) display(theirs->handler(), *result.theirs);
        else theirsLabel->setText(tr("Theirs (deleted)"));
    });
}

void ConflictPage::propertySet() const {
    m_conflictWidget->propertySet();
    m_resolveWidget->propertySet();
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
    g_globalManager->gitModuleGet()->add(m_documentUrl).then(this, [this](bool success) {
        if (success) emit reloadDocument(m_documentUrl.toLocalFile());
    });
}
