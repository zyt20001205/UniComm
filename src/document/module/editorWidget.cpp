#include "document/module/editorWidget.h"

#include <QFileInfo>
#include <QFile>
#include <QSaveFile>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>
#include <SciLexer.h>

#include "globals.h"
#include "core/globalManager.h"
#include "document/module/scintillaWidget.h"
#include "document/module/searchWidget.h"
#include "mainWindow/toastModule.h"
#include "types/scintillaTypes.h"

// public
EditorWidget::EditorWidget(const QJsonObject &documentConfig, const QUrl &documentUrl, QWidget *parent)
    : QWidget(parent),
      m_documentUrl(documentUrl),
      m_scintillaWidget(new ScintillaWidget(this)),
      m_config(documentConfig),
      m_searchWidget(new SearchWidget(this)),
      m_selectionTimer(new QTimer(this)),
      m_contentTimer(new QTimer(this)),
      m_pair{
          {'"', '"'},
          {'\'', '\''},
          {'(', ')'},
          {'[', ']'},
          {'{', '}'}
      } {
    auto *layout = new QVBoxLayout(this); // NOLINT
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_searchWidget);
    layout->addWidget(m_scintillaWidget);

    documentOpen();
    connect(m_scintillaWidget, &ScintillaEdit::modifyAttemptReadOnly, this, &EditorWidget::permissionSet);
    connect(m_scintillaWidget, &ScintillaEdit::savePointChanged, this, &EditorWidget::changeSavepoint);
    connect(m_scintillaWidget, &ScintillaEdit::updateUi, this, [this](const Scintilla::Update updated) {
        if (static_cast<int>(updated) & static_cast<int>(Scintilla::Update::Selection)) m_selectionTimer->start();
    });
    connect(m_scintillaWidget, &ScintillaEdit::focusChanged, this, [this](const bool focused) {
        if (focused) m_selectionTimer->start();
    });
    connect(m_scintillaWidget, &ScintillaEdit::modified, this, [this](const Scintilla::ModificationFlags type) {
        if (static_cast<int>(type) & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
            m_selectionTimer->start();
            m_contentTimer->start();
        }
    });
    connect(m_searchWidget, &SearchWidget::setSearchFlags, m_scintillaWidget, &ScintillaWidget::searchFlagsSet);
    connect(m_searchWidget, &SearchWidget::requestSearch, this, &EditorWidget::searchRequest);
    connect(m_searchWidget, &SearchWidget::prevSearch, this, &EditorWidget::searchPrev);
    connect(m_searchWidget, &SearchWidget::nextSearch, this, &EditorWidget::searchNext);
    connect(m_searchWidget, &SearchWidget::replaceText, this, &EditorWidget::textReplace);
    connect(m_searchWidget, &SearchWidget::replaceAll, this, &EditorWidget::allReplace);
    m_scintillaWidget->installEventFilter(this);
    // 100ms debounce for selection change
    m_selectionTimer->setSingleShot(true);
    m_selectionTimer->setInterval(100);
    connect(m_selectionTimer, &QTimer::timeout, this, &EditorWidget::selectionChange);
    // 500ms debounce for content change
    m_contentTimer->setSingleShot(true);
    m_contentTimer->setInterval(500);
    connect(m_contentTimer, &QTimer::timeout, this, &EditorWidget::contentChange);
}

void EditorWidget::propertySet(const QVariantHash &objects) {
    m_toast = qvariant_cast<ToastModule *>(objects["mainWindowToast"]);
    m_propertyDialog = qvariant_cast<QObject *>(objects["fileModulePropertyDialog"]);
    m_gotoDialog = qvariant_cast<QObject *>(objects["documentModuleGotoDialog"]);
    m_searchWidget->propertySet();
    shortcutInit();
    miscInit();
    indicatorInit();
    marginInit();
    markerInit();
    styleInit();
    lexerInit();
}

QString EditorWidget::documentSave() {
    if (!m_scintillaWidget->modifyGet()) return {};

    const auto documentPath = m_documentUrl.toLocalFile();
    QSaveFile documentFile(documentPath);
    if (!documentFile.open(QIODevice::WriteOnly)) {
        const auto error = tr("Document save failed: %1").arg(documentFile.errorString());
        emit appendLog(LogType::Error, "document save failed", error);
        return error;
    }

    QTextStream documentTextStream(&documentFile);
    documentTextStream << m_scintillaWidget->textGet();
    documentTextStream.flush();
    if (documentTextStream.status() != QTextStream::Ok) {
        documentFile.cancelWriting();
        const auto error = tr("Document save failed: unable to write the complete document.");
        emit appendLog(LogType::Error, "document save failed", error);
        return error;
    }
    if (!documentFile.commit()) {
        const auto error = tr("Document save failed: %1").arg(documentFile.errorString());
        emit appendLog(LogType::Error, "document save failed", error);
        return error;
    }

    m_scintillaWidget->savepointSet();
    emit appendLog(LogType::Info, "document saved", QString("<a href='%1'>%2</a>").arg(m_documentUrl.toString(), m_documentUrl.toString()));
    return {};
}

void EditorWidget::documentGoto() {
    m_gotoDialog->setProperty("documentUrl", m_documentUrl);
    m_gotoDialog->setProperty("line", m_selection["line"]);
    m_gotoDialog->setProperty("character", m_selection["character"]);
    QMetaObject::invokeMethod(m_gotoDialog, "open");
}

bool EditorWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_scintillaWidget && event->type() == QEvent::ShortcutOverride) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Undo)) {
            event->accept();
            if (!m_scintillaWidget->undoable()) {
                m_toast->show(ToastLevel::Info, tr("Undo"), tr("Nothing to undo."));
            }
            return true;
        }
        if (keyEvent->matches(QKeySequence::Redo)) {
            event->accept();
            if (!m_scintillaWidget->redoable()) {
                m_toast->show(ToastLevel::Info, tr("Redo"), tr("Nothing to redo."));
            }
            return true;
        }
    }
    if (watched == m_scintillaWidget && event->type() == QEvent::KeyPress) {
        const auto *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
            case Qt::Key_QuoteDbl:
                return symbolPair('"');
            case Qt::Key_Apostrophe:
                return symbolPair('\'');
            case Qt::Key_ParenLeft:
                return symbolPair('(');
            case Qt::Key_BracketLeft:
                return symbolPair('[');
            case Qt::Key_BraceLeft:
                return symbolPair('{');
            case Qt::Key_Escape:
                if (m_searchWidget->isVisible()) m_searchWidget->hide();
                return false;
            case Qt::Key_Backspace:
                if (m_scintillaWidget->selectionEmpty() && !m_scintillaWidget->atLineEnd()) {
                    return symbolPair('\b');
                }
                return false;
            default: return false;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// protected
void EditorWidget::shortcutInit() {
    auto shortcutDuplicate = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_D), this); // NOLINT
    shortcutDuplicate->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcutDuplicate, &QShortcut::activated, m_scintillaWidget, &ScintillaWidget::lineDuplicate);
    auto shortcutGoto = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_G), this); // NOLINT
    shortcutGoto->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcutGoto, &QShortcut::activated, this, &EditorWidget::documentGoto);
    auto shortcutSearch = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this); // NOLINT
    shortcutSearch->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcutSearch, &QShortcut::activated, this, &EditorWidget::searchShow);
    auto shortcutReplace = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), this); // NOLINT
    shortcutReplace->setContext(Qt::WidgetWithChildrenShortcut);
    connect(shortcutReplace, &QShortcut::activated, this, &EditorWidget::replaceShow);
}

void EditorWidget::selectionChange() {
    m_selection = m_scintillaWidget->selectionGet();
    emit changeSelection(m_selection);
}

void EditorWidget::contentChange() {
    emit changeContent();
}

bool EditorWidget::symbolPair(const QChar ch) {
    // handle backspace
    if (ch == '\b') {
        const auto line = m_selection["line"];
        const auto character = m_selection["character"];
        if (character > 0) {
            const auto prevChar = m_scintillaWidget->textGet(line, character - 1, line, character)[0];
            const auto nextChar = m_scintillaWidget->textGet(line, character, line, character + 1);
            if (m_pair.contains(prevChar) && !nextChar.isEmpty() && m_pair[prevChar] == nextChar) {
                m_scintillaWidget->textSet("", line, character - 1, line, character + 1);
                return true;
            }
        }
        return false;
    }
    // handle pair
    if (m_scintillaWidget->selectionEmpty()) {
        m_scintillaWidget->textSetSelected(QString(ch) + m_pair[ch]);
        const auto position = m_scintillaWidget->positionGet();
        m_scintillaWidget->positionSet(position - 1);
    }
    // handle surround
    else {
        m_scintillaWidget->textSetSelected(ch + m_scintillaWidget->textGetSelected() + m_pair[ch]);
    }
    return true;
}

void EditorWidget::miscInit() const {
    const auto general = g_globalManager->themeGet()["general"].toObject();
    // annotation
    m_scintillaWidget->send(SCI_ANNOTATIONSETVISIBLE, ANNOTATION_STANDARD); // NOLINT
    m_scintillaWidget->send(SCI_EOLANNOTATIONSETVISIBLE, ANNOTATION_STANDARD); // NOLINT
    // folding
    m_scintillaWidget->send(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CLICK | SC_AUTOMATICFOLD_CHANGE); // NOLINT
    m_scintillaWidget->send(SCI_SETFOLDMARGINCOLOUR, true, ScintillaWidget::colorGet(general["back"].toString())); // NOLINT
    m_scintillaWidget->send(SCI_SETFOLDMARGINHICOLOUR, true, ScintillaWidget::colorGet(general["back"].toString())); // NOLINT
    m_scintillaWidget->send(SCI_FOLDDISPLAYTEXTSETSTYLE, SC_FOLDDISPLAYTEXT_STANDARD); // NOLINT
    m_scintillaWidget->send(SCI_SETDEFAULTFOLDDISPLAYTEXT, 0, reinterpret_cast<sptr_t>("...")); // NOLINT
    // scrollbar
    m_scintillaWidget->send(SCI_SETSCROLLWIDTH, 1); // NOLINT
    m_scintillaWidget->send(SCI_SETSCROLLWIDTHTRACKING, true); // NOLINT
    // selection
    m_scintillaWidget->send(SCI_SETELEMENTCOLOUR, SC_ELEMENT_SELECTION_BACK, ScintillaWidget::colorGet(general["brandBack"].toString(), 128)); // NOLINT
    m_scintillaWidget->send(SCI_SETSELECTIONLAYER, SC_LAYER_UNDER_TEXT); // NOLINT
    m_scintillaWidget->send(SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET, ScintillaWidget::colorGet(general["fore"].toString(), 255)); // NOLINT
    m_scintillaWidget->send(SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK, ScintillaWidget::colorGet(general["backSelected"].toString(), 128)); // NOLINT
    m_scintillaWidget->send(SCI_SETCARETLINELAYER, SC_LAYER_UNDER_TEXT); // NOLINT
    // tab
    m_scintillaWidget->send(SCI_SETUSETABS, false); // NOLINT
    m_scintillaWidget->send(SCI_SETINDENT, 4); // NOLINT
    m_scintillaWidget->send(SCI_SETTABINDENTS, true); // NOLINT
    m_scintillaWidget->send(SCI_SETBACKSPACEUNINDENTS, true); // NOLINT
    m_scintillaWidget->send(SCI_SETINDENTATIONGUIDES, SC_IV_REAL); // NOLINT
}

void EditorWidget::indicatorInit() const {
    const auto general = g_globalManager->themeGet()["general"].toObject();
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Result,
        QVariantHash{
            {"style", INDIC_STRAIGHTBOX},
            {"fore", ScintillaWidget::colorGet(general["brandBack"].toString())},
            {"alpha", 128},
            {"outlineAlpha", 128},
            {"setUnder", true}
        });
    m_scintillaWidget->indicatorDefine(
        ScintillaIndicator::Current,
        QVariantHash{
            {"style", INDIC_STRAIGHTBOX},
            {"fore", ScintillaWidget::colorGet(general["fore"].toString())},
            {"strokeWidth", 200},
            {"alpha", 0},
            {"outlineAlpha", 255},
            {"setUnder", false}
        });
}

void EditorWidget::marginInit() const {
    m_scintillaWidget->marginDefine(
        0,
        QVariantHash{
            {"type", SC_MARGIN_NUMBER},
            {"width", 32}
        });
    m_scintillaWidget->marginDefine(
        1,
        QVariantHash{
            {"type", SC_MARGIN_SYMBOL},
            {"width", 16},
            {"mask", static_cast<int>(SC_MASK_FOLDERS)},
            {"sensitive", true},
            {"back", ScintillaWidget::colorGet(g_globalManager->themeGet()["general"].toObject()["back"].toString())}
        });
    m_scintillaWidget->marginDefine(
        2,
        QVariantHash{
            {"type", SC_MARGIN_SYMBOL},
            {"width", 4},
            {"mask", SC_MASK_HISTORY},
        });
}

void EditorWidget::markerInit() const {
    const auto general = g_globalManager->themeGet()["general"].toObject();
    m_scintillaWidget->markerDefine(
        ScintillaMarker::FolderEnd,
        QVariantHash{
            {"symbol", SC_MARK_BOXPLUSCONNECTED},
            {"fore", ScintillaWidget::colorGet(general["back"].toString())},
            {"back", ScintillaWidget::colorGet(general["stroke"].toString())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::FolderOpenMid,
        QVariantHash{
            {"symbol", SC_MARK_BOXMINUSCONNECTED},
            {"fore", ScintillaWidget::colorGet(general["back"].toString())},
            {"back", ScintillaWidget::colorGet(general["stroke"].toString())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::FolderMidTail,
        QVariantHash{
            {"symbol", SC_MARK_TCORNER},
            {"fore", ScintillaWidget::colorGet(general["back"].toString())},
            {"back", ScintillaWidget::colorGet(general["stroke"].toString())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::FolderTail,
        QVariantHash{
            {"symbol", SC_MARK_LCORNER},
            {"fore", ScintillaWidget::colorGet(general["back"].toString())},
            {"back", ScintillaWidget::colorGet(general["stroke"].toString())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::FolderSub,
        QVariantHash{
            {"symbol", SC_MARK_VLINE},
            {"fore", ScintillaWidget::colorGet(general["back"].toString())},
            {"back", ScintillaWidget::colorGet(general["stroke"].toString())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::Folder,
        QVariantHash{
            {"symbol", SC_MARK_BOXPLUS},
            {"fore", ScintillaWidget::colorGet(general["back"].toString())},
            {"back", ScintillaWidget::colorGet(general["stroke"].toString())}
        });
    m_scintillaWidget->markerDefine(
        ScintillaMarker::FolderOpen,
        QVariantHash{
            {"symbol", SC_MARK_BOXMINUS},
            {"fore", ScintillaWidget::colorGet(general["back"].toString())},
            {"back", ScintillaWidget::colorGet(general["stroke"].toString())}
        });
}

void EditorWidget::styleInit() const {
    const auto general = g_globalManager->themeGet()["general"].toObject();
    m_scintillaWidget->styleDefine(
        CustomStyle::Default,
        QVariantHash{
            {"font", m_config["fontFamily"].toString()},
            {"size", m_config["fontSize"].toInt()},
            {"fore", ScintillaWidget::colorGet(general["fore"].toString())},
            {"back", ScintillaWidget::colorGet(general["back"].toString())}
        });
    m_scintillaWidget->styleClearAll();
    m_scintillaWidget->styleDefine(
        CustomStyle::LineNumber,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(general["stroke"].toString())},
            {"back", ScintillaWidget::colorGet(general["back"].toString())}
        });
    m_scintillaWidget->styleDefine(
        CustomStyle::FoldDisplayText,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(general["fore"].toString())},
            {"back", ScintillaWidget::colorGet(general["backSelected"].toString())}
        });
    m_scintillaWidget->styleDefine(
        CustomStyle::Annotation,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(general["stroke"].toString())},
            {"back", ScintillaWidget::colorGet(general["back"].toString())}
        });
    m_scintillaWidget->styleDefine(
        CustomStyle::EolAnnotation,
        QVariantHash{
            {"fore", ScintillaWidget::colorGet(general["warningFore2"].toString())},
            {"italic", true}
        });
}

void EditorWidget::lexerInit() const {
    const QFileInfo documentInfo(m_documentUrl.toLocalFile());
    const auto suffix = documentInfo.suffix().toLower();
    const auto fileName = documentInfo.fileName().toLower();
    // html
    if (suffix == "html") {
        const auto htmlTheme = g_globalManager->themeGet()["html"].toObject();
        m_scintillaWidget->lexerSet("hypertext");
        m_scintillaWidget->send(SCI_SETPROPERTY, reinterpret_cast<sptr_t>("lexer.html.allow.php"), reinterpret_cast<sptr_t>("0"));
        m_scintillaWidget->send(SCI_SETPROPERTY, reinterpret_cast<sptr_t>("lexer.html.allow.asp"), reinterpret_cast<sptr_t>("0"));
        m_scintillaWidget->send(SCI_SETPROPERTY, reinterpret_cast<sptr_t>("fold.html"), reinterpret_cast<sptr_t>("1"));
        m_scintillaWidget->styleDefine(
            SCE_H_TAG,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["tag"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_TAGUNKNOWN,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["tagUnknown"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_ATTRIBUTE,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["attribute"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_ATTRIBUTEUNKNOWN,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["attributeUnknown"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_NUMBER,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["number"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_DOUBLESTRING,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["doubleString"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_SINGLESTRING,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["singleString"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_OTHER,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["other"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_COMMENT,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["comment"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_ENTITY,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["entity"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_TAGEND,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["tagEnd"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_H_VALUE,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(htmlTheme["value"].toObject()["fore"].toString())}
            });
    }
    // json
    else if (suffix == "json") {
        const auto jsonTheme = g_globalManager->themeGet()["json"].toObject();
        m_scintillaWidget->lexerSet("json");
        m_scintillaWidget->send(SCI_SETPROPERTY, reinterpret_cast<sptr_t>("lexer.json.escape.sequence"), reinterpret_cast<sptr_t>("1"));
        m_scintillaWidget->send(SCI_SETPROPERTY, reinterpret_cast<sptr_t>("lexer.json.allow.comments"), reinterpret_cast<sptr_t>("1"));
        m_scintillaWidget->send(SCI_SETKEYWORDS, 0, reinterpret_cast<sptr_t>("true false null"));
        m_scintillaWidget->styleDefine(
            SCE_JSON_NUMBER,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["number"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_STRING,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["string"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_STRINGEOL,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["stringEOL"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_PROPERTYNAME,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["propertyName"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_ESCAPESEQUENCE,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["escapeSequence"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_LINECOMMENT,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["lineComment"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_BLOCKCOMMENT,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["blockComment"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_OPERATOR,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["operator"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_URI,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["uri"].toObject()["fore"].toString())},
                {"underline", jsonTheme["uri"].toObject()["underline"].toBool()}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_COMPACTIRI,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["compactIri"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_KEYWORD,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["keyword"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_LDKEYWORD,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["ldKeyword"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_JSON_ERROR,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(jsonTheme["error"].toObject()["fore"].toString())}
            });
    }
    // markdown
    else if (suffix == "md") {
        const auto markdownTheme = g_globalManager->themeGet()["markdown"].toObject();
        m_scintillaWidget->lexerSet("markdown");
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_STRONG1,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["strong1"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_STRONG2,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["strong2"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_EM1,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["em1"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_EM2,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["em2"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_HEADER1,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["header1"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_HEADER2,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["header2"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_HEADER3,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["header3"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_HEADER4,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["header4"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_HEADER5,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["header5"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_HEADER6,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["header6"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_ULIST_ITEM,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["ulistItem"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_OLIST_ITEM,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["olistItem"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_BLOCKQUOTE,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["blockquote"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_STRIKEOUT,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["strikeout"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_HRULE,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["hRule"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_LINK,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["link"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_CODE,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["code"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_CODE2,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["code2"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_MARKDOWN_CODEBK,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(markdownTheme["codeBk"].toObject()["fore"].toString())}
            });
    }
    // powershell
    else if (suffix == "ps1") {
        const auto powershellTheme = g_globalManager->themeGet()["powershell"].toObject();
        m_scintillaWidget->lexerSet("powershell");
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_COMMENT,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["comment"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_STRING,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["string"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_CHARACTER,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["character"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_NUMBER,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["number"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_VARIABLE,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["variable"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_OPERATOR,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["operator"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_IDENTIFIER,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["identifier"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_KEYWORD,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["keyword"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_CMDLET,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["cmdlet"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_ALIAS,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["alias"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_FUNCTION,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["function"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_USER1,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["user1"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_COMMENTSTREAM,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["commentStream"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_HERE_STRING,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["hereString"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_HERE_CHARACTER,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["hereCharacter"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_POWERSHELL_COMMENTDOCKEYWORD,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(powershellTheme["commentDocKeyword"].toObject()["fore"].toString())}
            });
    }
    // toml
    else if (suffix == "toml") {
        const auto tomlTheme = g_globalManager->themeGet()["toml"].toObject();
        m_scintillaWidget->lexerSet("toml");
        m_scintillaWidget->styleDefine(
            SCE_TOML_COMMENT,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["comment"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_IDENTIFIER,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["identifier"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_KEYWORD,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["keyword"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_NUMBER,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["number"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_TABLE,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["table"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_KEY,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["key"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_ERROR,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["error"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_OPERATOR,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["operator"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_STRING_SQ,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["stringSQ"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_STRING_DQ,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["stringDQ"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_TRIPLE_STRING_SQ,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["tripleStringSQ"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_TRIPLE_STRING_DQ,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["tripleStringDQ"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_ESCAPECHAR,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["escapeChar"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_DATETIME,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["datetime"].toObject()["fore"].toString())}
            });
        m_scintillaWidget->styleDefine(
            SCE_TOML_STRINGEOL,
            QVariantHash{
                {"fore", ScintillaWidget::colorGet(tomlTheme["stringEOL"].toObject()["fore"].toString())}
            });
    }
    // more to go
    else {
        return;
    }
    m_scintillaWidget->send(SCI_SETPROPERTY, reinterpret_cast<sptr_t>("fold"), reinterpret_cast<sptr_t>("1"));
    m_scintillaWidget->send(SCI_COLOURISE, 0, -1); // NOLINT
}

void EditorWidget::searchShow() const {
    m_searchWidget->searchShow(m_scintillaWidget->textGetSelected());
    m_searchWidget->show();
}

void EditorWidget::replaceShow() const {
    m_searchWidget->replaceShow(m_scintillaWidget->textGetSelected());
    m_searchWidget->show();
}

// private
void EditorWidget::documentOpen() const {
    // text get
    const auto documentPath = m_documentUrl.toLocalFile();
    auto documentFile = QFile(documentPath);
    if (!documentFile.open(QIODevice::ReadOnly)) return;
    auto documentTextStream = QTextStream(&documentFile);
    const auto documentText = documentTextStream.readAll();
    documentFile.close();
    m_scintillaWidget->textSet(documentText);
    // permission set
    const auto documentInfo = QFileInfo(documentPath);
    m_scintillaWidget->readonlySet(!documentInfo.isWritable());
    // history set
    m_scintillaWidget->send(SCI_EMPTYUNDOBUFFER); // NOLINT
    m_scintillaWidget->send(SCI_SETCHANGEHISTORY,SC_CHANGE_HISTORY_ENABLED | SC_CHANGE_HISTORY_MARKERS); // NOLINT
}

void EditorWidget::permissionSet() const {
    m_propertyDialog->setProperty("documentUrl", m_documentUrl);
    QMetaObject::invokeMethod(m_propertyDialog, "permission");
    QMetaObject::invokeMethod(m_propertyDialog, "open");
}

// private: search
void EditorWidget::searchRequest(const QString &text) {
    searchClear();
    if (!text.isEmpty()) {
        int current = 0;

        m_scintillaWidget->targetSetWhole();
        while (true) {
            if (m_scintillaWidget->targetSearch(text) == -1) break;
            const auto range = ScintillaWidget::PositionRange{
                m_scintillaWidget->targetGetStart(),
                m_scintillaWidget->targetGetEnd()
            };
            m_search.matches.append(range);
            const auto indexRange = m_scintillaWidget->cast<ScintillaWidget::Utf16Index>(range);
            m_scintillaWidget->indicatorFill(
                ScintillaIndicator::Result,
                indexRange.start.line,
                indexRange.start.character,
                indexRange.end.line,
                indexRange.end.character
            );
            if (m_selection["startPosition"] > range.start) current++;
            m_scintillaWidget->targetSetStart(range.end);
            m_scintillaWidget->targetSetEnd(m_scintillaWidget->lengthGet());
        }
        const auto total = static_cast<int>(m_search.matches.size());
        if (current == total && total > 0) current--;
        m_search.current = current;
    }
    searchResponse();
}

void EditorWidget::searchResponse() {
    const auto total = static_cast<int>(m_search.matches.size());
    if (total == 0) {
        m_searchWidget->searchEnable(false);
        m_searchWidget->replaceEnable(false);
        m_searchWidget->searchResponse("0/0");
        return;
    }
    m_searchWidget->searchEnable(true);
    m_searchWidget->replaceEnable(true);
    const auto current = m_search.current;
    m_searchWidget->searchResponse(QString("%1/%2").arg(QString::number(current + 1), QString::number(total)));
    const auto indexRange = m_scintillaWidget->cast<ScintillaWidget::Utf16Index>(m_search.matches[current]);
    m_scintillaWidget->indexSet(
        indexRange.start.line,
        indexRange.start.character
    );
    m_scintillaWidget->indicatorFill(
        ScintillaIndicator::Current,
        indexRange.start.line,
        indexRange.start.character,
        indexRange.end.line,
        indexRange.end.character
    );
}

void EditorWidget::searchPrev() {
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Current);
    const auto total = static_cast<int>(m_search.matches.size());
    if (m_search.current != 0) m_search.current--;
    else m_search.current = total - 1;
    searchResponse();
}

void EditorWidget::searchNext() {
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Current);
    const auto total = static_cast<int>(m_search.matches.size());
    if (m_search.current != total - 1) m_search.current++;
    else m_search.current = 0;
    searchResponse();
}

void EditorWidget::searchClear() {
    m_search = {};
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Result);
    m_scintillaWidget->indicatorClear(ScintillaIndicator::Current);
}

void EditorWidget::textReplace(const QString &text) {
    const auto indexRange = m_scintillaWidget->cast<ScintillaWidget::Utf16Index>(m_search.matches[m_search.current]);
    m_scintillaWidget->indexSet(
        indexRange.start.line,
        indexRange.start.character
    );
    m_scintillaWidget->textSet(
        text,
        indexRange.start.line,
        indexRange.start.character,
        indexRange.end.line,
        indexRange.end.character
    );
    m_searchWidget->searchRequest();
}

void EditorWidget::allReplace(const QString &text) {
    m_scintillaWidget->undoBegin();
    for (int index = static_cast<int>(m_search.matches.size()) - 1; index >= 0; --index) {
        const auto indexRange = m_scintillaWidget->cast<ScintillaWidget::Utf16Index>(m_search.matches[index]);
        m_scintillaWidget->textSet(
            text,
            indexRange.start.line,
            indexRange.start.character,
            indexRange.end.line,
            indexRange.end.character
        );
    }
    m_scintillaWidget->undoEnd();
    m_searchWidget->searchRequest();
}
