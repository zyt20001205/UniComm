#ifndef UNICOMM_DOCUMENTMODULE_H
#define UNICOMM_DOCUMENTMODULE_H

#include <QJsonObject>
#include <QSharedPointer>
#include <kddockwidgets/qtwidgets/views/DockWidget.h>

#include "document/page/welcomePage.h"

class QFileSystemWatcher;
class QLabel;
class QTableWidget;
class QTabWidget;
class QTextBrowser;

class DocumentPage;
class CodeAssistant;
class CodePage;
class EditorWidget;
class ScintillaWidget;
class SignatureWidget;
class ToastModule;

class DocumentModule final : public QObject {
    Q_OBJECT

public:
    explicit DocumentModule(QWidget *parent = nullptr);

    ~DocumentModule() override;

    void propertySet(const QVariantHash &objects);

    [[nodiscard]] KDDockWidgets::QtWidgets::DockWidget *welcomePage() const {
        return m_welcomePage;
    }

    void documentConfigSave();

    void scriptFontReload(const QJsonObject &fontConfigScript) const;

    void scriptFontSave(const QJsonObject &fontConfigScript);

    [[nodiscard]] DocumentPage *documentConstruct(const QUrl &documentUrl);

    Q_INVOKABLE void documentOpen(const QUrl &documentUrl);

    void documentGoto(const QUrl &documentUrl) const;

    [[nodiscard]] QJsonArray directoryList(const QUrl &directoryUrl) const;

    Q_INVOKABLE [[nodiscard]] QString directoryCreate(const QUrl &directoryUrl, const QString &undoGroupId = {});

    Q_INVOKABLE [[nodiscard]] QString directoryRename(const QUrl &sourceUrl, const QUrl &targetUrl, const QString &undoGroupId = {});

    Q_INVOKABLE [[nodiscard]] QString directoryDelete(const QUrl &directoryUrl, const QString &undoGroupId = {});

    Q_INVOKABLE [[nodiscard]] QString documentCreate(const QUrl &documentUrl, const QString &undoGroupId = {});

    Q_INVOKABLE [[nodiscard]] QString documentRename(const QUrl &sourceUrl, const QUrl &targetUrl, const QString &undoGroupId = {});

    Q_INVOKABLE [[nodiscard]] QString documentDelete(const QUrl &documentUrl, const QString &undoGroupId = {});

    Q_INVOKABLE [[nodiscard]] QString documentSave(const QUrl &documentUrl) const;

    Q_INVOKABLE void documentClose(const QUrl &documentUrl, bool save) const;

    void documentReload(const QString &documentPath);

    void permissionSet(const QUrl &documentUrl) const;

    Q_INVOKABLE [[nodiscard]] QVariantHash menuLoad(const QString &name);

    Q_INVOKABLE void menuCall(const QString &name) const;

    Q_INVOKABLE [[nodiscard]] int eolModeGet(const QUrl &documentUrl) const;

    Q_INVOKABLE void eolModeSet(const QUrl &documentUrl, int eolMode) const;

    Q_INVOKABLE [[nodiscard]] bool eolViewGet(const QUrl &documentUrl) const;

    Q_INVOKABLE void eolViewSet(const QUrl &documentUrl, bool status) const;

    Q_INVOKABLE void foldContractTop(const QUrl &documentUrl);

    Q_INVOKABLE void foldContractRecursively(const QUrl &documentUrl);

    Q_INVOKABLE void foldExpandRecursively(const QUrl &documentUrl);

    Q_INVOKABLE void navigationPrev();

    Q_INVOKABLE void navigationNext();

    void focusSet(const QUrl &documentUrl, bool status);

    Q_INVOKABLE void indexSet(const QUrl &documentUrl, int line, int character);

    void indexGet() const;

    void transactionBegin(const QString &undoGroupId);

    [[nodiscard]] QString transactionCommit(const QString &undoGroupId);

    [[nodiscard]] QString linesGet(const QUrl &documentUrl, int startLine, int lineCount) const;

    [[nodiscard]] QString linesSet(const QUrl &documentUrl, const QStringList &texts, const QList<int> &startLines, const QList<int> &lineCounts,
                                   const QString &undoGroupId = {});

    [[nodiscard]] QString textGet(const QUrl &documentUrl, int startLine = -1, int startCharacter = -1, int endLine = -1, int endCharacter = -1) const;

    void textSet(const QUrl &documentUrl, const QString &text, int startLine, int startCharacter, int endLine, int endCharacter);

    void indicatorFill(const QUrl &documentUrl, int type, int startLine, int startCharacter, int endLine, int endCharacter, int time = -1);

    void indicatorClear(const QUrl &documentUrl, int type, int startLine = -1, int startCharacter = -1, int endLine = -1, int endCharacter = -1) const;

    void markerAdd(const QUrl &documentUrl, int type, int line, int time = -1);

    void markerDelete(const QUrl &documentUrl, int type, int line = -1) const;

    [[nodiscard]] QJsonArray diagnosticsGet(const QUrl &documentUrl) const;

    void diagnosticsNotification(const QUrl &documentUrl, const QJsonArray &diagnostics);

    void codeActionRequest(const QUrl &documentUrl, int startLine, int startCharacter, int endLine, int endCharacter);

    Q_INVOKABLE void completionRequest(const QUrl &documentUrl, int line, int character);

    void completionResponse(const QUrl &documentUrl, const QJsonArray &items) const;

    Q_INVOKABLE void definitionRequest(const QUrl &documentUrl, int line, int character);

    void definitionResponse(const QUrl &documentUrl, const QJsonArray &definitions) const;

    void documentSymbolRequest(const QUrl &documentUrl);

    void documentSymbolResponse(const QUrl &documentUrl, const QJsonArray &result);

    void documentHighlightRequest(const QUrl &documentUrl, int line, int character);

    void documentHighlightResponse(const QUrl &documentUrl, const QJsonArray &result) const;

    void foldingRangeRequest(const QUrl &documentUrl);

    void foldingRangeResponse(const QUrl &documentUrl, const QJsonArray &result) const;

    Q_INVOKABLE void formattingRequest(const QUrl &documentUrl);

    void formattingResponse(const QUrl &documentUrl, const QString &newText) const;

    void hoverRequest(const QUrl &documentUrl, int line, int character);

    void hoverResponse(const QUrl &documentUrl, const QString &message) const;

    Q_INVOKABLE void implementationRequest(const QUrl &documentUrl, int line, int character);

    void implementationResponse(const QUrl &documentUrl, const QJsonArray &implementations) const;

    void onTypeFormattingRequest(const QUrl &documentUrl, int line, int character);

    void onTypeFormattingResponse(const QUrl &documentUrl, const QJsonObject &newText) const;

    Q_INVOKABLE void prepareRenameRequest(const QUrl &documentUrl, int line, int character);

    void prepareRenameResponse(const QUrl &documentUrl, const QString &oldName) const;

    Q_INVOKABLE void rangeFormattingRequest(const QUrl &documentUrl, int startLine, int startCharacter, int endLine, int endCharacter);

    void rangeFormattingResponse(const QUrl &documentUrl, const QString &newText) const;

    Q_INVOKABLE void referencesRequest(const QUrl &documentUrl, int line, int character);

    void referencesResponse(const QUrl &documentUrl, const QJsonArray &references) const;

    Q_INVOKABLE void renameRequest(const QUrl &documentUrl, int line, int character, const QString &newName);

    void renameResponse(const QUrl &documentUrl, const QJsonObject &workspaceEdit);

    void semanticTokensRequest(const QUrl &documentUrl);

    void semanticTokensResponse(const QUrl &documentUrl, const QJsonArray &data) const;

    void signatureHelpRequest(const QUrl &documentUrl, int line, int character);

    void signatureHelpResponse(const QUrl &documentUrl, const QJsonArray &signatures) const;

    Q_INVOKABLE void typeDefinitionRequest(const QUrl &documentUrl, int line, int character);

    void typeDefinitionResponse(const QUrl &documentUrl, const QJsonArray &typeDefinitions) const;

    void spellCheckResponse(const QUrl &documentUrl, const QVariantList &typos);

signals:
    void appendLog(int type, const QString &prefix, const QString &message);

    void updateDiff(const QString &undoGroupId, const QVariantMap &fileDiffs, int additions, int deletions);

    void openWorkspace();

    void focusDocument(const QUrl &documentUrl, const QVariantHash &session);

    void changeSelection(const QHash<QString, int> &selection);

    void insertBreakpoint(const QUrl &documentUrl, int line, const QVariantHash &session);

    void removeBreakpoint(const QUrl &documentUrl, int line);

    void requestJson(const QString &method, const QJsonObject &params);

    void notificationJson(const QString &method, const QJsonObject &params);

    void responseCodeAction(const QUrl &documentUrl, const QJsonArray &result);

    void requestSpellCheck(const QUrl &documentUrl, const QString &script);

    void requestSpellSuggest(const QUrl &documentUrl, const QString &word);

private:
    struct DocumentTextState {
        QString before{};
        QString after{};
        bool dirty{};
    };

    struct DocumentDiffState {
        int additions{};
        int deletions{};
    };

    struct DocumentTransaction {
        QHash<QUrl, DocumentTextState> documents{};
        QHash<QUrl, DocumentDiffState> diffs{};
    };

    QString _directoryCreate(const QUrl &directoryUrl);

    QString _directoryRename(const QUrl &sourceUrl, const QUrl &targetUrl);

    QString _directoryDelete(const QUrl &directoryUrl, QUrl &trashUrl);

    QString _directoryRestore(const QUrl &directoryUrl, const QUrl &trashUrl);

    QString _documentCreate(const QUrl &documentUrl);

    QString _documentRename(const QUrl &sourceUrl, const QUrl &targetUrl);

    QString _documentDelete(const QUrl &documentUrl, QUrl &trashUrl);

    QString _documentRestore(const QUrl &documentUrl, const QUrl &trashUrl);

    QString _linesSet(const QUrl &documentUrl, const QStringList &texts, const QList<int> &startLines, const QList<int> &lineCounts) const;

    QString _transactionRedo(const QSharedPointer<const DocumentTransaction> &transaction);

    QString _transactionUndo(const QSharedPointer<const DocumentTransaction> &transaction);

    QString _transactionFlush(const QString &undoGroupId, const QUrl &documentUrl = {}, bool recursive = false);

    QString _transactionCheck(const QString &undoGroupId, const QUrl &documentUrl, bool recursive) const;

    void _transactionRename(const QString &undoGroupId, const QUrl &sourceUrl, const QUrl &targetUrl, bool recursive);

    [[nodiscard]] ScintillaWidget *handlerGet(const QUrl &documentUrl) const;

    void documentFocus(DocumentPage *documentPage, bool status);

    void documentClose(const QUrl &documentUrl);

    void charAdd(const QUrl &documentUrl, QChar character) const;

    void textSetSelected(const QUrl &documentUrl, const QString &text);

    void navigationRecord(const QUrl &documentUrl, int line, int character);

    void didCreateFilesNotification(const QUrl &documentUrl);

    void didRenameFilesNotification(const QUrl &oldUrl, const QUrl &newUrl);

    void didDeleteFilesNotification(const QUrl &documentUrl);

    QJsonObject m_config{};
    QJsonObject m_theme{};
    ToastModule *m_toast{};
    QObject *m_toolTip{};
    QObject *m_breakpointEditDialog{};
    QObject *m_systemPropertyDialog{};
    QObject *m_gotoDialog{};
    QObject *m_renameDialog{};
    QObject *m_saveDialog{};
    QObject *m_editorMenu{};
    QUrl m_focusedUrl{};
    QFileSystemWatcher *m_watcher{};
    WelcomePage *m_welcomePage{}; // TODO: inherits base page later
    CodeAssistant *m_codeAssistant{};
    QHash<QUrl, DocumentPage *> m_pageHash{};
    QHash<QString, QSharedPointer<DocumentTransaction> > m_transactions{};
    QHash<QUrl, QJsonArray> m_diagnosticsHash{};
    QHash<QUrl, QJsonArray> m_symbolHash{};
    QVariantHash m_navigationHistory{};
};

#endif //UNICOMM_DOCUMENTMODULE_H
