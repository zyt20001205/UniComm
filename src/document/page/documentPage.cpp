#include "document/page/documentPage.h"

#include <QCloseEvent>
#include <QDir>

#include "globals.h"
#include "core/globalManager.h"
#include "core/themeManager.h"
#include "util/uniCast.h"

// public
DocumentPage::DocumentPage(const QUrl &documentUrl)
    : DockWidget(documentUrl.toString()),
      m_documentUrl(documentUrl) {
    setTitle(documentUrl.fileName());
    DocumentPage::permissionGet();
}

void DocumentPage::pathDisambiguation() {
    const QString documentPath = m_documentUrl.toLocalFile();
    const QString workspacePath = g_workspaceUrl.toLocalFile();
    const QString relatedPath = QDir(workspacePath).relativeFilePath(documentPath);
    setTitle(relatedPath);
}

void DocumentPage::closeApprove() {
    m_closeApproved = true;
    close();
}

void DocumentPage::permissionGet() {
    const QString documentPath = m_documentUrl.toLocalFile();
    const QFileInfo documentInfo(documentPath);
    documentInfo.isWritable() ? setIcon(uni_cast<QIcon>(m_documentUrl)) :
    g_globalManager->themeGet()->modeGet() == Theme::Light ? setIcon(QIcon(":/icon/lockLight.svg")) :
    setIcon(QIcon(":/icon/lockDark.svg"));
}

// protected
void DocumentPage::closeEvent(QCloseEvent *event) {
    if (!m_closeApproved && documentModified()) {
        event->ignore();
        emit closeRequest(m_documentUrl);
        return;
    }

    event->accept();
    emit closeDocument(m_documentUrl);
    deleteLater();
}
