#include "document/module/codeWidget.h"

#include <QFileInfo>
#include <QShortcut>
#include <QTimer>

#include "globals.h"
#include "core/globalManager.h"
#include "document/module/scintillaWidget.h"
#include "util/uniCast.h"

// public
CodeWidget::CodeWidget(const QJsonObject &documentConfig, const QUrl &documentUrl, QWidget *parent)
    : EditorWidget(documentConfig, documentUrl, parent),
      m_dwellTimer(new QTimer(this)),
      m_completionTrigger{'.', ':', '\'', '"', '[', '#', '*', '@', '|', '=', '-', '{', '+', '?'},
      m_formattingTrigger{'\n'},
      m_signatureHelpTrigger{'(', ','} {
    connect(m_scintillaWidget, &ScintillaEdit::charAdded, this, &CodeWidget::charAdd);
    m_scintillaWidget->viewport()->installEventFilter(this);
    // 1000ms debounce for dwell change
    m_dwellTimer->setSingleShot(true);
    m_dwellTimer->setInterval(1000);
    connect(m_dwellTimer, &QTimer::timeout, this, &CodeWidget::dwellChange);
}

void CodeWidget::propertySet(const QVariantHash &objects) {
    m_theme = objects["theme"].toJsonObject();
    m_toolTip = qvariant_cast<QObject *>(objects["mainWindowToolTip"]);
    m_breakpointEditDialog = qvariant_cast<QObject *>(objects["breakpointModuleEditDialog"]);
    m_editorMenu = qvariant_cast<QObject *>(objects["documentModuleEditorMenu"]);
    EditorWidget::propertySet(objects);
    QTimer::singleShot(0, [this] {
        didOpenNotification();
        contentChange();
    });
}

QString CodeWidget::documentSave() {
    auto error = EditorWidget::documentSave();
    if (!error.isEmpty()) return error;
    didSaveNotification();
    return {};
}

QVariantHash CodeWidget::menuLoad(const QString &name) const {
    QVariantHash menuSession{};
    if (name == "edit") {
        menuSession = {
            {"copiable", m_scintillaWidget->copiable()},
            {"pastable", m_scintillaWidget->pastable()}
        };
    } else if (name == "nav") {
        menuSession = {
            {"documentUrl", m_documentUrl},
            {"line", m_selection["startLine"]},
            {"character", m_selection["character"]},
            {"navigation", navigable(m_scintillaWidget->currentPos())}
        };
    } else if (name == "code") {
        menuSession = {
            {"documentUrl", m_documentUrl},
            {"line", m_selection["line"]},
            {"character", m_selection["character"]},
            {"startLine", m_selection["startLine"]},
            {"startCharacter", m_selection["startCharacter"]},
            {"endLine", m_selection["endLine"]},
            {"endCharacter", m_selection["endCharacter"]},
            {"text", m_scintillaWidget->textGetSelected()}
        };
    } else if (name == "exec") {
        menuSession = {
            {"documentUrl", m_documentUrl},
            {"documentName", m_documentUrl.fileName()}
        };
    } else if (name == "editor") {
        menuSession = {
            {"documentUrl", m_documentUrl},
            {"line", m_selection["line"]},
            {"character", m_selection["character"]},
            {"startLine", m_selection["startLine"]},
            {"startCharacter", m_selection["startCharacter"]},
            {"endLine", m_selection["endLine"]},
            {"endCharacter", m_selection["endCharacter"]},
            {"text", m_scintillaWidget->textGetSelected()},
            {"navigation", navigable(m_scintillaWidget->currentPos())}
        };
    }
    return menuSession;
}

void CodeWidget::menuCall(const QString &name) const {
    m_scintillaWidget->focusSet(true);
    if (name == "cut") {
        m_scintillaWidget->cut();
    } else if (name == "copy") {
        m_scintillaWidget->copy();
    } else if (name == "paste") {
        m_scintillaWidget->paste();
    } else if (name == "search") {
        searchShow();
    } else if (name == "replace") {
        replaceShow();
    }
}

// public: lsp
void CodeWidget::diagnosticsNotification(const QJsonArray &diagnostics) {
    m_diagnostic = diagnostics;
    // clear
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Password);
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Error);
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Warning);
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Info);
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Hint);
    // publish
    for (const auto &value: diagnostics) {
        const QJsonObject diagnostic = value.toObject();
        // placeholder operation
        int severity{};
        if (diagnostic["message"].toString().contains("__PLACEHOLDER__PASSWORD__")) {
            severity = 0;
        } else {
            severity = diagnostic["severity"].toInt();
        }
        const QJsonObject range = diagnostic["range"].toObject();
        const QJsonObject start = range["start"].toObject();
        const QJsonObject end = range["end"].toObject();
        const int startLine = start["line"].toInt();
        const int startCharacter = start["character"].toInt();
        const int endLine = end["line"].toInt();
        const int endCharacter = end["character"].toInt();
        int type{};
        switch (severity) {
            case 0: type = ScintillaIndicator::Password;
                break;
            case 1: type = ScintillaIndicator::Error;
                break;
            case 2: type = ScintillaIndicator::Warning;
                break;
            case 3: type = ScintillaIndicator::Info;
                break;
            case 4: type = ScintillaIndicator::Hint;
                break;
            default: break;
        }
        m_scintillaWidget->indicatorFill(type, startLine, startCharacter, endLine, endCharacter);
    }
}

void CodeWidget::documentHighlightResponse(const QJsonArray &result) const {
    // clear previous highlight
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Highlight);
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Read);
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Write);
    // highlight
    for (const auto &highlight: result) {
        const QJsonObject highlightObject = highlight.toObject();
        const int kind = highlightObject["kind"].toInt();
        const QJsonObject highlightRange = highlightObject["range"].toObject();
        const QJsonObject highlightStartPos = highlightRange["start"].toObject();
        const QJsonObject highlightEndPos = highlightRange["end"].toObject();
        const int startLine = highlightStartPos["line"].toInt();
        const int startCharacter = highlightStartPos["character"].toInt();
        const int endLine = highlightEndPos["line"].toInt();
        const int endCharacter = highlightEndPos["character"].toInt();
        m_scintillaWidget->indicatorFill(ScintillaIndicator::Highlight, startLine, startCharacter, endLine, endCharacter);
        if (kind == 2) m_scintillaWidget->indicatorFill(ScintillaIndicator::Read, startLine, startCharacter, endLine, endCharacter);
        else if (kind == 3) m_scintillaWidget->indicatorFill(ScintillaIndicator::Write, startLine, startCharacter, endLine, endCharacter);
    }
}

void CodeWidget::foldingRangeResponse(const QJsonArray &result) const {
    QHash<int, int> deltaDepthHash{};
    for (const auto &value: result) {
        const QJsonObject valueObject = value.toObject();
        const int startLine = valueObject["startLine"].toInt();
        const int endLine = valueObject["endLine"].toInt();
        deltaDepthHash.insert(startLine + 1, deltaDepthHash.value(startLine + 1, 0) + 1);
        deltaDepthHash.insert(endLine + 1, deltaDepthHash.value(endLine + 1, 0) - 1);
    }
    int depth = 0;
    for (int line = 0; line < m_scintillaWidget->lineCountGet(); line++) {
        const int deltaDepth = deltaDepthHash.value(line, 0);
        depth += deltaDepth;
        int level = SC_FOLDLEVELBASE + depth;
        if (deltaDepthHash.value(line + 1, 0) > 0) level |= SC_FOLDLEVELHEADERFLAG;
        m_scintillaWidget->foldLevelSet(line, level);
    }
}

void CodeWidget::formattingResponse(const QString &newText) const {
    m_scintillaWidget->textSet(newText);
}

void CodeWidget::onTypeFormattingResponse(const QJsonObject &newText) const {
    const QString text = newText["newText"].toString();
    const QJsonObject range = newText["range"].toObject();
    const QJsonObject start = range["start"].toObject();
    const QJsonObject end = range["end"].toObject();
    const int startLine = start["line"].toInt();
    const int startCharacter = start["character"].toInt();
    const int endLine = end["line"].toInt();
    const int endCharacter = end["character"].toInt();
    m_scintillaWidget->textSet(text, startLine, startCharacter, endLine, endCharacter);
}

void CodeWidget::rangeFormattingResponse(const QString &newText) const {
    m_scintillaWidget->textSetSelected(newText);
}

void CodeWidget::renameResponse(const QJsonArray &changes) const {
    m_scintillaWidget->undoBegin();
    for (int index = static_cast<int>(changes.size()) - 1; index >= 0; --index) {
        const auto edit = changes[index].toObject();
        const auto range = edit["range"].toObject();
        const auto start = range["start"].toObject();
        const auto end = range["end"].toObject();
        m_scintillaWidget->textSet(
            edit["newText"].toString(),
            start["line"].toInt(),
            start["character"].toInt(),
            end["line"].toInt(),
            end["character"].toInt()
        );
    }
    m_scintillaWidget->undoEnd();
}

void CodeWidget::semanticTokensResponse(const QJsonArray &data) {
    // clear
    m_scintillaWidget->styleSet(ScintillaStyle::Unused);
    // publish
    int line = 0;
    int character = 0;
    for (int i = 0; i < data.size(); i += 5) {
        const int deltaLine = data[i].toInt();
        const int deltaCharacter = data[i + 1].toInt();
        const int length = data[i + 2].toInt();
        const int tokenType = data[i + 3].toInt();
        const int tokenModifiers = data[i + 4].toInt();
        line += deltaLine;
        character = deltaLine > 0 ? deltaCharacter : character + deltaCharacter;
        int type{};
        switch (tokenType) {
            case LspTokenType::Namespace:
                type = ScintillaStyle::Namespace;
                break;
            case LspTokenType::Class:
                type = ScintillaStyle::Class;
                break;
            case LspTokenType::Type:
                type = ScintillaStyle::Type;
                break;
            case LspTokenType::Parameter:
                type = ScintillaStyle::Parameter;
                break;
            case LspTokenType::Variable:
                type = ScintillaStyle::Variable;
                break;
            case LspTokenType::Property:
                type = ScintillaStyle::Property;
                break;
            case LspTokenType::EnumMember:
                type = ScintillaStyle::EnumMember;
                break;
            case LspTokenType::Function:
                if (tokenModifiers == LspTokenModifiers::Declaration || tokenModifiers == LspTokenModifiers::Global) type = ScintillaStyle::FunctionDeclaration;
                else type = ScintillaStyle::FunctionCall;
                break;
            case LspTokenType::Method:
                type = ScintillaStyle::Method;
                break;
            case LspTokenType::Macro:
                type = ScintillaStyle::Macro;
                break;
            case LspTokenType::Keyword:
                type = ScintillaStyle::Keyword;
                break;
            case LspTokenType::Comment:
                type = ScintillaStyle::Comment;
                break;
            case LspTokenType::String:
                type = ScintillaStyle::String;
                break;
            case LspTokenType::Number:
                type = ScintillaStyle::Number;
                break;
            case LspTokenType::Operator:
                type = ScintillaStyle::Operator;
                break;
            default:
                emit appendLog(LogLevel::Warning, "contact author:", QString("unsupported semantic (token type:%1)").arg(QString::number(tokenType)));
                break;
        }
        m_scintillaWidget->styleSet(type, line, character, length);
    }
}

// public: typo
void CodeWidget::spellCheckResponse(const QVariantList &typos) {
    m_typo = typos;
    // clear
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Typo);
    // publish
    for (const auto &value: typos) {
        auto typo = value.toMap();
        const int startLine = typo["line"].toInt();
        const int endLine = typo["line"].toInt();
        const int startCharacter = typo["startCharacter"].toInt();
        const int endCharacter = typo["endCharacter"].toInt();
        m_scintillaWidget->indicatorFill(ScintillaIndicator::Typo, startLine, startCharacter, endLine, endCharacter);
    }
}

bool CodeWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_scintillaWidget->viewport()) {
        const QPoint globalPos = QCursor::pos();
        const QPoint localPos = m_scintillaWidget->viewport()->mapFromGlobal(globalPos);
        if (event->type() == QEvent::MouseButtonPress) {
            m_dwellTimer->stop();
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            const auto modifiers = mouseEvent->modifiers();
            const auto position = m_scintillaWidget->cast<Scintilla::Position>(ScintillaWidget::ViewportPoint{localPos});
            const auto index = m_scintillaWidget->cast<ScintillaWidget::Utf16Index>(position);
            // margin click
            if (localPos.x() < m_scintillaWidget->marginWidthGet()) return false;
            // text area click
            if (mouseEvent->button() == Qt::LeftButton) {
                if (modifiers == Qt::ControlModifier) {
                    m_scintillaWidget->positionSet(position);
                    if (m_scintillaWidget->indicatorGet(position) & 1 << ScintillaIndicator::Hyperlink) {
                        emit requestDefinition(m_documentUrl, index.line, index.character);
                        emit requestReferences(m_documentUrl, index.line, index.character);
                    }
                    return false;
                }
            }
            if (mouseEvent->button() == Qt::RightButton) {
                if (m_scintillaWidget->textGetSelected().isEmpty()) m_scintillaWidget->positionSet(position);
                QMetaObject::invokeMethod(m_editorMenu, "popup");
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            const auto modifiers = mouseEvent->modifiers();
            if (modifiers == Qt::ControlModifier) {
                m_dwellTimer->stop();
                const auto position = m_scintillaWidget->cast<Scintilla::Position>(ScintillaWidget::ViewportPoint{localPos});
                navigationToggle(position);
                return true;
            }
            m_dwellTimer->start();
            navigationToggle();
            return false;
        }
    }
    return EditorWidget::eventFilter(watched, event);
}

// protected
void CodeWidget::shortcutInit() {
    EditorWidget::shortcutInit();
    auto shortcutComment = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), this); // NOLINT
    connect(shortcutComment, &QShortcut::activated, this, &CodeWidget::commentToggle);
    shortcutComment->setContext(Qt::WidgetWithChildrenShortcut);
}

void CodeWidget::selectionChange() {
    EditorWidget::selectionChange();
    if (m_scintillaWidget->selectionEmpty()) {
        documentHighlightRequest();
        emit showDocumentSymbol(m_selection["line"], m_selection["character"]);
    }
}

void CodeWidget::contentChange() {
    // status refresh
    breakpointGet();
    // lsp request
    didChangeNotification();
    documentHighlightRequest();
    documentSymbolRequest();
    foldingRangeRequest();
    semanticTokensRequest();
    // nuspell request
    spellCheckRequest();
}

bool CodeWidget::symbolPair(const QChar ch) {
    charAdd(ch.toLatin1());
    return EditorWidget::symbolPair(ch);
}

void CodeWidget::indicatorInit() const {
    EditorWidget::indicatorInit();
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Typo,
        QVariantHash{
            {"style", INDIC_SQUIGGLE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->successFore3Get())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Hint,
        QVariantHash{
            {"style", INDIC_SQUIGGLE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->brandBackGet())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Info,
        QVariantHash{
            {"style", INDIC_SQUIGGLE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->warningFore3Get())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Warning,
        QVariantHash{
            {"style", INDIC_SQUIGGLE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->warningFore3Get())},
            {"strokeWidth", 200},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Error,
        QVariantHash{
            {"style", INDIC_SQUIGGLE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->dangerFore3Get())},
            {"strokeWidth", 200},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Password,
        QVariantHash{
            {"style", INDIC_STRAIGHTBOX},
            {"fore", ScintillaWidget::colorGet(g_globalManager->backGet())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false},
            {"hoverStyle", 5}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Highlight,
        QVariantHash{
            {"style", INDIC_STRAIGHTBOX},
            {"fore", ScintillaWidget::colorGet(g_globalManager->strokeGet())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", true}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Read,
        QVariantHash{
            {"style", INDIC_POINT},
            {"fore", ScintillaWidget::colorGet(g_globalManager->successFore3Get())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Write,
        QVariantHash{
            {"style", INDIC_POINT_TOP},
            {"fore", ScintillaWidget::colorGet(g_globalManager->warningFore3Get())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Result,
        QVariantHash{
            {"style", INDIC_STRAIGHTBOX},
            {"fore", ScintillaWidget::colorGet(g_globalManager->brandBackGet())},
            {"alpha", 128},
            {"outlineAlpha", 128},
            {"setUnder", true}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Current,
        QVariantHash{
            {"style", INDIC_BOX},
            {"fore", ScintillaWidget::colorGet(g_globalManager->strokeGet())},
            {"strokeWidth", 200},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Hyperlink,
        QVariantHash{
            {"style", INDIC_TEXTFORE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->brandLinkGet())},
            {"alpha", 255},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
}

void CodeWidget::marginInit() const {
    m_scintillaWidget->marginDefine(
        0,
        QVariantHash{
            {"type", SC_MARGIN_NUMBER},
            {"width", 32},
            {"back", ScintillaWidget::colorGet(g_globalManager->backGet())}
        });
    m_scintillaWidget->marginDefine(
        1,
        QVariantHash{
            {"type", SC_MARGIN_SYMBOL},
            {"width", 16},
            {"mask", static_cast<int>(~SC_MASK_FOLDERS & ~SC_MASK_HISTORY)},
            {"sensitive", true},
            {"back", ScintillaWidget::colorGet(g_globalManager->backGet())}
        });
    m_scintillaWidget->marginDefine(
        2,
        QVariantHash{
            {"type", SC_MARGIN_SYMBOL},
            {"width", 16},
            {"mask", static_cast<int>(SC_MASK_FOLDERS)},
            {"sensitive", true},
            {"back", ScintillaWidget::colorGet(g_globalManager->backGet())}
        });
    m_scintillaWidget->marginDefine(
        3,
        QVariantHash{
            {"type", SC_MARGIN_SYMBOL},
            {"width", 4},
            {"mask", SC_MASK_HISTORY},
            {"back", ScintillaWidget::colorGet(g_globalManager->backGet())}
        });
}

void CodeWidget::markerInit() const {
    EditorWidget::markerInit();
    m_scintillaWidget->markerDefine(
        ScintillaMarker::BreakpointEnabled,
        QVariantHash{
            {"symbol", SC_MARK_CIRCLE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->dangerFore2Get())},
            {"back", ScintillaWidget::colorGet(g_globalManager->dangerBack2Get())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::BreakpointDisabled,
        QVariantHash{
            {"symbol", SC_MARK_CIRCLE},
            {"fore", ScintillaWidget::colorGet(g_globalManager->dangerFore2Get())},
            {"back", ScintillaWidget::colorGet(g_globalManager->backGet())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::Navigation,
        QVariantHash{
            {"symbol", SC_MARK_ARROWS},
            {"fore", ScintillaWidget::colorGet(g_globalManager->foreGet())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::Debug,
        QVariantHash{
            {"symbol", SC_MARK_ARROW},
            {"fore", ScintillaWidget::colorGet(g_globalManager->warningFore2Get())},
            {"back", ScintillaWidget::colorGet(g_globalManager->warningFore2Get())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::Hint,
        QVariantHash{
            {"symbol", SC_MARK_BACKGROUND},
            {"back", ScintillaWidget::colorGet(g_globalManager->strokeGet())}
        });
}

void CodeWidget::lexerInit() const {
    const auto lspTheme = m_theme["lsp"].toObject();
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Namespace,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["namespace"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Class,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["class"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Type,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["type"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Parameter,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["parameter"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Variable,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["variable"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Property,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["property"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::EnumMember,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["enumMember"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::FunctionCall,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["functionCall"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::FunctionDeclaration,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["functionDeclaration"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Method,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["method"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Macro,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["macro"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Keyword,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["keyword"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Comment,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["comment"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::String,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["string"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Number,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["number"].toObject()["fore"].toString())}
        });
    m_scintillaWidget->styleDefine(
        ScintillaStyle::Operator,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(lspTheme["operator"].toObject()["fore"].toString())}
        });
}

// private
void CodeWidget::charAdd(const int ch) {
    m_contentTimer->start();
    m_dwellTimer->stop();
    selectionChange();
    const QChar character(ch);
    if (character.isLetter() || m_completionTrigger.contains(character)) {
        didChangeNotification();
        completionRequest();
    } else if (m_signatureHelpTrigger.contains(character)) {
        didChangeNotification();
        completionRequest();
        signatureHelpRequest();
    } else if (m_formattingTrigger.contains(character)) {
        didChangeNotification();
        onTypeFormattingRequest();
    }
}

// private: slot
void CodeWidget::marginClick(const Scintilla::Position position, const int mouseButton, const Scintilla::KeyMod modifiers, const int margin) {
    const int line = m_scintillaWidget->lineGet(position);
    if (margin == 1) {
        if (mouseButton == Qt::LeftButton) {
            if (m_scintillaWidget->markerGet(line) & 1 << ScintillaMarker::BreakpointEnabled) {
                emit removeBreakpoint(m_documentUrl, line + 1);
                m_scintillaWidget->markerDelete(ScintillaMarker::BreakpointEnabled, line);
            } else if (m_scintillaWidget->markerGet(line) & 1 << ScintillaMarker::BreakpointDisabled) {
                emit removeBreakpoint(m_documentUrl, line + 1);
                m_scintillaWidget->markerDelete(ScintillaMarker::BreakpointDisabled, line);
            } else if (m_scintillaWidget->markerGet(line) & 1 << ScintillaMarker::Navigation) {
                // TODO: assembly interaction
                // m_assemblyWidget->handler()->markerAdd(ScintillaMarker::Hint, m_l2aHash[line], 1000);
            } else {
                emit insertBreakpoint(m_documentUrl, line + 1, QVariantHash({
                                          {"condition", ""},
                                          {"enabled", true}
                                      }));
                if (modifiers == Scintilla::KeyMod::Ctrl) breakpointSet(line + 1);
            }
        } else if (mouseButton == Qt::RightButton) {
            if (m_scintillaWidget->markerGet(line) & 1 << ScintillaMarker::BreakpointEnabled || m_scintillaWidget->markerGet(line) & 1 << ScintillaMarker::BreakpointDisabled) {
                breakpointSet(line + 1);
            }
        }
    }
}

void CodeWidget::dwellChange() {
    hoverRequest();
}

// private: file
void CodeWidget::breakpointGet() const {
    m_scintillaWidget->markerDelete(ScintillaMarker::BreakpointEnabled);
    m_scintillaWidget->markerDelete(ScintillaMarker::BreakpointDisabled);
    if (g_breakpoints.contains(m_documentUrl)) {
        for (const auto &line: g_breakpoints[m_documentUrl].keys()) {
            if (g_breakpoints[m_documentUrl][line]["enabled"].toBool()) m_scintillaWidget->markerAdd(ScintillaMarker::BreakpointEnabled, line - 1);
            else m_scintillaWidget->markerAdd(ScintillaMarker::BreakpointDisabled, line - 1);
        }
    }
}

void CodeWidget::breakpointSet(const int line) const {
    m_breakpointEditDialog->setProperty("documentUrl", m_documentUrl);
    m_breakpointEditDialog->setProperty("line", line);
    QMetaObject::invokeMethod(m_breakpointEditDialog, "open");
}

// private: lsp
void CodeWidget::didOpenNotification() {
    const QString suffix = QFileInfo(m_documentUrl.toLocalFile()).suffix().toLower();
    QString languageId{};
    if (suffix == "md") languageId = "markdown";
    else languageId = suffix;
    // did open notification to lsp manager
    const QJsonObject didOpenParams{
        {
            "textDocument", QJsonObject{
                {"uri", m_documentUrl.toString()},
                {"languageId", languageId},
                {"version", m_version++},
                {"text", m_scintillaWidget->textGet()}
            }
        }
    };
    emit notificationJson("textDocument/didOpen", didOpenParams);
}

void CodeWidget::didChangeNotification() {
    // did change notification to lsp manager
    const auto content = m_scintillaWidget->textGet();
    const QJsonObject didChangeParams{
        {
            "textDocument", QJsonObject{
                {"uri", m_documentUrl.toString()},
                {"version", m_version++}
            }
        },
        {
            "contentChanges", QJsonArray{
                QJsonObject{
                    {"text", content}
                }
            }
        }
    };
    emit notificationJson("textDocument/didChange", didChangeParams);
}

void CodeWidget::didSaveNotification() {
    // did save notification to lsp manager
    const QJsonObject didSaveParams{
        {
            "textDocument", QJsonObject{
                {"uri", m_documentUrl.toString()}
            }
        }
    };
    emit notificationJson("textDocument/didSave", didSaveParams);
}

void CodeWidget::didCloseNotification() {
    // did close notification to lsp manager
    const QJsonObject didCloseParams{
        {
            "textDocument", QJsonObject{
                {"uri", m_documentUrl.toString()}
            }
        }
    };
    emit notificationJson("textDocument/didClose", didCloseParams);
}

void CodeWidget::completionRequest() {
    // completion request to script module
    emit requestCompletion(m_documentUrl, m_selection["line"], m_selection["character"]);
}

void CodeWidget::definitionRequest() {
    // definition request to script module
    emit requestDefinition(m_documentUrl, m_selection["line"], m_selection["character"]);
}

void CodeWidget::documentHighlightRequest() {
    // document highlight request to script module
    emit requestDocumentHighlight(m_documentUrl, m_selection["line"], m_selection["character"]);
}

void CodeWidget::documentSymbolRequest() {
    // document symbol request to script module
    emit requestDocumentSymbol(m_documentUrl);
}

void CodeWidget::foldingRangeRequest() {
    // folding range request to script module
    emit requestFoldingRange(m_documentUrl);
}

void CodeWidget::formattingRequest() {
    // formatting request to script module
    emit requestFormatting(m_documentUrl);
}

void CodeWidget::hoverRequest() {
    const auto globalPos = QCursor::pos();
    const auto localPos = m_scintillaWidget->mapFromGlobal(globalPos);
    if (!rect().contains(localPos)) return;
    const auto closePosition = m_scintillaWidget->closePositionGet(localPos);
    if (closePosition == -1) return;
    const auto index = m_scintillaWidget->cast<ScintillaWidget::Utf16Index>(closePosition);
    const auto line = index.line;
    const auto character = index.character;
    if (line == 0 && character == 0) return;
    // show diagnostic if exists
    QString diagnosticText = "<table width='100%'>";
    for (const auto &value: m_diagnostic) {
        const QJsonObject diagnostic = value.toObject();
        const QJsonObject range = diagnostic["range"].toObject();
        const QJsonObject start = range["start"].toObject();
        const QJsonObject end = range["end"].toObject();
        const int startLine = start["line"].toInt();
        const int startCharacter = start["character"].toInt();
        const int endLine = end["line"].toInt();
        const int endCharacter = end["character"].toInt();
        if (line < startLine || line > endLine) continue;
        if (line == startLine && character < startCharacter) continue;
        if (line == endLine && character > endCharacter) continue;
        const int severity = diagnostic["severity"].toInt();
        QString severityString{};
        switch (severity) {
            case 1: {
                severityString = "Error";
            }
            break;
            case 2: {
                severityString = "Warning";
            }
            break;
            case 3: {
                severityString = "Info";
            }
            break;
            case 4: {
                severityString = "Hint";
            }
            break;
            default: break;
        }
        const QString message = diagnostic["message"].toString();
        // qDebug() << message << parsed;
        const QString customUrl = QString("request.code.action://reserved/%1/%2/%3/%4").arg(
            QString::number(startLine), QString::number(startCharacter), QString::number(endLine), QString::number(endCharacter));
        auto html = uni_cast<QHtmlString>(message).value;
        if (html.startsWith("<p>") && html.endsWith("</p>\n")) {
            html = html.mid(3, html.size() - 8);
        }
        diagnosticText += QString("<tr><td><b>%1</b>: %2</td><td align='right'><a href='%3'>Code Action</a></td></tr>").arg(severityString, html, customUrl);
    }
    // show typo if exists
    for (const auto &value: m_typo) {
        auto typo = value.toMap();
        const int startLine = typo["line"].toInt();
        const int endLine = typo["line"].toInt();
        const int startCharacter = typo["startCharacter"].toInt();
        const int endCharacter = typo["endCharacter"].toInt();
        if (line < startLine || line > endLine) continue;
        if (line == startLine && character < startCharacter) continue;
        if (line == endLine && character > endCharacter) continue;
        const QString word = m_scintillaWidget->textGet(startLine, startCharacter, endLine, endCharacter);
        const QString customUrl = QString("request.spell.suggestion://%1/%2/%3/%4/%5").arg(
            word, QString::number(startLine), QString::number(startCharacter), QString::number(endLine), QString::number(endCharacter));
        diagnosticText += QString("<tr><td><b>Typo</b>: In word '%1'</td><td align='right'><a href='%2'>Show Suggestions</a></td></tr>").arg(word, customUrl);
    }
    // call diagnostic show
    const QVariantHash diagnosticSession = {
        {"documentUrl", m_documentUrl},
        {"position", QCursor::pos()}
    };
    if (diagnosticText == "<table width='100%'>") {
        emit showDiagnostic(diagnosticSession, "");
    } else {
        diagnosticText += "</table>";
        emit showDiagnostic(diagnosticSession, diagnosticText);
    }
    // hover request to script module
    emit requestHover(m_documentUrl, line, character);
}

void CodeWidget::implementationRequest() {
    // implementation request to script module
    emit requestImplementation(m_documentUrl, m_selection["line"], m_selection["character"]);
}

void CodeWidget::prepareRenameRequest() {
    emit requestPrepareRename(m_documentUrl, m_selection["line"], m_selection["character"]);
}

void CodeWidget::referencesRequest() {
    // references request to script module
    emit requestReferences(m_documentUrl, m_selection["line"], m_selection["character"]);
}

void CodeWidget::onTypeFormattingRequest() {
    // on type formatting request to script module
    emit requestOnTypeFormatting(m_documentUrl, m_selection["line"], m_selection["character"]);
}

void CodeWidget::semanticTokensRequest() {
    // semantic tokens request to script module
    emit requestSemanticTokens(m_documentUrl);
}

void CodeWidget::signatureHelpRequest() {
    // signature help request to script module
    emit requestSignatureHelp(m_documentUrl, m_selection["line"], m_selection["character"]);
}

void CodeWidget::typeDefinitionRequest() {
    // type definition request to script module
    emit requestTypeDefinition(m_documentUrl, m_selection["line"], m_selection["character"]);
}

// private: typo
void CodeWidget::spellCheckRequest() {
    // spell check request to script module
    emit requestSpellCheck(m_documentUrl, m_scintillaWidget->textGet());
}

// private: misc
void CodeWidget::commentToggle() {
    if (m_scintillaWidget->textGetSelected().isEmpty()) {
        const auto position = m_scintillaWidget->positionGet();
        const auto index = m_scintillaWidget->cast<ScintillaWidget::Utf16Index>(position);
        auto text = m_scintillaWidget->textGet(index.line, 0, index.line, -1);
        if (text.contains("--")) {
            text.remove("--");
        } else {
            text = "--" + text;
        }
        m_scintillaWidget->textSet(text, index.line, 0, index.line, -1);
    } else {
        auto text = m_scintillaWidget->textGetSelected();
        if (text.contains("--[[") || text.contains("]]")) {
            text.remove("--[[");
            text.remove("]]");
        } else {
            text = "--[[" + text + "]]";
        }
        m_scintillaWidget->textSetSelected(text);
    }
    contentChange();
}

bool CodeWidget::navigable(const Scintilla::Position position) const {
    const int type = m_scintillaWidget->styleGet(position);
    if (type > 0 && type < ScintillaStyle::Macro) return true;
    return false;
}

void CodeWidget::navigationToggle(const Scintilla::Position position) const {
    if (position == -1) {
        m_scintillaWidget->indicatorClear(ScintillaIndicator::Hyperlink);
        m_toolTip->setProperty("text", "");
    } else {
        if (navigable(position)) {
            const auto wordRange = m_scintillaWidget->wordIndexGet(position);
            m_scintillaWidget->indicatorFill(ScintillaIndicator::Hyperlink, wordRange.start.line, wordRange.start.character, wordRange.end.line, wordRange.end.character);
            m_toolTip->setProperty("position", QCursor::pos());
            m_toolTip->setProperty("text", tr("Click to navigate"));
        } else {
            m_scintillaWidget->indicatorClear(ScintillaIndicator::Hyperlink);
            m_toolTip->setProperty("text", "");
        }
    }
}
