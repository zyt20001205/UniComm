#ifndef UNICOMM_SCINTILLAWIDGET_H
#define UNICOMM_SCINTILLAWIDGET_H

#include <ScintillaEdit.h>
#include <type_traits>

#include "types/scintillaTypes.h"

class QQuickWidget;

class ScintillaWidget final : public ScintillaEdit {
    Q_OBJECT

public:
    struct Utf8Index {
        int line{};
        int character{};
    };

    struct Utf16Index {
        int line{};
        int character{};
    };

    struct Utf32Index {
        int line{};
        int character{};
    };

    template<typename T>
    struct Range {
        T start{};
        T end{};
    };

    using PositionRange = Range<Scintilla::Position>;
    using Utf8IndexRange = Range<Utf8Index>;
    using Utf16IndexRange = Range<Utf16Index>;
    using Utf32IndexRange = Range<Utf32Index>;

    struct ViewportPoint {
        QPoint value{};
    };

    struct GlobalPoint {
        QPoint value{};
    };

    template<typename To, typename From>
    [[nodiscard]] Range<To> cast(const Range<From> &from) const {
        return Range<To>{
            cast<To>(from.start),
            cast<To>(from.end)
        };
    }

    template<typename To, typename From>
    [[nodiscard]] To cast(const From &from) const {
        static_assert(coordinateType<To> && coordinateType<From>);

        if constexpr (std::is_same_v<To, From>) {
            return from;
        } else if constexpr (pointType<To> && pointType<From>) {
            return pointCast<To>(from);
        } else if constexpr (std::is_same_v<From, GlobalPoint>) {
            return cast<To>(cast<ViewportPoint>(from));
        } else if constexpr (std::is_same_v<To, GlobalPoint>) {
            return pointCast<GlobalPoint>(cast<ViewportPoint>(from));
        } else if constexpr (std::is_same_v<To, Scintilla::Position>) {
            return positionFrom(from);
        } else if constexpr (std::is_same_v<From, Scintilla::Position>) {
            return positionCast<To>(from);
        } else {
            return cast<To>(cast<Scintilla::Position>(from));
        }
    }

    explicit ScintillaWidget(QWidget *parent = nullptr);

    ~ScintillaWidget() override = default;

    [[nodiscard]] ScintillaCodePage::Value codePageGet() const;

    [[nodiscard]] ScintillaEolMode::Value eolModeGet() const;

    void eolModeSet(ScintillaEolMode::Value eolMode) const;

    [[nodiscard]] bool eolViewGet() const;

    void eolViewSet(bool status) const;

    [[nodiscard]] bool modifyGet() const;

    void readonlySet(bool status) const;

    [[nodiscard]] bool readonlyGet() const;

    void savepointSet() const;

    [[nodiscard]] static int colorGet(const QString &color);

    [[nodiscard]] static int colorGet(const QString &color, int alpha);

    void annotationClear() const;

    void annotationSet(int line, const QString &annotation) const;

    void eolAnnotationClear() const;

    void eolAnnotationSet(int line, const QString &annotation) const;

    void focusSet(bool status) const;

    void foldLevelSet(int line, int level) const;

    void foldContractTop() const;

    void foldContractRecursively() const;

    void foldExpandRecursively() const;

    [[nodiscard]] int heightGet() const;

    [[nodiscard]] Utf16IndexRange wordIndexGet(Scintilla::Position position = -1, bool onlyWordCharacters = true) const;

    void indexSet(int line, int character) const;

    void indicatorDefine(ScintillaIndicator::Value type, const QVariantHash &session) const;

    void indicatorFill(ScintillaIndicator::Value type, int startLine, int startCharacter, int endLine, int endCharacter, int time = -1) const;

    void indicatorClear(ScintillaIndicator::Value type, int startLine = -1, int startCharacter = -1, int endLine = -1, int endCharacter = -1) const;

    [[nodiscard]] int indicatorGet(Scintilla::Position position) const;

    [[nodiscard]] Scintilla::Position lengthGet() const;

    void lexerSet(const char *name) const;

    [[nodiscard]] int lineCountGet() const;

    void lineDuplicate() const;

    [[nodiscard]] int lineGet(Scintilla::Position position) const;

    [[nodiscard]] QString linesGet(int startLine, int lineCount) const;

    void linesSet(const QStringList &texts, const QList<int> &startLines, const QList<int> &lineCounts) const;

    void marginDefine(int type, const QVariantHash &session) const;

    void marginTextSet(int line, const QString &text) const;

    [[nodiscard]] int marginCountGet() const;

    [[nodiscard]] int marginWidthGet(int col = -1) const;

    void markerDefine(ScintillaMarker::Value type, const QVariantHash &session) const;

    void markerAdd(ScintillaMarker::Value type, int line, int time = -1) const;

    void markerDelete(ScintillaMarker::Value type, int line = -1) const;

    [[nodiscard]] int markerGet(int line) const;

    [[nodiscard]] bool atLineEnd() const;

    [[nodiscard]] Scintilla::Position positionGet(int line = -1, int character = -1) const;

    [[nodiscard]] Scintilla::Position closePositionGet(const QPoint &point) const;

    void positionSet(Scintilla::Position position) const;

    void searchFlagsSet(bool matchCase, bool wholeWord, bool wordStart, bool regExp) const;

    [[nodiscard]] Scintilla::Position targetGetStart() const;

    void targetSetStart(Scintilla::Position position) const;

    [[nodiscard]] Scintilla::Position targetGetEnd() const;

    void targetSetEnd(Scintilla::Position position) const;

    void targetSetWhole() const;

    [[nodiscard]] Scintilla::Position targetSearch(const QString &text) const;

    [[nodiscard]] QHash<QString, int> selectionGet() const;

    void selectionSet(int startLine, int startCharacter, int endLine, int endCharacter) const;

    void styleDefine(int type, const QVariantHash &session) const;

    [[nodiscard]] int styleGet(Scintilla::Position position) const;

    void styleSet(int type, int startLine = -1, int startCharacter = -1, int length = -1) const;

    void textAppend(const QString &text) const;

    void textClear() const;

    [[nodiscard]] QString textGet(int startLine = -1, int startCharacter = -1, int endLine = -1, int endCharacter = -1) const;

    [[nodiscard]] QString textGetSelected() const;

    void textSet(const QString &text, int startLine = -1, int startCharacter = -1, int endLine = -1, int endCharacter = -1) const;

    void textSetSelected(const QString &text) const;

    [[nodiscard]] bool copiable() const;

    [[nodiscard]] bool pastable() const;

    [[nodiscard]] bool undoable() const;

    [[nodiscard]] bool redoable() const;

    void undoBegin() const;

    void undoEnd() const;

private:
    template<typename T>
    static constexpr bool coordinateType = std::is_same_v<T, Scintilla::Position>
                                           || std::is_same_v<T, Utf8Index>
                                           || std::is_same_v<T, Utf16Index>
                                           || std::is_same_v<T, Utf32Index>
                                           || std::is_same_v<T, ViewportPoint>
                                           || std::is_same_v<T, GlobalPoint>;

    template<typename T>
    static constexpr bool pointType = std::is_same_v<T, ViewportPoint>
                                      || std::is_same_v<T, GlobalPoint>;

    template<typename To, typename From>
    [[nodiscard]] To pointCast(const From &from) const {
        if constexpr (std::is_same_v<To, GlobalPoint>) return GlobalPoint{viewport()->mapToGlobal(from.value)};
        else return ViewportPoint{viewport()->mapFromGlobal(from.value)};
    }

    template<typename To>
    [[nodiscard]] To positionCast(const Scintilla::Position position) const {
        if constexpr (std::is_same_v<To, Utf8Index>) return utf8IndexFrom(position);
        else if constexpr (std::is_same_v<To, Utf16Index>) return utf16IndexFrom(position);
        else if constexpr (std::is_same_v<To, Utf32Index>) return utf32IndexFrom(position);
        else return viewportPointFrom(position);
    }

    [[nodiscard]] Scintilla::Position positionFrom(const Utf8Index &index) const;

    [[nodiscard]] Scintilla::Position positionFrom(const Utf16Index &index) const;

    [[nodiscard]] Scintilla::Position positionFrom(const Utf32Index &index) const;

    [[nodiscard]] Scintilla::Position positionFrom(const ViewportPoint &point) const;

    [[nodiscard]] Scintilla::Position positionResolve(int line, int character) const;

    [[nodiscard]] Utf8Index utf8IndexFrom(Scintilla::Position position) const;

    [[nodiscard]] Utf16Index utf16IndexFrom(Scintilla::Position position) const;

    [[nodiscard]] Utf32Index utf32IndexFrom(Scintilla::Position position) const;

    [[nodiscard]] ViewportPoint viewportPointFrom(Scintilla::Position position) const;
};

#endif //UNICOMM_SCINTILLAWIDGET_H
