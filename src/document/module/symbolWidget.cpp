#include "document/module/symbolWidget.h"

#include <QJsonArray>

#include "globals.h"
#include "core/globalManager.h"
#include "core/themeManager.h"
#include "types/scintillaTypes.h"

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

void SymbolWidget::documentSymbolShow(const QJsonArray &result, const int line, const int character) {
    const auto symbolList = symbolParse(result, line, character);
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
    const auto *theme = g_globalManager->themeGet();
    const auto colors = theme->configGet()["lua"].toObject()["structure"].toObject();
    for (const auto &value: result) {
        const auto symbol = value.toObject();
        const auto kind = symbol["kind"].toInt();
        const auto detail = symbol["detail"].toString().trimmed();
        const auto name = symbol["name"].toString();
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
        QUrl source{};
        QString iconColor = theme->foreGet();
        switch (kind) {
            case LspSymbolKind::Package: {
                iconColor = colors["package"].toObject()["fore"].toString(theme->foreGet());
                source = "qrc:/icon/symbolPackage.svg";
            }
            break;
            case LspSymbolKind::Method:
            case LspSymbolKind::Function: {
                iconColor = colors[kind == LspSymbolKind::Method ? "method" : "function"].toObject()["fore"].toString(theme->foreGet());
                source = "qrc:/icon/symbolMethod.svg";
            }
            break;
            case LspSymbolKind::Variable: {
                iconColor = colors["variable"].toObject()["fore"].toString(theme->foreGet());
                source = "qrc:/icon/symbolVariable.svg";
            }
            break;
            case LspSymbolKind::Constant: {
                iconColor = colors["constant"].toObject()["fore"].toString(theme->foreGet());
                source = "qrc:/icon/symbolConstant.svg";
            }
            break;
            case LspSymbolKind::String: {
                iconColor = colors["string"].toObject()["fore"].toString(theme->foreGet());
                source = "qrc:/icon/symbolString.svg";
            }
            break;
            case LspSymbolKind::Number: {
                iconColor = colors["number"].toObject()["fore"].toString(theme->foreGet());
                source = "qrc:/icon/symbolNumeric.svg";
            }
            break;
            case LspSymbolKind::Boolean: {
                iconColor = colors["boolean"].toObject()["fore"].toString(theme->foreGet());
                source = "qrc:/icon/symbolBoolean.svg";
            }
            break;
            case LspSymbolKind::Array: {
                iconColor = colors["array"].toObject()["fore"].toString(theme->foreGet());
                source = "qrc:/icon/symbolArray.svg";
            }
            break;
            case LspSymbolKind::Object: {
                iconColor = colors["object"].toObject()["fore"].toString(theme->foreGet());
                source = "qrc:/icon/symbolMisc.svg";
            }
            break;
            default: {
                source = "qrc:/icon/symbolMisc.svg";
                emit appendLog(LogLevel::Warning, "contact author:", QString("unsupported symbol (kind:%1, name:%2, detail:%3)").arg(QString::number(kind), name, detail));
            }
            break;
        }
        symbolList.append(QVariantHash{
            {"text", name},
            {"source", source},
            {"iconColor", iconColor},
            {"detail", detail},
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
