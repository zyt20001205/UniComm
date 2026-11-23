#include "document/module/scintillaWidget.h"

#include <ILexer.h>
#include <Lexilla.h>
#include <QFile>
#include <QTimer>

#include "types/scintillaTypes.h"

using namespace Scintilla;

// public
ScintillaWidget::ScintillaWidget(QWidget *parent)
    : ScintillaEdit(parent) {
    setContextMenuPolicy(Qt::NoContextMenu);
    setFrameStyle(NoFrame);
}

// public: file
ScintillaCodePage::Value ScintillaWidget::codePageGet() const {
    return static_cast<ScintillaCodePage::Value>(send(SCI_GETCODEPAGE));
}

ScintillaEolMode::Value ScintillaWidget::eolModeGet() const {
    return static_cast<ScintillaEolMode::Value>(send(SCI_GETEOLMODE));
}

void ScintillaWidget::eolModeSet(const ScintillaEolMode::Value eolMode) const {
    send(SCI_SETEOLMODE, eolMode); // NOLINT
    send(SCI_CONVERTEOLS, eolMode); // NOLINT
}

bool ScintillaWidget::eolViewGet() const {
    return send(SCI_GETVIEWEOL);
}

void ScintillaWidget::eolViewSet(const bool status) const {
    send(SCI_SETVIEWEOL, status); // NOLINT
}

bool ScintillaWidget::useTabsGet() const {
    return send(SCI_GETUSETABS);
}

int ScintillaWidget::indentWidthGet() const {
    const auto width = send(SCI_GETINDENT);
    return static_cast<int>(width == 0 ? send(SCI_GETTABWIDTH) : width);
}

bool ScintillaWidget::modifyGet() const {
    return send(SCI_GETMODIFY);
}

void ScintillaWidget::readonlySet(const bool status) const {
    send(SCI_SETREADONLY, status); // NOLINT
}

bool ScintillaWidget::readonlyGet() const {
    return send(SCI_GETREADONLY);
}

void ScintillaWidget::savepointSet() const {
    send(SCI_SETSAVEPOINT); // NOLINT
}

// color
int ScintillaWidget::colorGet(const QString &color) {
    const auto _color = QColor(color);
    return _color.red() | _color.green() << 8 | _color.blue() << 16;
}

int ScintillaWidget::colorGet(const QString &color, const int alpha) {
    const auto _color = QColor(color);
    return _color.red() | _color.green() << 8 | _color.blue() << 16 | alpha << 24;
}

// public: annotation
void ScintillaWidget::annotationClear() const {
    send(SCI_ANNOTATIONCLEARALL); // NOLINT
}

void ScintillaWidget::annotationSet(const int line, const QString &annotation) const {
    send(SCI_ANNOTATIONSETTEXT, line, reinterpret_cast<sptr_t>(annotation.toUtf8().constData()));
    send(SCI_ANNOTATIONSETSTYLE, line, CustomStyle::Annotation); // NOLINT
}

void ScintillaWidget::eolAnnotationClear() const {
    send(SCI_EOLANNOTATIONCLEARALL); // NOLINT
}

void ScintillaWidget::eolAnnotationSet(const int line, const QString &annotation) const {
    send(SCI_EOLANNOTATIONSETTEXT, line, reinterpret_cast<sptr_t>(annotation.toUtf8().constData()));
    send(SCI_EOLANNOTATIONSETSTYLE, line, CustomStyle::EolAnnotation); // NOLINT
}

void ScintillaWidget::focusSet(const bool status) const {
    send(SCI_SETFOCUS, status); // NOLINT
}

// public: fold
void ScintillaWidget::foldLevelSet(const int line, const int level) const {
    send(SCI_SETFOLDLEVEL, line, level); // NOLINT
}

void ScintillaWidget::foldContractTop() const {
    send(SCI_FOLDALL, SC_FOLDACTION_CONTRACT); // NOLINT
}

void ScintillaWidget::foldContractRecursively() const {
    send(SCI_FOLDALL, SC_FOLDACTION_CONTRACT | SC_FOLDACTION_CONTRACT_EVERY_LEVEL); // NOLINT
}

void ScintillaWidget::foldExpandRecursively() const {
    send(SCI_FOLDALL, SC_FOLDACTION_EXPAND); // NOLINT
}

// public: height
int ScintillaWidget::heightGet() const {
    return static_cast<int>(send(SCI_TEXTHEIGHT, 0));
}

ScintillaWidget::Utf16IndexRange ScintillaWidget::wordIndexGet(Position position, const bool onlyWordCharacters) const {
    if (position == -1) position = positionResolve(-1, -1);
    return cast<Utf16Index>(PositionRange{
        send(SCI_WORDSTARTPOSITION, position, onlyWordCharacters),
        send(SCI_WORDENDPOSITION, position, onlyWordCharacters)
    });
}

void ScintillaWidget::indexSet(const int line, const int character) const {
    positionSet(positionResolve(line, character));
}

// public: indicator
void ScintillaWidget::indicatorDefine(const ScintillaIndicator::Value type, const QVariantHash &session) const {
    if (session.contains("style")) send(SCI_INDICSETSTYLE, type, session["style"].toInt()); // NOLINT
    if (session.contains("fore")) send(SCI_INDICSETFORE, type, session["fore"].toInt()); // NOLINT
    if (session.contains("strokeWidth")) send(SCI_INDICSETSTROKEWIDTH, type, session["strokeWidth"].toInt()); // NOLINT
    if (session.contains("alpha")) send(SCI_INDICSETALPHA, type, session["alpha"].toInt()); // NOLINT
    if (session.contains("outlineAlpha")) send(SCI_INDICSETOUTLINEALPHA, type, session["outlineAlpha"].toInt()); // NOLINT
    if (session.contains("setUnder")) send(SCI_INDICSETUNDER, type, session["setUnder"].toBool()); // NOLINT
    if (session.contains("hoverStyle")) send(SCI_INDICSETHOVERSTYLE, type, session["hoverStyle"].toInt()); // NOLINT
    // if (session.contains("hoverFore")) send(SCI_INDICSETHOVERFORE, type, session["hoverFore"].toInt()); // NOLINT
    // if (session.contains("flags")) send(SCI_INDICSETFLAGS, type, session["flags"].toInt()); // NOLINT
}

void ScintillaWidget::indicatorFill(const ScintillaIndicator::Value type, const int startLine, const int startCharacter, const int endLine, const int endCharacter, const int time) const {
    const Position start = positionResolve(startLine, startCharacter);
    const Position end = positionResolve(endLine, endCharacter);
    const Position length = end - start;
    if (length <= 0) return;
    send(SCI_SETINDICATORCURRENT, type); // NOLINT
    send(SCI_INDICATORFILLRANGE, start, length); // NOLINT
    if (time == -1) return;
    QTimer::singleShot(time, this, [this, type, startLine, startCharacter, endLine, endCharacter] { indicatorClear(type, startLine, startCharacter, endLine, endCharacter); });
}

void ScintillaWidget::indicatorClear(const ScintillaIndicator::Value type, const int startLine, const int startCharacter, const int endLine, const int endCharacter) const {
    Position start{};
    Position lengthFill;
    if (startLine != -1) {
        start = positionResolve(startLine, startCharacter);
        lengthFill = positionResolve(endLine, endCharacter) - start;
    } else {
        lengthFill = send(SCI_GETTEXTLENGTH);
    }
    send(SCI_SETINDICATORCURRENT, type); // NOLINT
    send(SCI_INDICATORCLEARRANGE, start, lengthFill); // NOLINT
}

int ScintillaWidget::indicatorGet(const Position position) const {
    return static_cast<int>(send(SCI_INDICATORALLONFOR, position));
}

// public: length
Position ScintillaWidget::lengthGet() const {
    return send(SCI_GETLENGTH);
}

// lexer
void ScintillaWidget::lexerSet(const char *name) const {
    auto *lexer = CreateLexer(name);
    if (!lexer) return;
    send(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
}

// public: line
int ScintillaWidget::lineCountGet() const {
    return static_cast<int>(send(SCI_GETLINECOUNT));
}

void ScintillaWidget::lineDuplicate() const {
    send(SCI_LINEDUPLICATE); // NOLINT
}

int ScintillaWidget::lineGet(const Position position) const {
    return static_cast<int>(send(SCI_LINEFROMPOSITION, position));
}

QString ScintillaWidget::linesGet(const int startLine, const int lineCount) const {
    const auto endLine = lineCount == -1 ? lineCountGet() - 1 : startLine + lineCount - 1;
    auto text = textGet(startLine, 0, endLine, -1);
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    return text;
}

void ScintillaWidget::linesSet(const QStringList &texts, const QList<int> &startLines, const QList<int> &lineCounts) const {
    const auto eolMode = eolModeGet();
    undoBegin();
    for (qsizetype index = 0; index < texts.size(); ++index) {
        const auto startLine = startLines.at(index);
        const auto lineCount = lineCounts.at(index);
        const auto editorLineCount = lineCountGet();
        if (startLine >= editorLineCount) continue;

        const auto endLine = lineCount == -1 ? editorLineCount : qMin(startLine + lineCount, editorLineCount);
        auto replacement = texts.at(index);
        if (!replacement.isEmpty() && endLine < editorLineCount && !replacement.endsWith('\n') && !replacement.endsWith('\r')) replacement.append('\n');

        if (endLine < editorLineCount) textSet(replacement, startLine, 0, endLine, 0);
        else textSet(replacement, startLine, 0, editorLineCount - 1, -1);
    }
    eolModeSet(eolMode);
    undoEnd();
}

// public: margin
void ScintillaWidget::marginDefine(const int type, const QVariantHash &session) const {
    if (session.contains("type")) send(SCI_SETMARGINTYPEN, type, session["type"].toInt()); // NOLINT
    if (session.contains("width")) send(SCI_SETMARGINWIDTHN, type, session["width"].toInt()); // NOLINT
    if (session.contains("mask")) send(SCI_SETMARGINMASKN, type, session["mask"].toInt()); // NOLINT
    if (session.contains("sensitive")) send(SCI_SETMARGINSENSITIVEN, type, session["sensitive"].toBool()); // NOLINT
    // if (session.contains("cursor")) send(SCI_SETMARGINCURSORN, type, session["cursor"].toInt()); // NOLINT
    if (session.contains("back")) send(SCI_SETMARGINBACKN, type, session["back"].toInt()); // NOLINT
    // if (session.contains("left")) send(SCI_SETMARGINLEFT, type, session["left"].toInt()); // NOLINT
    // if (session.contains("right")) send(SCI_SETMARGINRIGHT, type, session["right"].toInt()); // NOLINT
    // if (session.contains("text")) send(SCI_MARGINSETTEXT, type, reinterpret_cast<sptr_t>(session["text"].toString().toUtf8().constData())); // NOLINT
    // if (session.contains("style")) send(SCI_MARGINSETSTYLE, type, session["style"].toInt()); // NOLINT
    // if (session.contains("styleOffset")) send(SCI_MARGINSETSTYLEOFFSET, type); // NOLINT
    // if (session.contains("options")) send(SCI_SETMARGINOPTIONS, type, session["options"].toInt()); // NOLINT
}

void ScintillaWidget::marginTextSet(const int line, const QString &text) const {
    send(SCI_MARGINSETTEXT, line, reinterpret_cast<sptr_t>(text.toUtf8().constData())); // NOLINT
}

int ScintillaWidget::marginCountGet() const {
    return send(SCI_GETMARGINS); // NOLINT
}

int ScintillaWidget::marginWidthGet(const int col) const {
    if (col == -1) {
        int width = 0;
        for (int current = 0; current < marginCountGet(); ++current) {
            width += send(SCI_GETMARGINWIDTHN, current);
        }
        return width;
    }
    return static_cast<int>(send(SCI_GETMARGINWIDTHN, col));
}

// public: marker
void ScintillaWidget::markerDefine(const ScintillaMarker::Value type, const QVariantHash &session) const {
    if (session.contains("symbol")) send(SCI_MARKERDEFINE, type, session["symbol"].toInt()); // NOLINT
    if (session.contains("fore")) send(SCI_MARKERSETFORE, type, session["fore"].toInt()); // NOLINT
    // if (session.contains("foreTranslucent")) send(SCI_MARKERSETFORETRANSLUCENT, type, session["foreTranslucent"].toInt()); // NOLINT
    if (session.contains("back")) send(SCI_MARKERSETBACK, type, session["back"].toInt()); // NOLINT
    // if (session.contains("backTranslucent")) send(SCI_MARKERSETBACKTRANSLUCENT, type, session["backTranslucent"].toInt()); // NOLINT
    // if (session.contains("backSelected")) send(SCI_MARKERSETBACKSELECTED, type, session["backSelected"].toInt()); // NOLINT
    // if (session.contains("backSelectedTranslucent")) send(SCI_MARKERSETBACKSELECTEDTRANSLUCENT, type, session["backSelectedTranslucent"].toInt()); // NOLINT
    // if (session.contains("strokeWidth")) send(SCI_MARKERSETSTROKEWIDTH, type, session["strokeWidth"].toInt()); // NOLINT
    // if (session.contains("enableHighlight")) send(SCI_MARKERENABLEHIGHLIGHT, type, session["enableHighlight"].toBool()); // NOLINT
    // if (session.contains("layer")) send(SCI_MARKERSETLAYER, type, session["layer"].toInt()); // NOLINT
    // if (session.contains("alpha")) send(SCI_MARKERSETALPHA, type, session["alpha"].toInt()); // NOLINT
}

void ScintillaWidget::markerAdd(const ScintillaMarker::Value type, const int line, const int time) const {
    send(SCI_MARKERADD, line, type); // NOLINT
    if (type != ScintillaMarker::Debug && type != ScintillaMarker::DebugLine)
        send(SCI_ENSUREVISIBLE, line); // NOLINT
    if (time == -1) return;
    QTimer::singleShot(time, this, [this, type, line] { markerDelete(type, line); });
}

void ScintillaWidget::markerDelete(const ScintillaMarker::Value type, const int line) const {
    if (line == -1) send(SCI_MARKERDELETEALL, type); // NOLINT
    else send(SCI_MARKERDELETE, line, type); // NOLINT
}

int ScintillaWidget::markerGet(const int line) const {
    return static_cast<int>(send(SCI_MARKERGET, line));
}

bool ScintillaWidget::atLineEnd() const {
    const Position position = positionResolve(-1, -1);
    const Position lineEndPosition = send(SCI_GETLINEENDPOSITION, lineGet(position));
    return position == lineEndPosition;
}

// public: position
Position ScintillaWidget::positionGet(const int line, const int character) const {
    return positionResolve(line, character);
}

Position ScintillaWidget::closePositionGet(const QPoint &point) const {
    return send(SCI_POSITIONFROMPOINTCLOSE, point.x(), point.y());
}

void ScintillaWidget::positionSet(const Position position) const {
    send(SCI_GOTOPOS, position); // NOLINT
}

// public: search
void ScintillaWidget::searchFlagsSet(const bool matchCase, const bool wholeWord, const bool wordStart, const bool regExp) const {
    auto flags = SCFIND_NONE;
    if (matchCase) flags |= SCFIND_MATCHCASE;
    if (wholeWord) flags |= SCFIND_WHOLEWORD;
    if (wordStart) flags |= SCFIND_WORDSTART;
    if (regExp) flags |= SCFIND_REGEXP;
    send(SCI_SETSEARCHFLAGS, flags); // NOLINT
}

Position ScintillaWidget::targetGetStart() const {
    return send(SCI_GETTARGETSTART);
}

void ScintillaWidget::targetSetStart(const Position position) const {
    send(SCI_SETTARGETSTART, position); // NOLINT
}

Position ScintillaWidget::targetGetEnd() const {
    return send(SCI_GETTARGETEND);
}

void ScintillaWidget::targetSetEnd(const Position position) const {
    send(SCI_SETTARGETEND, position); // NOLINT
}

void ScintillaWidget::targetSetWhole() const {
    send(SCI_TARGETWHOLEDOCUMENT); // NOLINT
}

Position ScintillaWidget::targetSearch(const QString &text) const {
    const auto utf8 = text.toUtf8();
    return send(SCI_SEARCHINTARGET, utf8.size(), reinterpret_cast<uptr_t>(utf8.constData())); // NOLINT
}

// public: selection
QHash<QString, int> ScintillaWidget::selectionGet() const {
    const auto index = cast<Utf16Index>(positionResolve(-1, -1));
    const Position startPosition{send(SCI_GETSELECTIONSTART)};
    const Position endPosition{send(SCI_GETSELECTIONEND)};
    const int characters = static_cast<int>(send(SCI_COUNTCHARACTERS, startPosition, endPosition));
    const auto startIndex = cast<Utf16Index>(startPosition);
    const auto endIndex = cast<Utf16Index>(endPosition);
    return QHash<QString, int>{
        {"line", index.line},
        {"character", index.character},
        {"startLine", startIndex.line},
        {"startCharacter", startIndex.character},
        {"startPosition", static_cast<int>(startPosition)},
        {"endLine", endIndex.line},
        {"endCharacter", endIndex.character},
        {"lines", endIndex.line - startIndex.line},
        {"characters", characters}
    };
}

void ScintillaWidget::selectionSet(const int startLine, const int startCharacter, const int endLine, const int endCharacter) const {
    const Position anchor = positionResolve(startLine, startCharacter);
    const Position caret = positionResolve(endLine, endCharacter);
    send(SCI_SETSEL, anchor, caret); // NOLINT
}

// public: style
void ScintillaWidget::styleDefine(const int type, const QVariantHash &session) const {
    if (session.contains("font")) send(SCI_STYLESETFONT, type, reinterpret_cast<sptr_t>(session["font"].toString().toUtf8().constData())); // NOLINT
    if (session.contains("size")) send(SCI_STYLESETSIZE, type, session["size"].toInt()); // NOLINT
    // if (session.contains("bold")) send(SCI_STYLESETBOLD, type, session["bold"].toBool()); // NOLINT
    // if (session.contains("weight")) send(SCI_STYLESETWEIGHT, type, session["weight"].toInt()); // NOLINT
    // if (session.contains("stretch")) send(SCI_STYLESETSTRETCH, type, session["stretch"].toInt()); // NOLINT
    if (session.contains("italic")) send(SCI_STYLESETITALIC, type, session["italic"].toBool()); // NOLINT
    if (session.contains("underline")) send(SCI_STYLESETUNDERLINE, type, session["underline"].toBool()); // NOLINT
    if (session.contains("fore")) send(SCI_STYLESETFORE, type, session["fore"].toInt()); // NOLINT    if (session.contains("fore")) send(SCI_STYLESETFORE, type, session["fore"].toInt()); // NOLINT
    if (session.contains("back")) send(SCI_STYLESETBACK, type, session["back"].toInt()); // NOLINT
    // if (session.contains("eolFilled")) send(SCI_STYLESETEOLFILLED, type, session["eolFilled"].toBool()); // NOLINT
    // if (session.contains("characterSet")) send(SCI_STYLESETCHARACTERSET, type, session["characterSet"].toInt()); // NOLINT
    // if (session.contains("case")) send(SCI_STYLESETCASE, type, session["case"].toInt()); // NOLINT
    // if (session.contains("visible")) send(SCI_STYLESETVISIBLE, type, session["visible"].toBool()); // NOLINT
    // if (session.contains("changeable")) send(SCI_STYLESETCHANGEABLE, type, session["changeable"].toBool()); // NOLINT
    // if (session.contains("hotspot")) send(SCI_STYLESETHOTSPOT, type, session["hotspot"].toBool()); // NOLINT
    // if (session.contains("checkMonospaced")) send(SCI_STYLESETCHECKMONOSPACED, type, session["checkMonospaced"].toBool()); // NOLINT
    // if (session.contains("representation")) send(SCI_STYLESETINVISIBLEREPRESENTATION, type, reinterpret_cast<sptr_t>(session["bold"].toString().toUtf8().constData())); // NOLINT
    // if (session.contains("locale")) send(SCI_SETFONTLOCALE, type, reinterpret_cast<sptr_t>(session["locale"].toString().toUtf8().constData())); // NOLINT
}

int ScintillaWidget::styleGet(const Position position) const {
    return send(SCI_GETSTYLEAT, position); // NOLINT
}

void ScintillaWidget::styleSet(const int type, const int startLine, const int startCharacter, int length) const {
    Position start{};
    if (startLine != -1) {
        start = positionResolve(startLine, startCharacter);
        const Position end = positionResolve(startLine, startCharacter + length);
        length = static_cast<int>(end - start);
    } else {
        length = static_cast<int>(lengthGet());
    }
    if (start < 0 || start + length > lengthGet() || length <= 0) {
        // qDebug() << "long string skipped (this is lua language server bug)";
        return;
    }
    send(SCI_STARTSTYLING, start); // NOLINT
    send(SCI_SETSTYLING, length, type); // NOLINT
}

// public: text
void ScintillaWidget::textAppend(const QString &text) const {
    const auto ba = text.toUtf8();
    send(SCI_APPENDTEXT, ba.size(), reinterpret_cast<sptr_t>(ba.constData()));
}

void ScintillaWidget::textClear() const {
    send(SCI_CLEARALL); // NOLINT
}

QString ScintillaWidget::textGet(const int startLine, const int startCharacter, const int endLine, const int endCharacter) const {
    Position startPosition{};
    Position endPosition{};
    Position length{};
    if (startLine == -1) {
        endPosition = lengthGet();
        length = lengthGet();
    } else {
        const auto lineCount = lineCountGet();
        if (startLine < 0 || startLine >= lineCount || endLine < startLine) return {};
        startPosition = positionResolve(startLine, startCharacter);
        endPosition = positionResolve(qMin(endLine, lineCount - 1), endCharacter);
        length = endPosition - startPosition;
    }
    if (length < 0 || length > static_cast<int>(lengthGet())) return {};
    QByteArray buffer(static_cast<int>(length + 1), '\0');
    Sci_TextRange tr = {{static_cast<int>(startPosition), static_cast<int>(endPosition)}, buffer.data()};
    send(SCI_GETTEXTRANGE, 0, reinterpret_cast<sptr_t>(&tr));
    buffer.chop(1); // Remove extra NUL
    return QString::fromUtf8(buffer.constData(), static_cast<int>(length));
}

QString ScintillaWidget::textGetSelected() const {
    const int length = static_cast<int>(send(SCI_GETSELTEXT, 0, NULL));
    QByteArray buffer(length + 1, '\0');
    send(SCI_GETSELTEXT, 0, reinterpret_cast<sptr_t>(buffer.data()));
    buffer.chop(1); // Remove extra NUL
    return QString::fromUtf8(buffer.constData(), length);
}

void ScintillaWidget::textSet(const QString &text, const int startLine, const int startCharacter, const int endLine, const int endCharacter) const {
    if (startLine == -1) {
        send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(text.toUtf8().constData())); // NOLINT
    } else {
        selectionSet(startLine, startCharacter, endLine, endCharacter);
        textSetSelected(text);
    }
}

void ScintillaWidget::textSetSelected(const QString &text) const {
    send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(text.toUtf8().constData()));
}

// public: edit
bool ScintillaWidget::copiable() const {
    return !send(SCI_GETSELECTIONEMPTY);
}

bool ScintillaWidget::pastable() const {
    return send(SCI_CANPASTE);
}

bool ScintillaWidget::undoable() const {
    return send(SCI_CANUNDO);
}

bool ScintillaWidget::redoable() const {
    return send(SCI_CANREDO);
}

void ScintillaWidget::undoBegin() const {
    send(SCI_BEGINUNDOACTION); // NOLINT
}

void ScintillaWidget::undoEnd() const {
    send(SCI_ENDUNDOACTION); // NOLINT
}

// private:
Position ScintillaWidget::positionFrom(const Utf8Index &index) const {
    const int line = qBound(0, index.line, lineCountGet() - 1);
    const Position lineStart = send(SCI_POSITIONFROMLINE, line);
    const Position lineEnd = send(SCI_GETLINEENDPOSITION, line);
    const Position offset = qBound(Position{}, static_cast<Position>(index.character), lineEnd - lineStart);
    return lineStart + offset;
}

Position ScintillaWidget::positionFrom(const Utf16Index &index) const {
    const int line = qBound(0, index.line, lineCountGet() - 1);
    const Position lineStart = send(SCI_POSITIONFROMLINE, line);
    const Position lineEnd = send(SCI_GETLINEENDPOSITION, line);
    const Position lineLength = send(SCI_COUNTCODEUNITS, lineStart, lineEnd);
    const Position offset = qBound(Position{}, static_cast<Position>(index.character), lineLength);
    return send(SCI_POSITIONRELATIVECODEUNITS, lineStart, offset);
}

Position ScintillaWidget::positionFrom(const Utf32Index &index) const {
    const int line = qBound(0, index.line, lineCountGet() - 1);
    const Position lineStart = send(SCI_POSITIONFROMLINE, line);
    const Position lineEnd = send(SCI_GETLINEENDPOSITION, line);
    const Position lineLength = send(SCI_COUNTCHARACTERS, lineStart, lineEnd);
    const Position offset = qBound(Position{}, static_cast<Position>(index.character), lineLength);
    return send(SCI_POSITIONRELATIVE, lineStart, offset);
}

Position ScintillaWidget::positionFrom(const ViewportPoint &point) const {
    return send(SCI_POSITIONFROMPOINT, point.value.x(), point.value.y());
}

Position ScintillaWidget::positionResolve(const int line, const int character) const {
    if (line == -1) return send(SCI_GETCURRENTPOS);
    if (character == -1) return send(SCI_GETLINEENDPOSITION, line);
    return cast<Position>(Utf16Index{line, character});
}

ScintillaWidget::Utf8Index ScintillaWidget::utf8IndexFrom(const Position position) const {
    const Position boundedPosition = qBound(Position{}, position, lengthGet());
    const int line = static_cast<int>(send(SCI_LINEFROMPOSITION, boundedPosition));
    const Position lineStart = send(SCI_POSITIONFROMLINE, line);
    return Utf8Index{line, static_cast<int>(boundedPosition - lineStart)};
}

ScintillaWidget::Utf16Index ScintillaWidget::utf16IndexFrom(const Position position) const {
    const Position boundedPosition = qBound(Position{}, position, lengthGet());
    const int line = static_cast<int>(send(SCI_LINEFROMPOSITION, boundedPosition));
    const Position lineStart = send(SCI_POSITIONFROMLINE, line);
    const int character = static_cast<int>(send(SCI_COUNTCODEUNITS, lineStart, boundedPosition));
    return Utf16Index{line, character};
}

ScintillaWidget::Utf32Index ScintillaWidget::utf32IndexFrom(const Position position) const {
    const Position boundedPosition = qBound(Position{}, position, lengthGet());
    const int line = static_cast<int>(send(SCI_LINEFROMPOSITION, boundedPosition));
    const Position lineStart = send(SCI_POSITIONFROMLINE, line);
    const int character = static_cast<int>(send(SCI_COUNTCHARACTERS, lineStart, boundedPosition));
    return Utf32Index{line, character};
}

ScintillaWidget::ViewportPoint ScintillaWidget::viewportPointFrom(const Position position) const {
    const Position boundedPosition = qBound(Position{}, position, lengthGet());
    const int x = static_cast<int>(send(SCI_POINTXFROMPOSITION, 0, boundedPosition));
    const int y = static_cast<int>(send(SCI_POINTYFROMPOSITION, 0, boundedPosition));
    return ViewportPoint{QPoint{x, y}};
}
