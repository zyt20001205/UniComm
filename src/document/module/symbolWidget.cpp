#include "document/module/symbolWidget.h"

#include <QJsonObject>

#include "globals.h"
#include "core/globalManager.h"

// public
SymbolWidget::SymbolWidget(QWidget *parent)
    : QQuickWidget(g_globalManager->engineGet(), parent) {
    setFixedHeight(24);
}

void SymbolWidget::propertySet() {
    setInitialProperties({
        {"symbolWidget", QVariant::fromValue(this)}
    });

    setResizeMode(SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/document/module/symbolWidget.qml"));
}

void SymbolWidget::documentSymbolSet(const QJsonArray &result) {
    m_symbol = result;
}

void SymbolWidget::documentSymbolShow(const int line, const int character) {
    const auto symbolList = symbolParse(m_symbol, line, character);
    if (m_model == symbolList) return;
    m_model = symbolList;
    emit modelChanged();
}

void SymbolWidget::indicatorFill(const QVariantHash &position) {
    emit setFocus(true);
    emit setIndex(
        position["startLine"].toInt(),
        position["startCharacter"].toInt());
    emit fillIndicator(
        ScintillaIndicator::Current,
        position["startLine"].toInt(),
        position["startCharacter"].toInt(),
        position["endLine"].toInt(),
        position["endCharacter"].toInt(),
        1000);
}

QVariantList SymbolWidget::symbolParse(const QJsonArray &result, const int line, const int character) {
    QVariantList symbolList{};
    for (const auto &value: result) {
        const auto symbol = value.toObject();
        const auto range = symbol["range"].toObject();
        const auto start = range["start"].toObject();
        const auto end = range["end"].toObject();
        if (line < start["line"].toInt() || line > end["line"].toInt()) continue;
        if (line == start["line"].toInt() && character < start["character"].toInt()) continue;
        if (line == end["line"].toInt() && character > end["character"].toInt()) continue;
        const QVariantHash position = {
            {"startLine", start["line"].toInt()},
            {"startCharacter", start["character"].toInt()},
            {"endLine", end["line"].toInt()},
            {"endCharacter", end["character"].toInt()}
        };
        symbolList.append(QVariantHash{
            {"kind", symbol["kind"].toInt()},
            {"text", symbol["name"].toString()},
            {"detail", symbol["detail"].toString().trimmed()},
            {"position", position}
        });
        if (symbol.contains("children")) {
            const auto children = symbol["children"].toArray();
            symbolList.append(symbolParse(children, line, character));
        }
        return symbolList;
    }
    return {};
}
