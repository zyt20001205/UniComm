#include "document/documentModule.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QImageReader>
#include <QProcess>
#include <QSet>
#include <QSharedPointer>
#include <QTextBrowser>

#include "globals.h"
#include "core/fileModule.h"
#include "core/globalManager.h"
#include "core/undoModule.h"
#include "document/assistant/codeAssistant.h"
#include "document/module/scintillaWidget.h"
#include "document/page/webPage.h"
#include "document/page/imagePage.h"
#include "document/page/codePage.h"
#include "document/page/pdfPage.h"
#include "document/page/textPage.h"
#include "document/page/conflictPage.h"
#include "document/page/markupPage.h"
#include "document/page/welcomePage.h"
#include "mainWindow/toastModule.h"
#include "util/uniCast.h"

// public
DocumentModule::DocumentModule(QWidget *parent)
    : QObject(parent),
      m_config(g_workspaceConfig["documentConfig"].toObject()),
      m_watcher(new QFileSystemWatcher(this)),
      m_welcomePage(new WelcomePage()),
      m_codeAssistant(new CodeAssistant(parent)) {
    m_navigationHistory = QVariantHash{
        {"index", -1},
        {"list", QVariantList{}}
    };
    const auto themeDir = QDir(QCoreApplication::applicationDirPath());
    auto themeFile = QFile(themeDir.filePath(QString("theme/%1.json").arg(QString::number(g_mainConfig["theme"].toInt()))));
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const auto themeData = themeFile.readAll();
        themeFile.close();
        const auto themeDoc = QJsonDocument::fromJson(themeData);
        m_theme = themeDoc.object();
    }

    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &DocumentModule::documentReload);
    qApp->installEventFilter(m_codeAssistant);
    connect(m_welcomePage, &WelcomePage::openWorkspace, this, &DocumentModule::openWorkspace);
    connect(m_welcomePage, &WelcomePage::openDocument, this, &DocumentModule::documentOpen, Qt::QueuedConnection);
    connect(this, &DocumentModule::responseCodeAction, m_codeAssistant, &CodeAssistant::codeActionShow);
    connect(m_codeAssistant, &CodeAssistant::addChar, this, &DocumentModule::charAdd);
    connect(m_codeAssistant, &CodeAssistant::setIndex, this, &DocumentModule::indexSet);
    connect(m_codeAssistant, &CodeAssistant::getText, this, &DocumentModule::textGet);
    connect(m_codeAssistant, &CodeAssistant::setText, this, &DocumentModule::textSet);
    connect(m_codeAssistant, &CodeAssistant::setTextSelected, this, &DocumentModule::textSetSelected);
    connect(m_codeAssistant, &CodeAssistant::addMarker, this, &DocumentModule::markerAdd);
    connect(m_codeAssistant, &CodeAssistant::insertIndicator, this, &DocumentModule::indicatorFill);
    connect(m_codeAssistant, &CodeAssistant::requestCodeAction, this, &DocumentModule::codeActionRequest);
    connect(m_codeAssistant, &CodeAssistant::recordNavigation, this, &DocumentModule::navigationRecord);
}

DocumentModule::~DocumentModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] document module destructed").arg(timestamp);
}

void DocumentModule::propertySet(const QVariantHash &objects) {
    m_toast = qvariant_cast<ToastModule *>(objects["mainWindowToast"]);
    m_toolTip = qvariant_cast<QObject *>(objects["mainWindowToolTip"]);
    m_breakpointEditDialog = qvariant_cast<QObject *>(objects["breakpointModuleEditDialog"]);
    m_systemPropertyDialog = qvariant_cast<QObject *>(objects["fileModulePropertyDialog"]);
    m_gotoDialog = qvariant_cast<QObject *>(objects["documentModuleGotoDialog"]);
    m_renameDialog = qvariant_cast<QObject *>(objects["documentModuleRenameDialog"]);
    m_saveDialog = qvariant_cast<QObject *>(objects["documentModuleSaveDialog"]);
    m_editorMenu = qvariant_cast<QObject *>(objects["documentModuleEditorMenu"]);

    // const auto focusedUrl = QUrl(m_config["documentFocused"].toString());
    // if (!focusedUrl.isEmpty() && m_pageHash.contains(focusedUrl)) {
    //     QTimer::singleShot(0, this, [this, focusedUrl] {
    //         m_pageHash[focusedUrl]->setFocus(Qt::FocusReason::MouseFocusReason);
    //     });
    // }

    m_welcomePage->propertySet(QVariantHash{
    });
    m_codeAssistant->propertySet(objects);
    m_codeAssistant->fontSet(m_config["fontFamily"].toString(), m_config["fontSize"].toInt());
}

void DocumentModule::documentConfigSave() {
    // save config
    for (const auto &url: m_pageHash.keys()) {
        (void) documentSave(url);
    }
    if (m_focusedUrl.isEmpty()) {
        m_config["documentFocused"] = "";
    } else {
        m_config["documentFocused"] = m_focusedUrl.toString();
    }
    g_workspaceConfig["documentConfig"] = m_config;
}

void DocumentModule::scriptFontReload(const QJsonObject &fontConfigScript) const {
    // const auto scriptFont = QFont(fontConfigScript["fontFamily"].toString(), fontConfigScript["fontSize"].toInt());
    // for (const auto &codePage: m_pageHash) {
    //     codePage->m_editorWidget->setFont(scriptFont);
    // }
}

void DocumentModule::scriptFontSave(const QJsonObject &fontConfigScript) {
    m_config["fontFamily"] = fontConfigScript["fontFamily"].toString();
    m_config["fontSize"] = fontConfigScript["fontSize"].toInt();
}

// public: directory
QJsonArray DocumentModule::directoryList(const QUrl &directoryUrl) const {
    QJsonArray entries{};
    const QDir directory(directoryUrl.toLocalFile());
    const auto fileInfos = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDir::DirsFirst | QDir::Name);
    for (const auto &fileInfo: fileInfos) {
        entries.append(QJsonObject{
            {"name", fileInfo.fileName()},
            {"url", QUrl::fromLocalFile(fileInfo.absoluteFilePath()).toString()},
            {"type", fileInfo.isDir() ? "directory" : "document"}
        });
    }
    return entries;
}

QString DocumentModule::directoryCreate(const QUrl &directoryUrl, const QString &undoGroupId) {
    if (!directoryUrl.isLocalFile()) return tr("Directory create failed: URL is not local.");

    const QString directoryPath = directoryUrl.toLocalFile();
    if (QFileInfo::exists(directoryPath)) return tr("Directory create failed: path already exists.");

    const auto trashUrl = QSharedPointer<QUrl>::create();
    const auto text = tr("Directory Create (%1)").arg(QFileInfo(directoryPath).fileName());
    const auto redo = [this, directoryUrl, trashUrl] {
        if (trashUrl->isEmpty()) return _directoryCreate(directoryUrl);
        return _directoryRestore(directoryUrl, *trashUrl);
    };
    const auto undo = [this, directoryUrl, trashUrl] { return _directoryDelete(directoryUrl, *trashUrl); };
    if (!undoGroupId.isEmpty()) return g_undo->push(text, redo, undo, undoGroupId);
    return g_undo->push(text, redo, undo, [this](const QString &error) { m_toast->show(ToastLevel::Error, tr("Directory"), error); });
}

QString DocumentModule::directoryRename(const QUrl &sourceUrl, const QUrl &targetUrl, const QString &undoGroupId) {
    if (!sourceUrl.isLocalFile() || !targetUrl.isLocalFile()) return tr("Directory rename failed: URL is not local.");

    const QString sourcePath = sourceUrl.toLocalFile();
    const QString targetPath = targetUrl.toLocalFile();
    const QFileInfo directoryInfo(sourcePath);
    if (!directoryInfo.isDir()) return tr("Directory rename failed: directory does not exist.");
    if (sourcePath == targetPath) return {};
    if (!undoGroupId.isEmpty()) {
        const auto error = _transactionCheck(undoGroupId, sourceUrl, true);
        if (!error.isEmpty()) return error;
    }

    const auto text = tr("Directory Rename (%1->%2)").arg(QFileInfo(sourcePath).fileName(), QFileInfo(targetPath).fileName());
    const auto redo = [this, sourceUrl, targetUrl] { return _directoryRename(sourceUrl, targetUrl); };
    const auto undo = [this, sourceUrl, targetUrl] { return _directoryRename(targetUrl, sourceUrl); };
    const auto error = undoGroupId.isEmpty()
                           ? g_undo->push(text, redo, undo, [this](const QString &message) { m_toast->show(ToastLevel::Error, tr("Directory"), message); })
                           : g_undo->push(text, redo, undo, undoGroupId);
    if (error.isEmpty() && !undoGroupId.isEmpty()) _transactionRename(undoGroupId, sourceUrl, targetUrl, true);
    return error;
}

QString DocumentModule::directoryDelete(const QUrl &directoryUrl, const QString &undoGroupId) {
    if (!directoryUrl.isLocalFile()) return tr("Directory delete failed: URL is not local.");

    const QString directoryPath = directoryUrl.toLocalFile();
    if (!QFileInfo(directoryPath).isDir()) return tr("Directory delete failed: directory does not exist.");

    if (!undoGroupId.isEmpty()) {
        const auto error = _transactionFlush(undoGroupId, directoryUrl, true);
        if (!error.isEmpty()) return error;
    }

    const auto trashUrl = QSharedPointer<QUrl>::create();
    const auto text = tr("Directory Delete (%1)").arg(QFileInfo(directoryPath).fileName());
    const auto redo = [this, directoryUrl, trashUrl] { return _directoryDelete(directoryUrl, *trashUrl); };
    const auto undo = [this, directoryUrl, trashUrl] { return _directoryRestore(directoryUrl, *trashUrl); };
    if (!undoGroupId.isEmpty()) return g_undo->push(text, redo, undo, undoGroupId);
    return g_undo->push(text, redo, undo, [this](const QString &error) { m_toast->show(ToastLevel::Error, tr("Directory"), error); });
}

// public: document
QString DocumentModule::documentCreate(const QUrl &documentUrl, const QString &undoGroupId) {
    if (!documentUrl.isLocalFile()) return tr("Document create failed: URL is not a local file.");

    const QString documentPath = documentUrl.toLocalFile();
    if (QFileInfo::exists(documentPath)) return tr("Document create failed: path already exists.");

    const auto trashUrl = QSharedPointer<QUrl>::create();
    const auto text = tr("Document Create (%1)").arg(documentUrl.fileName());
    const auto redo = [this, documentUrl, trashUrl] {
        if (trashUrl->isEmpty()) return _documentCreate(documentUrl);

        const auto error = _documentRestore(documentUrl, *trashUrl);
        if (error.isEmpty()) documentOpen(documentUrl);
        return error;
    };
    const auto undo = [this, documentUrl, trashUrl] { return _documentDelete(documentUrl, *trashUrl); };
    if (!undoGroupId.isEmpty()) return g_undo->push(text, redo, undo, undoGroupId);
    return g_undo->push(text, redo, undo, [this](const QString &error) { m_toast->show(ToastLevel::Error, tr("Document"), error); });
}

QString DocumentModule::documentRename(const QUrl &sourceUrl, const QUrl &targetUrl, const QString &undoGroupId) {
    if (!sourceUrl.isLocalFile() || !targetUrl.isLocalFile()) return tr("Document rename failed: URL is not a local file.");

    const QString sourcePath = sourceUrl.toLocalFile();
    const QString targetPath = targetUrl.toLocalFile();
    const QFileInfo documentInfo(sourcePath);
    if (!documentInfo.isFile()) return tr("Document rename failed: document does not exist.");
    if (sourcePath == targetPath) return {};
    if (!undoGroupId.isEmpty()) {
        const auto error = _transactionCheck(undoGroupId, sourceUrl, false);
        if (!error.isEmpty()) return error;
    }

    const auto text = tr("Document Rename (%1->%2)").arg(sourceUrl.fileName(), targetUrl.fileName());
    const auto redo = [this, sourceUrl, targetUrl] { return _documentRename(sourceUrl, targetUrl); };
    const auto undo = [this, sourceUrl, targetUrl] { return _documentRename(targetUrl, sourceUrl); };
    const auto error = undoGroupId.isEmpty()
                           ? g_undo->push(text, redo, undo, [this](const QString &message) { m_toast->show(ToastLevel::Error, tr("Document"), message); })
                           : g_undo->push(text, redo, undo, undoGroupId);
    if (error.isEmpty() && !undoGroupId.isEmpty()) _transactionRename(undoGroupId, sourceUrl, targetUrl, false);
    return error;
}

QString DocumentModule::documentDelete(const QUrl &documentUrl, const QString &undoGroupId) {
    if (!documentUrl.isLocalFile()) return tr("Document delete failed: URL is not a local file.");

    const QString documentPath = documentUrl.toLocalFile();
    if (!QFileInfo(documentPath).isFile()) return tr("Document delete failed: document does not exist.");

    if (!undoGroupId.isEmpty()) {
        const auto error = _transactionFlush(undoGroupId, documentUrl);
        if (!error.isEmpty()) return error;
    }

    const auto trashUrl = QSharedPointer<QUrl>::create();
    const auto text = tr("Document Delete (%1)").arg(documentUrl.fileName());
    const auto redo = [this, documentUrl, trashUrl] { return _documentDelete(documentUrl, *trashUrl); };
    const auto undo = [this, documentUrl, trashUrl] { return _documentRestore(documentUrl, *trashUrl); };
    if (!undoGroupId.isEmpty()) return g_undo->push(text, redo, undo, undoGroupId);
    return g_undo->push(text, redo, undo, [this](const QString &error) { m_toast->show(ToastLevel::Error, tr("Document"), error); });
}

DocumentPage *DocumentModule::documentConstruct(const QUrl &documentUrl) {
    DocumentPage *documentPage{};
    const auto scheme = documentUrl.scheme().toLower();
    // web page
    if (scheme == "http" || scheme == "https") {
        documentPage = new WebPage(m_config, documentUrl);
        auto *webPage = qobject_cast<WebPage *>(documentPage);
        webPage->propertySet(QVariantHash{
        });
    }
    // conflict page
    else if (g_globalManager->gitEnabledGet()) {
        QProcess process{};
        process.setWorkingDirectory(g_workspaceUrl.toLocalFile());
        process.start("git", {"diff", "--name-only", "--diff-filter=U", "--", documentUrl.toLocalFile()});
        process.waitForFinished(300);
        if (process.exitCode() == 0 && !process.readAllStandardOutput().isEmpty()) {
            documentPage = new ConflictPage(m_config, documentUrl);
            auto *conflictPage = qobject_cast<ConflictPage *>(documentPage);
            conflictPage->propertySet(QVariantHash{
                {"theme", m_theme},
                {"mainWindowToast", QVariant::fromValue(m_toast)},
                {"mainWindowToolTip", QVariant::fromValue(m_toolTip)},
                {"fileModulePropertyDialog", QVariant::fromValue(m_systemPropertyDialog)},
                {"documentModuleGotoDialog", QVariant::fromValue(m_gotoDialog)}
            });
            connect(conflictPage, &ConflictPage::isFocusedChanged, this, [this, conflictPage](const bool status) { documentFocus(conflictPage, status); });
            connect(conflictPage, &ConflictPage::appendLog, this, &DocumentModule::appendLog);
            connect(conflictPage, &ConflictPage::changeSelection, this, &DocumentModule::changeSelection);
            connect(conflictPage, &ConflictPage::reloadDocument, this, &DocumentModule::documentReload);
        }
    }
    // normal page
    if (!documentPage) {
        const auto documentPath = documentUrl.toLocalFile();
        const QFileInfo documentInfo(documentPath);
        const auto suffix = documentInfo.suffix().toLower();
        const QStringList codeType = {"lua"};
        // image page
        if (QImageReader::supportedImageFormats().contains(suffix)) {
            documentPage = new ImagePage(m_config, documentUrl);
            auto *imagePage = qobject_cast<ImagePage *>(documentPage);
            imagePage->propertySet(QVariantHash{
            });
        }
        // code page
        else if (codeType.contains(suffix)) {
            documentPage = new CodePage(m_config, documentUrl);
            auto *codePage = qobject_cast<CodePage *>(documentPage);
            codePage->propertySet(QVariantHash{
                {"theme", m_theme},
                {"mainWindowToast", QVariant::fromValue(m_toast)},
                {"mainWindowToolTip", QVariant::fromValue(m_toolTip)},
                {"breakpointModuleEditDialog", QVariant::fromValue(m_breakpointEditDialog)},
                {"fileModulePropertyDialog", QVariant::fromValue(m_systemPropertyDialog)},
                {"documentModuleGotoDialog", QVariant::fromValue(m_gotoDialog)},
                {"documentModuleEditorMenu", QVariant::fromValue(m_editorMenu)}
            });
            connect(codePage, &CodePage::isFocusedChanged, this, [this, codePage](const bool status) { documentFocus(codePage, status); });
            connect(codePage, &CodePage::appendLog, this, &DocumentModule::appendLog);
            connect(codePage, &CodePage::startThread, this, &DocumentModule::startThread);
            connect(codePage, &CodePage::changeSelection, this, &DocumentModule::changeSelection);
            connect(codePage, &CodePage::insertBreakpoint, this, &DocumentModule::insertBreakpoint);
            connect(codePage, &CodePage::removeBreakpoint, this, &DocumentModule::removeBreakpoint);
            connect(codePage, &CodePage::notificationJson, this, &DocumentModule::notificationJson);
            connect(codePage, &CodePage::requestCompletion, this, &DocumentModule::completionRequest);
            connect(codePage, &CodePage::requestDefinition, this, &DocumentModule::definitionRequest);
            connect(codePage, &CodePage::requestDocumentHighlight, this, &DocumentModule::documentHighlightRequest);
            connect(codePage, &CodePage::requestDocumentSymbol, this, &DocumentModule::documentSymbolRequest);
            connect(codePage, &CodePage::requestFoldingRange, this, &DocumentModule::foldingRangeRequest);
            connect(codePage, &CodePage::requestFormatting, this, &DocumentModule::formattingRequest);
            connect(codePage, &CodePage::requestHover, this, &DocumentModule::hoverRequest);
            connect(codePage, &CodePage::requestImplementation, this, &DocumentModule::implementationRequest);
            connect(codePage, &CodePage::requestOnTypeFormatting, this, &DocumentModule::onTypeFormattingRequest);
            connect(codePage, &CodePage::requestPrepareRename, this, &DocumentModule::prepareRenameRequest);
            connect(codePage, &CodePage::requestReferences, this, &DocumentModule::referencesRequest);
            connect(codePage, &CodePage::requestSemanticTokens, this, &DocumentModule::semanticTokensRequest);
            connect(codePage, &CodePage::requestSignatureHelp, this, &DocumentModule::signatureHelpRequest);
            connect(codePage, &CodePage::requestSpellCheck, this, &DocumentModule::requestSpellCheck);
            connect(codePage, &CodePage::requestTypeDefinition, this, &DocumentModule::typeDefinitionRequest);
            connect(codePage, &CodePage::showDiagnostic, m_codeAssistant, &CodeAssistant::diagnosticShow);
            codePage->diagnosticsNotification(m_diagnosticsHash[documentUrl]);
        }
        // markup page
        else if (suffix == "md" || suffix == "html") {
            documentPage = new MarkupPage(m_config, documentUrl);
            auto *markupPage = qobject_cast<MarkupPage *>(documentPage);
            markupPage->propertySet(QVariantHash{
                {"theme", m_theme},
                {"mainWindowToast", QVariant::fromValue(m_toast)},
                {"mainWindowToolTip", QVariant::fromValue(m_toolTip)},
                {"fileModulePropertyDialog", QVariant::fromValue(m_systemPropertyDialog)},
                {"documentModuleGotoDialog", QVariant::fromValue(m_gotoDialog)}
            });
            connect(markupPage, &MarkupPage::isFocusedChanged, this, [this, markupPage](const bool status) { documentFocus(markupPage, status); });
            connect(markupPage, &MarkupPage::appendLog, this, &DocumentModule::appendLog);
            connect(markupPage, &MarkupPage::changeSelection, this, &DocumentModule::changeSelection);
        }
        // pdf page
        else if (suffix == "pdf") {
            documentPage = new PdfPage(m_config, documentUrl);
            auto *pdfPage = qobject_cast<PdfPage *>(documentPage);
            pdfPage->propertySet(QVariantHash{
            });
        }
        // text page
        else {
            documentPage = new TextPage(m_config, documentUrl);
            auto *textPage = qobject_cast<TextPage *>(documentPage);
            textPage->propertySet(QVariantHash{
                {"theme", m_theme},
                {"mainWindowToast", QVariant::fromValue(m_toast)},
                {"mainWindowToolTip", QVariant::fromValue(m_toolTip)},
                {"fileModulePropertyDialog", QVariant::fromValue(m_systemPropertyDialog)},
                {"documentModuleGotoDialog", QVariant::fromValue(m_gotoDialog)}
            });
            connect(textPage, &TextPage::isFocusedChanged, this, [this, textPage](const bool status) { documentFocus(textPage, status); });
            connect(textPage, &TextPage::appendLog, this, &DocumentModule::appendLog);
            connect(textPage, &TextPage::changeSelection, this, &DocumentModule::changeSelection);
        }
    }
    // path disambiguation
    if (scheme == "file") {
        bool conflict = false;
        for (const auto &url: m_pageHash.keys()) {
            if (url.fileName() == documentUrl.fileName()) {
                conflict = true;
                auto *page = m_pageHash[url];
                page->pathDisambiguation();
            }
        }
        if (conflict) documentPage->pathDisambiguation();
        m_watcher->addPath(documentUrl.toLocalFile());
    }
    m_pageHash[documentUrl] = documentPage;
    connect(documentPage, &DocumentPage::closeRequest, this, [this, documentUrl] {
        m_saveDialog->setProperty("documentUrl", documentUrl.toString());
        m_saveDialog->setProperty("documentName", documentUrl.fileName());
        QMetaObject::invokeMethod(m_saveDialog, "open");
    });
    connect(documentPage, &DocumentPage::closeDocument, this, qOverload<const QUrl &>(&DocumentModule::documentClose));
    return documentPage;
}

void DocumentModule::documentOpen(const QUrl &documentUrl) {
    // open page
    if (!m_pageHash.contains(documentUrl)) {
        const auto documentPage = documentConstruct(documentUrl);
        if (m_focusedUrl.isEmpty()) {
            m_welcomePage->open();
            m_welcomePage->addDockWidgetAsTab(documentPage);
            m_welcomePage->close();
        } else {
            m_pageHash[m_focusedUrl]->addDockWidgetAsTab(documentPage);
        }
        emit appendLog(LogLevel::Info, "document opened", QString("<a href='%1'>%2</a>").arg(documentUrl.toString(), documentUrl.toString()));

        if (auto *handler = handlerGet(documentUrl)) {
            const auto text = handler->textGet();
            for (const auto &transaction: m_transactions) {
                if (!transaction->documents.contains(documentUrl)) {
                    transaction->documents.insert(documentUrl, DocumentTextState{.before = text, .after = text, .dirty = handler->modifyGet()});
                }
            }
        }
    }
    m_pageHash[documentUrl]->raise();
    m_pageHash[documentUrl]->setFocus(Qt::FocusReason::MouseFocusReason);
}

void DocumentModule::documentGoto(const QUrl &documentUrl) const {
    const auto *handler = handlerGet(documentUrl);
    if (handler == nullptr) return;
    const auto index = handler->cast<ScintillaWidget::Utf16Index>(handler->positionGet());
    m_gotoDialog->setProperty("documentUrl", documentUrl);
    m_gotoDialog->setProperty("line", index.line);
    m_gotoDialog->setProperty("character", index.character);
    QMetaObject::invokeMethod(m_gotoDialog, "open");
}

QString DocumentModule::documentSave(const QUrl &documentUrl) const {
    auto *documentPage = m_pageHash.value(documentUrl, nullptr);
    if (documentPage == nullptr) return tr("Document save failed: document is not open.");

    const auto documentPath = documentUrl.toLocalFile();
    const bool watched = m_watcher->files().contains(documentPath);
    if (watched) m_watcher->removePath(documentPath);

    const auto error = documentPage->documentSave();
    if (watched) m_watcher->addPath(documentPath);
    return error;
}

void DocumentModule::documentClose(const QUrl &documentUrl, const bool save) const {
    auto *documentPage = m_pageHash.value(documentUrl);
    if (save) {
        const auto error = documentSave(documentUrl);
        if (!error.isEmpty()) {
            m_toast->show(ToastLevel::Error, tr("Document save failed"), error);
            return;
        }
    }
    documentPage->closeApprove();
}

void DocumentModule::documentReload(const QString &documentPath) {
    const auto documentUrl = QUrl::fromLocalFile(documentPath);
    if (m_pageHash.contains(documentUrl)) {
        auto *documentPage = m_pageHash.value(documentUrl);
        connect(documentPage, &DocumentPage::destroyed, this, [this, documentUrl, documentPath] {
            if (QFileInfo::exists(documentPath)) documentOpen(documentUrl);
        });
        documentPage->closeApprove();
    }
}

void DocumentModule::permissionSet(const QUrl &documentUrl) const {
    if (m_pageHash.contains(documentUrl)) m_pageHash.value(documentUrl)->permissionGet();
}

QVariantHash DocumentModule::menuLoad(const QString &name) {
    // TODO: text page
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(m_focusedUrl))) {
        if (name == "nav") {
            auto menuSession = codePage->menuLoad(name);
            menuSession.insert("prev", m_navigationHistory["index"].toInt() > 0);
            menuSession.insert("next", m_navigationHistory["index"].toInt() < m_navigationHistory["list"].toList().size() - 1);
            // qDebug() << menuSession["documentUrl"] << menuSession["line"] << menuSession["character"] << menuSession["navigation"];
            return menuSession;
        }
        return codePage->menuLoad(name);
    }
    return {};
}

void DocumentModule::menuCall(const QString &name) const {
    // TODO: text page
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(m_focusedUrl))) codePage->menuCall(name);
}

int DocumentModule::eolModeGet(const QUrl &documentUrl) const {
    if (const auto *handler = handlerGet(documentUrl)) return handler->eolModeGet();
    return {};
}

void DocumentModule::eolModeSet(const QUrl &documentUrl, const int eolMode) const {
    if (const auto *handler = handlerGet(documentUrl)) handler->eolModeSet(eolMode);
}

bool DocumentModule::eolViewGet(const QUrl &documentUrl) const {
    if (const auto *handler = handlerGet(documentUrl)) return handler->eolViewGet();
    return {};
}

void DocumentModule::eolViewSet(const QUrl &documentUrl, const bool status) const {
    if (const auto *handler = handlerGet(documentUrl)) handler->eolViewSet(status);
}

// public: document
void DocumentModule::foldContractTop(const QUrl &documentUrl) {
    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    if (const auto *handler = handlerGet(documentUrl)) handler->foldContractTop();
}

void DocumentModule::foldContractRecursively(const QUrl &documentUrl) {
    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    if (const auto *handler = handlerGet(documentUrl)) handler->foldContractRecursively();
}

void DocumentModule::foldExpandRecursively(const QUrl &documentUrl) {
    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    if (const auto *handler = handlerGet(documentUrl)) handler->foldExpandRecursively();
}

void DocumentModule::navigationPrev() {
    const auto index = m_navigationHistory["index"].toInt() - 1;
    if (index < 0) return;
    m_navigationHistory["index"] = index;
    const QVariantHash navigationSession = m_navigationHistory["list"].toList()[index].toHash();
    indexSet(navigationSession["documentUrl"].toUrl(), navigationSession["line"].toInt(), navigationSession["character"].toInt());
}

void DocumentModule::navigationNext() {
    const auto index = m_navigationHistory["index"].toInt() + 1;
    if (index >= m_navigationHistory["list"].toList().size()) return;
    m_navigationHistory["index"] = index;
    const QVariantHash navigationSession = m_navigationHistory["list"].toList()[index].toHash();
    indexSet(navigationSession["documentUrl"].toUrl(), navigationSession["line"].toInt(), navigationSession["character"].toInt());
}

void DocumentModule::focusSet(const QUrl &documentUrl, const bool status) {
    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    if (const auto *handler = handlerGet(documentUrl)) handler->focusSet(status);
}

void DocumentModule::indexSet(const QUrl &documentUrl, const int line, const int character) {
    if (documentUrl != m_focusedUrl) documentOpen(documentUrl);
    if (const auto *handler = handlerGet(documentUrl)) handler->indexSet(line, character);
}

void DocumentModule::indexGet() const {
    const auto *handler = handlerGet(m_focusedUrl);
    if (handler == nullptr) return;
    const auto index = handler->cast<ScintillaWidget::Utf16Index>(handler->positionGet());
    g_cursorPosition = {
        {"url", m_focusedUrl},
        {"line", index.line},
        {"character", index.character}
    };
}

void DocumentModule::transactionBegin(const QString &undoGroupId) {
    auto transaction = QSharedPointer<DocumentTransaction>::create();
    for (auto page = m_pageHash.cbegin(); page != m_pageHash.cend(); ++page) {
        auto *handler = handlerGet(page.key());
        if (handler == nullptr) continue;
        const auto text = handler->textGet();
        transaction->documents.insert(page.key(), DocumentTextState{
                                          .before = text,
                                          .after = text,
                                          .dirty = handler->modifyGet()
                                      });
    }
    m_transactions.insert(undoGroupId, transaction);
}

QString DocumentModule::transactionCommit(const QString &undoGroupId) {
    const auto error = _transactionFlush(undoGroupId);
    m_transactions.remove(undoGroupId);
    return error;
}

QString DocumentModule::linesGet(const QUrl &documentUrl, const int startLine, const int lineCount) const {
    if (const auto *handler = handlerGet(documentUrl)) return handler->linesGet(startLine, lineCount);
    return FileModule::linesGet(documentUrl, startLine, lineCount);
}

QString DocumentModule::linesSet(const QUrl &documentUrl, const QStringList &texts, const QList<int> &startLines, const QList<int> &lineCounts,
                                 const QString &undoGroupId) {
    if (texts.size() != startLines.size() || texts.size() != lineCounts.size()) return tr("Line set failed: edit lists have different sizes.");
    if (texts.isEmpty()) return {};

    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    auto *handler = handlerGet(documentUrl);
    if (handler == nullptr) return tr("Line set failed: document is not editable text.");

    auto transaction = m_transactions.value(undoGroupId);
    if (transaction.isNull()) {
        transaction = QSharedPointer<DocumentTransaction>::create();
        m_transactions.insert(undoGroupId, transaction);
    }

    auto state = transaction->documents.find(documentUrl);
    if (state == transaction->documents.end()) {
        state = transaction->documents.insert(documentUrl, DocumentTextState{
                                                  .before = handler->textGet(),
                                                  .after = handler->textGet(),
                                                  .dirty = handler->modifyGet()
                                              });
    } else if (handler->textGet() != state->after) {
        const auto error = tr("Line set failed: document content has changed outside the undo group.");
        g_undo->undoGroupInvalidate(undoGroupId, error);
        return error;
    }

    if (state->dirty) {
        const auto error = documentSave(documentUrl);
        if (!error.isEmpty()) return error;
        state->dirty = false;
    }

    const auto lineCount = [](QString text) {
        text.replace("\r\n", "\n");
        text.replace('\r', '\n');
        if (text.isEmpty()) return 0;
        return static_cast<int>(text.count('\n')) + (text.endsWith('\n') ? 0 : 1);
    };
    int additions{};
    int deletions{};
    for (qsizetype index = 0; index < texts.size(); ++index) {
        additions += lineCount(texts.at(index));
        deletions += lineCount(handler->linesGet(startLines.at(index), lineCounts.at(index)));
    }

    const auto error = _linesSet(documentUrl, texts, startLines, lineCounts);
    if (!error.isEmpty()) return error;

    const auto saveError = documentSave(documentUrl);
    if (!saveError.isEmpty()) {
        handler->textSet(state->after);
        handler->savepointSet();
        return saveError;
    }

    state->after = handler->textGet();
    auto &diff = transaction->diffs[documentUrl];
    diff.additions += additions;
    diff.deletions += deletions;

    QVariantMap fileDiffs{};
    int totalAdditions{};
    int totalDeletions{};
    for (auto document = transaction->diffs.cbegin(); document != transaction->diffs.cend(); ++document) {
        fileDiffs.insert(document.key().toString(), QVariantHash{
                             {"path", QDir(g_workspaceUrl.toLocalFile()).relativeFilePath(document.key().toLocalFile())},
                             {"additions", document->additions},
                             {"deletions", document->deletions}
                         });
        totalAdditions += document->additions;
        totalDeletions += document->deletions;
    }
    emit updateDiff(undoGroupId, fileDiffs, totalAdditions, totalDeletions);
    return {};
}

QString DocumentModule::textGet(const QUrl &documentUrl, const int startLine, const int startCharacter, const int endLine, const int endCharacter) const {
    if (const auto *handler = handlerGet(documentUrl)) return handler->textGet(startLine, startCharacter, endLine, endCharacter);
    if (const auto *pdfPage = qobject_cast<PdfPage *>(m_pageHash.value(documentUrl))) return pdfPage->textGet(startLine);
    return FileModule::textGet(documentUrl, startLine, startCharacter, endLine, endCharacter);
}

void DocumentModule::textSet(const QUrl &documentUrl, const QString &text, const int startLine, const int startCharacter, const int endLine, const int endCharacter) {
    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    if (const auto *handler = handlerGet(documentUrl)) handler->textSet(text, startLine, startCharacter, endLine, endCharacter);
}

void DocumentModule::indicatorFill(const QUrl &documentUrl, const int type, const int startLine, const int startCharacter, const int endLine, const int endCharacter,
                                   const int time) {
    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    if (const auto *handler = handlerGet(documentUrl)) handler->indicatorFill(type, startLine, startCharacter, endLine, endCharacter, time);
}

void DocumentModule::indicatorClear(const QUrl &documentUrl, const int type, const int startLine, const int startCharacter, const int endLine, const int endCharacter) const {
    if (!m_pageHash.contains(documentUrl)) return;
    if (const auto *handler = handlerGet(documentUrl)) handler->indicatorClear(type, startLine, startCharacter, endLine, endCharacter);
}

void DocumentModule::markerAdd(const QUrl &documentUrl, const int type, const int line, const int time) {
    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    if (const auto *handler = handlerGet(documentUrl)) handler->markerAdd(type, line, time);
}

void DocumentModule::markerDelete(const QUrl &documentUrl, const int type, const int line) const {
    if (!m_pageHash.contains(documentUrl)) return;
    if (const auto *handler = handlerGet(documentUrl)) handler->markerDelete(type, line);
}

QJsonArray DocumentModule::diagnosticsGet(const QUrl &documentUrl) const {
    return m_diagnosticsHash.value(documentUrl);
}

// public: lsp
void DocumentModule::diagnosticsNotification(const QUrl &documentUrl, const QJsonArray &diagnostics) {
    m_diagnosticsHash.insert(documentUrl, diagnostics);
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) codePage->diagnosticsNotification(diagnostics);
}

void DocumentModule::codeActionRequest(const QUrl &documentUrl, const int startLine, const int startCharacter, const int endLine, const int endCharacter) {
    QJsonArray diagnosticArray{};
    for (const auto &value: m_diagnosticsHash[documentUrl]) {
        const QJsonObject diagnostic = value.toObject();
        const QJsonObject range = diagnostic["range"].toObject();
        const QJsonObject start = range["start"].toObject();
        const QJsonObject end = range["end"].toObject();
        if (startLine == start["line"].toInt() && startCharacter == start["character"].toInt() && endLine == end["line"].toInt() && endCharacter == end["character"].toInt()) {
            diagnosticArray.append(diagnostic);
        }
    }
    // code action request to lua language server
    const QJsonObject codeActionParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "range", QJsonObject{
                {
                    "start", QJsonObject{
                        {"line", startLine},
                        {"character", startCharacter}
                    }
                },
                {
                    "end", QJsonObject{
                        {"line", endLine},
                        {"character", endCharacter}
                    }
                }
            }
        },
        {
            "context", QJsonObject{
                {"diagnostics", diagnosticArray}
            }
        }
    };
    emit requestJson("textDocument/codeAction", codeActionParams);
}

// codeActionResponse is sent to codeAssistant module

void DocumentModule::completionRequest(const QUrl &documentUrl, int line, int character) {
    // completion request to lua language server
    const QJsonObject completionParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        }
    };
    emit requestJson("textDocument/completion", completionParams);
}

void DocumentModule::completionResponse(const QUrl &documentUrl, const QJsonArray &items) const {
    if (const auto *scintilla = handlerGet(documentUrl)) {
        const auto wordRange = scintilla->wordIndexGet();
        const auto startLine = wordRange.start.line;
        const auto startCharacter = wordRange.start.character;
        const auto endLine = wordRange.end.line;
        const auto endCharacter = wordRange.end.character;
        const auto index = scintilla->cast<ScintillaWidget::Utf16Index>(scintilla->positionGet());
        const auto typed = index.character - startCharacter;
        const auto height = scintilla->heightGet();
        auto position = scintilla->cast<ScintillaWidget::GlobalPoint>(ScintillaWidget::Utf16Index{startLine, startCharacter}).value;
        position.ry() += height;
        // call completion show
        const QVariantHash completionSession = {
            {"documentUrl", documentUrl},
            {"position", position},
            {"startLine", startLine},
            {"startCharacter", startCharacter},
            {"endLine", endLine},
            {"endCharacter", endCharacter},
            {"typed", typed}
        };
        m_codeAssistant->completionShow(completionSession, items);
    }
}

void DocumentModule::definitionRequest(const QUrl &documentUrl, const int line, const int character) {
    // definition request to lua language server
    const QJsonObject definitionParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        }
    };
    emit requestJson("textDocument/definition", definitionParams);
}

void DocumentModule::definitionResponse(const QUrl &documentUrl, const QJsonArray &definitions) const {
    if (const auto *scintilla = handlerGet(documentUrl)) {
        const auto wordRange = scintilla->wordIndexGet();
        const auto startLine = wordRange.start.line;
        const auto startCharacter = wordRange.start.character;
        const auto height = scintilla->heightGet();
        auto position = scintilla->cast<ScintillaWidget::GlobalPoint>(ScintillaWidget::Utf16Index{startLine, startCharacter}).value;
        position.ry() += height;
        // call navigation show
        const QVariantHash navigationSession = {
            {"type", "definition"},
            {"documentUrl", documentUrl},
            {"position", position}
        };
        m_codeAssistant->navigationShow(navigationSession, definitions);
    }
}

void DocumentModule::documentSymbolRequest(const QUrl &documentUrl) {
    // document symbol request to lua language server
    const QJsonObject documentSymbolParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        }
    };
    emit requestJson("textDocument/documentSymbol", documentSymbolParams);
}

void DocumentModule::documentSymbolResponse(const QUrl &documentUrl, const QJsonArray &result) {
    m_symbolHash.insert(documentUrl, result);
    if (auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) codePage->documentSymbolResponse(result);
}

void DocumentModule::documentHighlightRequest(const QUrl &documentUrl, const int line, const int character) {
    // document highlight request to lua language server
    const QJsonObject documentHighlightParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        }
    };
    emit requestJson("textDocument/documentHighlight", documentHighlightParams);
}

void DocumentModule::documentHighlightResponse(const QUrl &documentUrl, const QJsonArray &result) const {
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) codePage->documentHighlightResponse(result);
}

void DocumentModule::foldingRangeRequest(const QUrl &documentUrl) {
    // folding range request to lua language server
    const QJsonObject foldingRangeParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        }
    };
    emit requestJson("textDocument/foldingRange", foldingRangeParams);
}

void DocumentModule::foldingRangeResponse(const QUrl &documentUrl, const QJsonArray &result) const {
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) codePage->foldingRangeResponse(result);
}

void DocumentModule::formattingRequest(const QUrl &documentUrl) {
    // formatting request to lua language server
    const QJsonObject formattingParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "options", QJsonObject{
                {"tabSize", 4},
                {"insertSpaces", true},
                {"trimTrailingWhitespace", true},
                {"insertFinalNewline", true}
            }
        }
    };
    emit requestJson("textDocument/formatting", formattingParams);
}

void DocumentModule::formattingResponse(const QUrl &documentUrl, const QString &newText) const {
    if (newText.isEmpty()) m_toast->show(ToastLevel::Info, tr("Format"), tr("File is already formatted."));
    else if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) codePage->formattingResponse(newText);
}

void DocumentModule::hoverRequest(const QUrl &documentUrl, int line, int character) {
    // hover request to lua language server
    const QJsonObject hoverParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        }
    };
    emit requestJson("textDocument/hover", hoverParams);
}

void DocumentModule::hoverResponse(const QUrl &documentUrl, const QString &message) const {
    // call hover show
    const QVariantHash hoverSession = {
        {"position", QCursor::pos()}
    };
    m_codeAssistant->hoverShow(hoverSession, message);
}

void DocumentModule::implementationRequest(const QUrl &documentUrl, const int line, const int character) {
    // implementation request to lua language server
    const QJsonObject implementationParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        }
    };
    emit requestJson("textDocument/implementation", implementationParams);
}

void DocumentModule::implementationResponse(const QUrl &documentUrl, const QJsonArray &implementations) const {
    if (const auto *scintilla = handlerGet(documentUrl)) {
        const auto wordRange = scintilla->wordIndexGet();
        const auto startLine = wordRange.start.line;
        const auto startCharacter = wordRange.start.character;
        const auto height = scintilla->heightGet();
        auto position = scintilla->cast<ScintillaWidget::GlobalPoint>(ScintillaWidget::Utf16Index{startLine, startCharacter}).value;
        position.ry() += height;
        // call navigation show
        const QVariantHash navigationSession = {
            {"type", "implementation"},
            {"documentUrl", documentUrl},
            {"position", position}
        };
        m_codeAssistant->navigationShow(navigationSession, implementations);
    }
}

void DocumentModule::onTypeFormattingRequest(const QUrl &documentUrl, int line, int character) {
    // on type formatting request to lua language server
    const QJsonObject onTypeFormattingParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        },
        {"ch", "\n"},
        {
            "options", QJsonObject{
                {"tabSize", 4},
                {"insertSpaces", true},
                {"trimTrailingWhitespace", true},
                {"insertFinalNewline", true}
            }
        }
    };
    emit requestJson("textDocument/onTypeFormatting", onTypeFormattingParams);
}

void DocumentModule::onTypeFormattingResponse(const QUrl &documentUrl, const QJsonObject &newText) const {
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) codePage->onTypeFormattingResponse(newText);
}

void DocumentModule::prepareRenameRequest(const QUrl &documentUrl, const int line, const int character) {
    m_renameDialog->setProperty("documentUrl", documentUrl);
    m_renameDialog->setProperty("line", line);
    m_renameDialog->setProperty("character", character);

    const QJsonObject prepareRenameParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        }
    };
    emit requestJson("textDocument/prepareRename", prepareRenameParams);
}

void DocumentModule::prepareRenameResponse(const QUrl &documentUrl, const QString &oldName) const {
    m_renameDialog->setProperty("oldName", oldName);
    QMetaObject::invokeMethod(m_renameDialog, "open");
}

void DocumentModule::rangeFormattingRequest(const QUrl &documentUrl, const int startLine, const int startCharacter, const int endLine, const int endCharacter) {
    // rangeFormatting request to lua language server
    const QJsonObject rangeFormattingParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "range", QJsonObject{
                {
                    "start", QJsonObject{
                        {"line", startLine},
                        {"character", startCharacter}
                    }
                },
                {
                    "end", QJsonObject{
                        {"line", endLine},
                        {"character", endCharacter}
                    }
                }
            }
        },
        {
            "options", QJsonObject{
                {"tabSize", 4},
                {"insertSpaces", true},
                {"trimTrailingWhitespace", true},
                {"insertFinalNewline", true}
            }
        }
    };
    emit requestJson("textDocument/rangeFormatting", rangeFormattingParams);
}

void DocumentModule::rangeFormattingResponse(const QUrl &documentUrl, const QString &newText) const {
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) {
        auto text = newText;
        if (text.endsWith("\r\n")) text.chop(2);
        codePage->rangeFormattingResponse(text);
    }
}

void DocumentModule::referencesRequest(const QUrl &documentUrl, int line, int character) {
    // references request to lua language server
    const QJsonObject referencesParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        },
        {
            "context", QJsonObject{
                {"includeDeclaration", false}
            }
        }
    };
    emit requestJson("textDocument/references", referencesParams);
}

void DocumentModule::referencesResponse(const QUrl &documentUrl, const QJsonArray &references) const {
    if (const auto *scintilla = handlerGet(documentUrl)) {
        const auto wordRange = scintilla->wordIndexGet();
        const auto startLine = wordRange.start.line;
        const auto startCharacter = wordRange.start.character;
        const auto height = scintilla->heightGet();
        auto position = scintilla->cast<ScintillaWidget::GlobalPoint>(ScintillaWidget::Utf16Index{startLine, startCharacter}).value;
        position.ry() += height;
        // call navigation show
        const QVariantHash navigationSession = {
            {"type", "reference"},
            {"documentUrl", documentUrl},
            {"position", position}
        };
        m_codeAssistant->navigationShow(navigationSession, references);
    }
}

void DocumentModule::renameRequest(const QUrl &documentUrl, const int line, const int character, const QString &newName) {
    const QJsonObject renameParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        },
        {"newName", newName}
    };
    emit requestJson("textDocument/rename", renameParams);
}

void DocumentModule::renameResponse(const QUrl &documentUrl, const QJsonObject &workspaceEdit) {
    // TODO: document changes not supported yet
    for (const auto &value: workspaceEdit["documentChanges"].toArray()) {
        const auto change = value.toObject();
        if (change.contains("kind")) {
            qDebug() << "lsp rename: unsupported file operation";
            return;
        }
    }

    QSet<QUrl> documentUrls{};
    const auto changes = workspaceEdit["changes"].toObject();
    for (auto change = changes.constBegin(); change != changes.constEnd(); ++change) {
        documentUrls.insert(uni_cast<QUrl>(LUrl(change.key())));
    }

    if (documentUrls.isEmpty()) {
        qDebug() << "lsp rename: no changes";
    } else if (documentUrls.size() == 1) {
        if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) codePage->renameResponse(changes.constBegin().value().toArray());
    } else {
        const auto undoGroupId = g_undo->undoGroupBegin();
        for (auto change = changes.constBegin(); change != changes.constEnd(); ++change) {
            documentOpen(uni_cast<QUrl>(LUrl(change.key())));
        }
        transactionBegin(undoGroupId);

        const auto transaction = m_transactions.value(undoGroupId);
        for (auto change = changes.constBegin(); change != changes.constEnd(); ++change) {
            const auto targetUrl = uni_cast<QUrl>(LUrl(change.key()));
            const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(targetUrl));
            codePage->renameResponse(change.value().toArray());
            transaction->documents[targetUrl].after = handlerGet(targetUrl)->textGet();
        }

        const auto transactionError = transactionCommit(undoGroupId);
        if (!transactionError.isEmpty()) m_toast->show(ToastLevel::Error, tr("Rename"), transactionError);
        const auto commitError = g_undo->undoGroupCommit(
            undoGroupId,
            tr("LSP Rename"),
            [this](const QString &error) { m_toast->show(ToastLevel::Error, tr("Rename"), error); });
        if (!commitError.isEmpty()) m_toast->show(ToastLevel::Error, tr("Rename"), commitError);
        g_undo->undoGroupRelease(undoGroupId);
    }
}

void DocumentModule::semanticTokensRequest(const QUrl &documentUrl) {
    // semantic tokens request to lua language server
    const QJsonObject semanticTokensParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        }
    };
    emit requestJson("textDocument/semanticTokens/full", semanticTokensParams);
}

void DocumentModule::semanticTokensResponse(const QUrl &documentUrl, const QJsonArray &data) const {
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) codePage->semanticTokensResponse(data);
}

void DocumentModule::signatureHelpRequest(const QUrl &documentUrl, int line, int character) {
    // signature help request to lua language server
    const QJsonObject signatureHelpParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        }
    };
    emit requestJson("textDocument/signatureHelp", signatureHelpParams);
}

void DocumentModule::signatureHelpResponse(const QUrl &documentUrl, const QJsonArray &signatures) const {
    if (const auto *scintilla = handlerGet(documentUrl)) {
        const auto wordRange = scintilla->wordIndexGet();
        const auto startLine = wordRange.start.line;
        const auto startCharacter = wordRange.start.character;
        const auto position = scintilla->cast<ScintillaWidget::GlobalPoint>(ScintillaWidget::Utf16Index{startLine, startCharacter - 1}).value;
        // call signature show
        const QVariantHash signatureSession = {
            {"documentUrl", documentUrl},
            {"position", position}
        };
        m_codeAssistant->signatureShow(signatureSession, signatures);
    }
}

void DocumentModule::typeDefinitionRequest(const QUrl &documentUrl, const int line, const int character) {
    // type definition request to lua language server
    const QJsonObject typeDefinitionParams{
        {
            "textDocument", QJsonObject{
                {"uri", documentUrl.toString()}
            }
        },
        {
            "position", QJsonObject{
                {"line", line},
                {"character", character}
            }
        }
    };
    emit requestJson("textDocument/typeDefinition", typeDefinitionParams);
}

void DocumentModule::typeDefinitionResponse(const QUrl &documentUrl, const QJsonArray &typeDefinitions) const {
    if (const auto *scintilla = handlerGet(documentUrl)) {
        const auto wordRange = scintilla->wordIndexGet();
        const auto startLine = wordRange.start.line;
        const auto startCharacter = wordRange.start.character;
        const auto height = scintilla->heightGet();
        auto position = scintilla->cast<ScintillaWidget::GlobalPoint>(ScintillaWidget::Utf16Index{startLine, startCharacter}).value;
        position.ry() += height;
        // call navigation show
        const QVariantHash navigationSession = {
            {"type", "typeDefinition"},
            {"documentUrl", documentUrl},
            {"position", position}
        };
        m_codeAssistant->navigationShow(navigationSession, typeDefinitions);
    }
}

// public: typo
void DocumentModule::spellCheckResponse(const QUrl &documentUrl, const QVariantList &typos) {
    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) codePage->spellCheckResponse(typos);
}

// private
QString DocumentModule::_directoryCreate(const QUrl &directoryUrl) {
    if (!QDir().mkpath(directoryUrl.toLocalFile())) return tr("Directory create failed: unable to create directory.");

    didCreateFilesNotification(directoryUrl);
    emit appendLog(LogLevel::Info, "directory created at", QString("<a href='%1'>%2</a>").arg(directoryUrl.toString(), directoryUrl.toString()));
    return {};
}

QString DocumentModule::_directoryRename(const QUrl &sourceUrl, const QUrl &targetUrl) {
    QFile directory(sourceUrl.toLocalFile());
    if (!directory.rename(targetUrl.toLocalFile())) return tr("Directory rename failed: %1").arg(directory.errorString());

    didRenameFilesNotification(sourceUrl, targetUrl);
    emit appendLog(LogLevel::Info, "directory renamed to", QString("<a href='%1'>%2</a>").arg(targetUrl.toString(), targetUrl.toString()));
    return {};
}

QString DocumentModule::_directoryDelete(const QUrl &directoryUrl, QUrl &trashUrl) {
    QString trashPath;
    if (!QFile::moveToTrash(directoryUrl.toLocalFile(), &trashPath)) return tr("Directory delete failed: unable to move directory to trash.");

    trashUrl = QUrl::fromLocalFile(trashPath);
    didDeleteFilesNotification(directoryUrl);
    emit appendLog(LogLevel::Info, "directory deleted", QString("<a href='%1'>%2</a>").arg(trashUrl.toString(), trashUrl.toString()));
    return {};
}

QString DocumentModule::_directoryRestore(const QUrl &directoryUrl, const QUrl &trashUrl) {
    if (!trashUrl.isLocalFile()) return tr("Directory restore failed: trash path is unavailable.");
    if (QFileInfo::exists(directoryUrl.toLocalFile())) return tr("Directory restore failed: path already exists.");

    QFile directory(trashUrl.toLocalFile());
    if (!directory.rename(directoryUrl.toLocalFile())) return tr("Directory restore failed: %1").arg(directory.errorString());

    didCreateFilesNotification(directoryUrl);
    emit appendLog(LogLevel::Info, "directory restored to", QString("<a href='%1'>%2</a>").arg(directoryUrl.toString(), directoryUrl.toString()));
    return {};
}

QString DocumentModule::_documentCreate(const QUrl &documentUrl) {
    QFile document(documentUrl.toLocalFile());
    if (!document.open(QIODevice::WriteOnly)) return tr("Document create failed: %1").arg(document.errorString());
    document.close();

    didCreateFilesNotification(documentUrl);
    documentOpen(documentUrl);
    emit appendLog(LogLevel::Info, "document created at", QString("<a href='%1'>%2</a>").arg(documentUrl.toString(), documentUrl.toString()));
    return {};
}

QString DocumentModule::_documentRename(const QUrl &sourceUrl, const QUrl &targetUrl) {
    QFile document(sourceUrl.toLocalFile());
    if (!document.rename(targetUrl.toLocalFile())) return tr("Document rename failed: %1").arg(document.errorString());

    didRenameFilesNotification(sourceUrl, targetUrl);
    emit appendLog(LogLevel::Info, "document renamed to", QString("<a href='%1'>%2</a>").arg(targetUrl.toString(), targetUrl.toString()));
    return {};
}

QString DocumentModule::_documentDelete(const QUrl &documentUrl, QUrl &trashUrl) {
    QString trashPath;
    if (!QFile::moveToTrash(documentUrl.toLocalFile(), &trashPath)) return tr("Document delete failed: unable to move document to trash.");

    trashUrl = QUrl::fromLocalFile(trashPath);
    didDeleteFilesNotification(documentUrl);
    emit appendLog(LogLevel::Info, "document deleted", QString("<a href='%1'>%2</a>").arg(trashUrl.toString(), trashUrl.toString()));
    return {};
}

QString DocumentModule::_documentRestore(const QUrl &documentUrl, const QUrl &trashUrl) {
    if (!trashUrl.isLocalFile()) return tr("Document restore failed: trash path is unavailable.");
    if (QFileInfo::exists(documentUrl.toLocalFile())) return tr("Document restore failed: path already exists.");

    QFile document(trashUrl.toLocalFile());
    if (!document.rename(documentUrl.toLocalFile())) return tr("Document restore failed: %1").arg(document.errorString());

    didCreateFilesNotification(documentUrl);
    emit appendLog(LogLevel::Info, "document restored to", QString("<a href='%1'>%2</a>").arg(documentUrl.toString(), documentUrl.toString()));
    return {};
}

QString DocumentModule::_linesSet(const QUrl &documentUrl, const QStringList &texts, const QList<int> &startLines, const QList<int> &lineCounts) const {
    auto *handler = handlerGet(documentUrl);
    if (handler == nullptr) return tr("Line set failed: document is not editable text.");
    handler->linesSet(texts, startLines, lineCounts);
    return {};
}

QString DocumentModule::_transactionRedo(const QSharedPointer<const DocumentTransaction> &transaction) {
    QHash<QUrl, ScintillaWidget *> handlers{};
    bool atBefore = true;
    bool atAfter = true;
    for (auto document = transaction->documents.cbegin(); document != transaction->documents.cend(); ++document) {
        if (!m_pageHash.contains(document.key())) documentOpen(document.key());
        auto *handler = handlerGet(document.key());
        if (handler == nullptr) return tr("Document redo failed: document is not editable text.");

        handlers.insert(document.key(), handler);
        const auto current = handler->textGet();
        atBefore = atBefore && current == document->before;
        atAfter = atAfter && current == document->after;
    }
    if (atAfter) return {};
    if (!atBefore) return tr("Document redo failed: document content has changed.");

    for (auto document = transaction->documents.cbegin(); document != transaction->documents.cend(); ++document) {
        handlers.value(document.key())->textSet(document->after);
        const auto error = documentSave(document.key());
        if (!error.isEmpty()) return error;
    }
    return {};
}

QString DocumentModule::_transactionUndo(const QSharedPointer<const DocumentTransaction> &transaction) {
    QHash<QUrl, ScintillaWidget *> handlers{};
    bool atBefore = true;
    bool atAfter = true;
    for (auto document = transaction->documents.cbegin(); document != transaction->documents.cend(); ++document) {
        if (!m_pageHash.contains(document.key())) documentOpen(document.key());
        auto *handler = handlerGet(document.key());
        if (handler == nullptr) return tr("Document undo failed: document is not editable text.");

        handlers.insert(document.key(), handler);
        const auto current = handler->textGet();
        atBefore = atBefore && current == document->before;
        atAfter = atAfter && current == document->after;
    }
    if (atBefore) return {};
    if (!atAfter) return tr("Document undo failed: document content has changed.");

    for (auto document = transaction->documents.cbegin(); document != transaction->documents.cend(); ++document) {
        handlers.value(document.key())->textSet(document->before);
        const auto error = documentSave(document.key());
        if (!error.isEmpty()) return error;
    }
    return {};
}

QString DocumentModule::_transactionFlush(const QString &undoGroupId, const QUrl &documentUrl, const bool recursive) {
    const auto transaction = m_transactions.value(undoGroupId);
    if (transaction.isNull()) return {};

    if (!documentUrl.isEmpty()) {
        const auto error = _transactionCheck(undoGroupId, documentUrl, recursive);
        if (!error.isEmpty()) return error;
    }

    const auto matches = [&documentUrl, recursive](const QUrl &url) {
        if (documentUrl.isEmpty()) return true;
        if (url == documentUrl) return true;
        if (!recursive) return false;

        const auto relativePath = QDir(documentUrl.toLocalFile()).relativeFilePath(url.toLocalFile());
        return relativePath != QStringLiteral("..") && !relativePath.startsWith(QStringLiteral("../")) && !relativePath.startsWith(QStringLiteral("..\\")) &&
               !QDir::isAbsolutePath(relativePath);
    };

    auto frozen = QSharedPointer<DocumentTransaction>::create();
    for (auto document = transaction->documents.begin(); document != transaction->documents.end();) {
        if (!matches(document.key())) {
            ++document;
            continue;
        }
        if (document->before != document->after) frozen->documents.insert(document.key(), document.value());
        document = transaction->documents.erase(document);
    }
    if (frozen->documents.isEmpty()) return {};

    const QSharedPointer<const DocumentTransaction> transactionData = frozen;
    const auto error = g_undo->push(
        tr("Document Edit"),
        [this, transactionData] { return _transactionRedo(transactionData); },
        [this, transactionData] { return _transactionUndo(transactionData); },
        undoGroupId);
    if (error.isEmpty()) return {};

    for (auto document = frozen->documents.cbegin(); document != frozen->documents.cend(); ++document) {
        transaction->documents.insert(document.key(), document.value());
    }
    g_undo->undoGroupInvalidate(undoGroupId, error);
    return error;
}

QString DocumentModule::_transactionCheck(const QString &undoGroupId, const QUrl &documentUrl, const bool recursive) const {
    const auto transaction = m_transactions.value(undoGroupId);
    if (transaction.isNull()) return {};

    const auto matches = [&documentUrl, recursive](const QUrl &url) {
        if (url == documentUrl) return true;
        if (!recursive) return false;

        const auto relativePath = QDir(documentUrl.toLocalFile()).relativeFilePath(url.toLocalFile());
        return relativePath != QStringLiteral("..") && !relativePath.startsWith(QStringLiteral("../")) && !relativePath.startsWith(QStringLiteral("..\\")) &&
               !QDir::isAbsolutePath(relativePath);
    };
    for (auto document = transaction->documents.cbegin(); document != transaction->documents.cend(); ++document) {
        if (!matches(document.key())) continue;
        const auto *handler = handlerGet(document.key());
        if (handler != nullptr && handler->textGet() == document->after) continue;

        const auto error = tr("Document operation failed: document content has changed outside the undo group.");
        g_undo->undoGroupInvalidate(undoGroupId, error);
        return error;
    }
    return {};
}

void DocumentModule::_transactionRename(const QString &undoGroupId, const QUrl &sourceUrl, const QUrl &targetUrl, const bool recursive) {
    const auto transaction = m_transactions.value(undoGroupId);
    if (transaction.isNull()) return;

    const auto target = [&sourceUrl, &targetUrl, recursive](const QUrl &url) {
        if (url == sourceUrl) return targetUrl;
        if (!recursive) return QUrl{};

        const auto relativePath = QDir(sourceUrl.toLocalFile()).relativeFilePath(url.toLocalFile());
        if (relativePath == QStringLiteral("..") || relativePath.startsWith(QStringLiteral("../")) || relativePath.startsWith(QStringLiteral("..\\")) ||
            QDir::isAbsolutePath(relativePath))
            return QUrl{};
        return QUrl::fromLocalFile(QDir(targetUrl.toLocalFile()).filePath(relativePath));
    };

    QHash<QUrl, DocumentTextState> renamedDocuments{};
    for (auto document = transaction->documents.begin(); document != transaction->documents.end();) {
        const auto targetUrl = target(document.key());
        if (targetUrl.isEmpty()) {
            ++document;
            continue;
        }
        renamedDocuments.insert(targetUrl, document.value());
        document = transaction->documents.erase(document);
    }
    for (auto document = renamedDocuments.cbegin(); document != renamedDocuments.cend(); ++document) {
        transaction->documents.insert(document.key(), document.value());
    }

    QHash<QUrl, DocumentDiffState> renamedDiffs{};
    for (auto diff = transaction->diffs.begin(); diff != transaction->diffs.end();) {
        const auto targetUrl = target(diff.key());
        if (targetUrl.isEmpty()) {
            ++diff;
            continue;
        }
        renamedDiffs.insert(targetUrl, diff.value());
        diff = transaction->diffs.erase(diff);
    }
    for (auto diff = renamedDiffs.cbegin(); diff != renamedDiffs.cend(); ++diff) {
        auto &targetDiff = transaction->diffs[diff.key()];
        targetDiff.additions += diff->additions;
        targetDiff.deletions += diff->deletions;
    }

    QVariantMap fileDiffs{};
    int totalAdditions{};
    int totalDeletions{};
    for (auto diff = transaction->diffs.cbegin(); diff != transaction->diffs.cend(); ++diff) {
        fileDiffs.insert(diff.key().toString(), QVariantHash{
                             {"path", QDir(g_workspaceUrl.toLocalFile()).relativeFilePath(diff.key().toLocalFile())},
                             {"additions", diff->additions},
                             {"deletions", diff->deletions}
                         });
        totalAdditions += diff->additions;
        totalDeletions += diff->deletions;
    }
    emit updateDiff(undoGroupId, fileDiffs, totalAdditions, totalDeletions);
}

ScintillaWidget *DocumentModule::handlerGet(const QUrl &documentUrl) const {
    if (const auto *codePage = qobject_cast<CodePage *>(m_pageHash.value(documentUrl))) return codePage->handler();
    if (const auto *textPage = qobject_cast<TextPage *>(m_pageHash.value(documentUrl))) return textPage->handler();
    if (const auto *markupPage = qobject_cast<MarkupPage *>(m_pageHash.value(documentUrl))) return markupPage->handler();
    if (const auto *conflictPage = qobject_cast<ConflictPage *>(m_pageHash.value(documentUrl))) return conflictPage->handler();
    return nullptr;
}

void DocumentModule::documentFocus(DocumentPage *documentPage, const bool status) {
    auto *handler = handlerGet(documentPage->documentUrl());
    if (handler == nullptr) return;
    if (status) {
        handler->focusSet(true);
        m_focusedUrl = documentPage->documentUrl();
        const QVariantHash session = {
            {"codePage", handler->codePageGet()},
            {"eolMode", handler->eolModeGet()}
        };
        emit focusDocument(m_focusedUrl, session);
    } else if (qobject_cast<CodePage *>(documentPage)) {
        handler->indicatorClear(ScintillaIndicator::Highlight);
        handler->indicatorClear(ScintillaIndicator::Read);
        handler->indicatorClear(ScintillaIndicator::Write);
    }
}

void DocumentModule::documentClose(const QUrl &documentUrl) {
    if (documentUrl.isLocalFile()) m_watcher->removePath(documentUrl.toLocalFile());
    m_pageHash.remove(documentUrl);
    if (g_terminating) return;
    if (m_pageHash.isEmpty()) {
        m_welcomePage->open();
        m_focusedUrl = nullptr;
    } else {
        const auto begin = m_pageHash.begin();
        m_focusedUrl = begin.key();
    }
    emit appendLog(LogLevel::Info, "document closed", QString("<a href='%1'>%2</a>").arg(documentUrl.toString(), documentUrl.toString()));
}

void DocumentModule::charAdd(const QUrl &documentUrl, const QChar character) const {
    if (auto *handler = handlerGet(documentUrl)) emit handler->charAdded(character.toLatin1());
}

void DocumentModule::textSetSelected(const QUrl &documentUrl, const QString &text) {
    if (!m_pageHash.contains(documentUrl)) documentOpen(documentUrl);
    if (const auto *handler = handlerGet(documentUrl)) handler->textSetSelected(text);
}

void DocumentModule::navigationRecord(const QUrl &documentUrl, const int line, const int character) {
    // not at end, resize
    if (m_navigationHistory["index"].toInt() < m_navigationHistory["list"].toList().size() - 1) {
        QVariantList list = m_navigationHistory["list"].toList();
        list.resize(m_navigationHistory["index"].toInt() + 1);
        m_navigationHistory["list"] = list;
    }
    m_navigationHistory["index"] = m_navigationHistory["index"].toInt() + 1;
    // src index
    if (documentUrl.isEmpty()) {
        const auto *scintilla = qobject_cast<CodePage *>(m_pageHash[m_focusedUrl])->handler();
        const auto index = scintilla->cast<ScintillaWidget::Utf16Index>(scintilla->positionGet());
        const auto navigationSession = QVariantHash{
            {"documentUrl", m_focusedUrl},
            {"line", index.line},
            {"character", index.character}
        };
        QVariantList list = m_navigationHistory["list"].toList();
        list.append(navigationSession);
        m_navigationHistory["list"] = list;
    }
    // dst index
    else {
        const auto navigationSession = QVariantHash{
            {"documentUrl", documentUrl},
            {"line", line},
            {"character", character}
        };
        QVariantList list = m_navigationHistory["list"].toList();
        list.append(navigationSession);
        m_navigationHistory["list"] = list;
    }
}

void DocumentModule::didCreateFilesNotification(const QUrl &documentUrl) {
    const QJsonObject params{
        {
            "files", QJsonArray{
                QJsonObject{
                    {"uri", documentUrl.toString()}
                }
            }
        }
    };
    emit notificationJson("workspace/didCreateFiles", params);
}

void DocumentModule::didRenameFilesNotification(const QUrl &oldUrl, const QUrl &newUrl) {
    const QJsonObject params{
        {
            "files", QJsonArray{
                QJsonObject{
                    {"oldUri", oldUrl.toString()},
                    {"newUri", newUrl.toString()}
                }
            }
        }
    };
    emit notificationJson("workspace/didRenameFiles", params);
}

void DocumentModule::didDeleteFilesNotification(const QUrl &documentUrl) {
    const QJsonObject params{
        {
            "files", QJsonArray{
                QJsonObject{
                    {"uri", documentUrl.toString()}
                }
            }
        }
    };
    emit notificationJson("workspace/didDeleteFiles", params);
}
