#include "util/uniCast.h"

#include <QDir>
#include <QHash>
#include <QMovie>
#include <QTime>
#include <QUuid>
#include <sol/state_view.hpp>
#include <sol/table_core.hpp>
#include <sol/utility/is_integer.hpp>
#include <sol/variadic_args.hpp>
#include <sol/userdata.hpp>

#include "cmark-gfm.h"
#include "cmark-gfm-core-extensions.h"
#include "cmark-gfm-extension_api.h"
#include "globals.h"
#include "core/globalManager.h"

// lua -> qt
template<>
QUrl uni_cast<QUrl, LUrl>(const LUrl &s, const int depth) {
    Q_UNUSED(depth);
    auto uri = QUrl::fromPercentEncoding(s.value.toUtf8());
    if (uri.size() > 8) {
        const QChar drive = uri[8];
        if (drive.isLetter() && drive.isLower()) {
            uri[8] = drive.toUpper();
        }
    }
    return {uri};
}

template<>
QUrl uni_cast<QUrl, LPath>(const LPath &s, int depth) {
    Q_UNUSED(depth);
    const QDir workspaceDir(g_workspaceUrl.toLocalFile());
    const auto documentPath = workspaceDir.absoluteFilePath(s.value);
    const auto documentUrl = QUrl::fromLocalFile(documentPath);
    return documentUrl;
}

// sol -> qt
template<>
QString uni_cast<QString, sol::object>(const sol::object &s, const int depth) {
    Q_UNUSED(depth);
    switch (s.get_type()) {
        case sol::type::nil:
            return "nil";
        case sol::type::boolean:
            return s.as<bool>() ? "true" : "false";
        case sol::type::lightuserdata:
            return "lightuserdata";
        case sol::type::number:
            if (s.is<int>()) {
                return QString::number(s.as<int>());
            }
            return QString::number(s.as<double>());
        case sol::type::string:
            return QString::fromStdString(s.as<std::string>());
        case sol::type::table:
            return "{...}";
        case sol::type::function:
            return "function";
        case sol::type::userdata:
            return "userdata";
        case sol::type::thread:
            return "thread";
        default:
            return "?";
    }
}

template<>
QVariant uni_cast<QVariant, sol::object>(const sol::object &s, const int depth) {
    constexpr int MAX_DEPTH = 100;
    if (depth > MAX_DEPTH) {
        throw sol::error("Maximum recursion depth exceeded");
    }
    switch (s.get_type()) {
        case sol::type::nil: {
            return "nil";
        }
        case sol::type::boolean: {
            return s.as<bool>();
        }
        case sol::type::number: {
            if (sol::utility::is_integer(s)) return s.as<lua_Integer>();
            return s.as<double>();
        }
        case sol::type::string: {
            const std::string str = s.as<std::string>();
            QString string{};
            // try utf-8
            string = QString::fromUtf8(str.data(), static_cast<qsizetype>(str.size()));
            if (!string.contains(QChar::ReplacementCharacter)) {
                return string;
            }
            // try ascii
            string = QString::fromLatin1(str.data(), static_cast<qsizetype>(str.size()));
            return string;
        }
        case sol::type::table: {
            const auto table = s.as<sol::table>();
            QVariantMap map{};
            sol::state_view lua(table.lua_state());
            const auto pairs = lua["pairs"](table);
            const auto next = pairs.get<sol::function>(0);
            const auto data = pairs.get<sol::object>(1);
            auto currentKey = pairs.get<sol::object>(2);
            while (true) {
                auto pair = next(data, currentKey);
                auto key = pair.get<sol::object>(0);
                if (key.get_type() == sol::type::nil) break;
                auto value = pair.get<sol::object>(1);
                currentKey = key;
                QString key_str{};
                if (key.is<std::string>()) key_str = QString::fromStdString(key.as<std::string>());
                else if (key.is<int>()) key_str = QString::number(key.as<int>());
                else if (key.is<double>()) key_str = QString::number(key.as<double>());
                else continue;
                map[key_str] = uni_cast<QVariant>(value, depth + 1);
            }
            return QVariant::fromValue(map);
        }
        case sol::type::userdata: {
            const auto mapPtr = s.as<sol::userdata>().as<QVariantMap *>();
            return QVariant::fromValue(*mapPtr);
        }
        default: {
            qDebug() << "Unsupported Lua Type" << static_cast<int>(s.get_type());;
            return "?";
        }
    }
}

template<>
QJsonObject uni_cast<QJsonObject, sol::table>(const sol::table &s, const int depth) {
    QJsonObject d{};
    for (const auto &[key, value]: s) {
        if (!key.is<std::string>()) continue;
        d[QString::fromStdString(key.as<std::string>())] = QJsonValue::fromVariant(uni_cast<QVariant>(value, depth + 1));
    }
    return d;
}

template<>
QVariantList uni_cast<QVariantList, sol::variadic_args>(const sol::variadic_args &s, const int depth) {
    Q_UNUSED(depth);
    QVariantList d{};
    for (const sol::object &arg: s) {
        d.append(uni_cast<QVariant>(arg));
    }
    return d;
}

// qt -> sol
template<>
sol::object uni_cast<sol::object, QVariant>(const sol::this_state ts, const QVariant &s, const int depth) {
    sol::state_view lua(ts);
    constexpr int MAX_DEPTH = 100;
    if (depth > MAX_DEPTH) {
        throw sol::error("Maximum recursion depth exceeded");
    }
    if (!s.isValid()) return sol::nil;
    switch (s.typeId()) {
        case QMetaType::Bool:
            return sol::make_object(lua, s.toBool());
        case QMetaType::Int:
            return sol::make_object(lua, s.toInt());
        case QMetaType::UInt:
            return sol::make_object(lua, s.toUInt());
        case QMetaType::LongLong:
            return sol::make_object(lua, s.toLongLong());
        case QMetaType::Double:
            return sol::make_object(lua, s.toDouble());
        case QMetaType::QVariantMap: {
            const auto _s = s.toMap();
            sol::table _d = lua.create_table();
            for (auto it = _s.constBegin(); it != _s.constEnd(); ++it) {
                const QString &key = it.key();
                const QVariant &value = it.value();
                _d[key.toStdString()] = uni_cast<sol::object>(ts, value, depth + 1);
            }
            return sol::make_object(lua, _d);
        }
        case QMetaType::QVariantList: {
            const auto _s = s.toList();
            sol::table _d = lua.create_table();
            for (int i = 0; i < _s.size(); ++i) {
                _d[i + 1] = uni_cast<sol::object>(ts, _s[i], depth + 1);
            }
            return sol::make_object(lua, _d);
        }
        case QMetaType::QString:
            return sol::make_object(lua, s.toString().toStdString());
        case QMetaType::QByteArray: {
            const auto ba = s.toByteArray();
            return sol::make_object(lua, std::string(ba.constData(), ba.size()));
        }
        case QMetaType::QVariantHash: {
            const auto _s = s.toHash();
            sol::table _d = lua.create_table();
            for (auto it = _s.constBegin(); it != _s.constEnd(); ++it) {
                const QString &key = it.key();
                const QVariant &value = it.value();
                _d[key.toStdString()] = uni_cast<sol::object>(ts, value, depth + 1);
            }
            return sol::make_object(lua, _d);
        }
        case QMetaType::Float:
            return sol::make_object(lua, s.toFloat());
        default:
            return sol::nil;
    }
    // Bool = 1, Int = 2, UInt = 3, LongLong = 4, ULongLong = 5, Double = 6, Long = 32, Short = 33, Char = 34, Char16 = 56, Char32 = 57, ULong = 35, UShort = 36, UChar = 37,
    // Float = 38, SChar = 40, Nullptr = 51, QCborSimpleType = 52, Void = 43, VoidStar = 31, QChar = 7, QString = 10, QByteArray = 12, QBitArray = 13, QDate = 14, QTime = 15,
    // QDateTime = 16, QUrl = 17, QLocale = 18, QRect = 19, QRectF = 20, QSize = 21, QSizeF = 22, QLine = 23, QLineF = 24, QPoint = 25, QPointF = 26, QEasingCurve = 29, QUuid = 30,
    // QVariant = 41, QRegularExpression = 44, QJsonValue = 45, QJsonObject = 46, QJsonArray = 47, QJsonDocument = 48, QCborValue = 53, QCborArray = 54, QCborMap = 55, Float16 = 63,
    // QModelIndex = 42, QPersistentModelIndex = 50, QObjectStar = 39, QVariantMap = 8, QVariantList = 9, QVariantHash = 28, QVariantPair = 58, QByteArrayList = 49, QStringList = 11,
    // QFont = 0x1000, QPixmap = 0x1001, QBrush = 0x1002, QColor = 0x1003, QPalette = 0x1004, QIcon = 0x1005, QImage = 0x1006, QPolygon = 0x1007, QRegion = 0x1008, QBitmap = 0x1009,
    // QCursor = 0x100a, QKeySequence = 0x100b, QPen = 0x100c, QTextLength = 0x100d, QTextFormat = 0x100e, QTransform = 0x1010, QMatrix4x4 = 0x1011, QVector2D = 0x1012,
    // QVector3D = 0x1013, QVector4D = 0x1014, QQuaternion = 0x1015, QPolygonF = 0x1016, QColorSpace = 0x1017, QSizePolicy = 0x2000,
}

template<>
sol::object uni_cast<sol::object, QVariantMap>(const sol::this_state ts, const QVariantMap &s, const int depth) {
    return uni_cast<sol::object, QVariant>(ts, QVariant::fromValue(s), depth);
}

template<>
sol::object uni_cast<sol::object, QVariantList>(const sol::this_state ts, const QVariantList &s, const int depth) {
    return uni_cast<sol::object, QVariant>(ts, QVariant::fromValue(s), depth);
}

template<>
sol::object uni_cast<sol::object, QVariantHash>(const sol::this_state ts, const QVariantHash &s, const int depth) {
    return uni_cast<sol::object, QVariant>(ts, QVariant::fromValue(s), depth);
}

template<>
sol::table uni_cast<sol::table, QSet<QString> >(const sol::this_state ts, const QSet<QString> &s, const int depth) {
    Q_UNUSED(depth);
    sol::state_view lua(ts);
    int index = 1;
    sol::table d = lua.create_table();
    for (const auto &value: s) {
        d[index++] = sol::make_object(lua, value.toStdString());
    }
    return d;
}

template<>
sol::table uni_cast<sol::table, QList<QVariant> >(const sol::this_state ts, const QList<QVariant> &s, const int depth) {
    sol::state_view lua(ts);
    sol::table d = lua.create_table();
    for (int i = 0; i < s.size(); ++i) {
        d[i + 1] = uni_cast<sol::object>(ts, s[i], depth + 1);
    }
    return d;
}

// qt -> qt
template<>
QFileIcon uni_cast<QFileIcon, QUrl>(const QUrl &s, const int depth) {
    Q_UNUSED(depth);
    const auto fileInfo = QFileInfo(s.toLocalFile());
    const auto suffix = fileInfo.suffix().toLower();
    const QStringList libType = {"dll", "so"};
    if (QImageReader::supportedImageFormats().contains(suffix)) return QUrl("qrc:/icon/fileTypeImage.svg");
    if (QMovie::supportedFormats().contains(suffix)) return QUrl("qrc:/icon/fileTypeVideo.svg");
    if (suffix == "bat") return QUrl("qrc:/icon/fileTypeBatch.svg");
    if (suffix == "csv") return QUrl("qrc:/icon/fileTypeCsv.svg");
    if (suffix == "exe") return QUrl("qrc:/icon/fileTypeExe.svg");
    if (suffix == "gitignore") return QUrl("qrc:/icon/fileTypeGit.svg");
    if (suffix == "json") return QUrl("qrc:/icon/fileTypeJson.svg");
    if (libType.contains(suffix)) return QUrl("qrc:/icon/fileTypeLib.svg");
    if (suffix == "lua") return QUrl("qrc:/icon/fileTypeLua.svg");
    if (suffix == "md") return QUrl("qrc:/icon/fileTypeMarkdown.svg");
    if (suffix == "pdf") return QUrl("qrc:/icon/fileTypePdf.svg");
    if (suffix == "ps1") return QUrl("qrc:/icon/fileTypePowershell.svg");
    if (suffix == "toml") return QUrl("qrc:/icon/fileTypeToml.svg");
    if (suffix == "txt") return QUrl("qrc:/icon/fileTypeTxt.svg");
    if (fileInfo.isDir() && fileInfo.fileName() == ".git") return QUrl("qrc:/icon/fileTypeFolderGit.svg");
    if (fileInfo.isDir() && fileInfo.fileName() == ".idea") return QUrl("qrc:/icon/fileTypeFolderIntellij.svg");
    if (fileInfo.isDir()) return QUrl("qrc:/icon/fileTypeFolder.svg");
    return QUrl("qrc:/icon/fileTypeDefault.svg");
}

template<>
QIcon uni_cast<QIcon, QUrl>(const QUrl &s, const int depth) {
    Q_UNUSED(depth);
    const auto &source = ':' + uni_cast<QFileIcon>(s).value.path();
    return QIcon(source);
}

template<>
QList<MarkdownBlock> uni_cast<QList<MarkdownBlock>, QString>(const QString &s, const int depth) {
    Q_UNUSED(depth);
    const auto markdown = s.toUtf8();
    cmark_gfm_core_extensions_ensure_registered();
    auto *parser = cmark_parser_new(CMARK_OPT_FOOTNOTES);
    cmark_parser_attach_syntax_extension(parser, cmark_find_syntax_extension("table"));
    cmark_parser_feed(parser, markdown.constData(), markdown.size());
    auto *document = cmark_parser_finish(parser);

    struct ParsedBlock {
        int startLine{};
        int endLine{};
        MarkdownBlock block{};
    };

    QList<ParsedBlock> parsedBlocks{};
    auto *iterator = cmark_iter_new(document);
    while (cmark_iter_next(iterator) != CMARK_EVENT_DONE) {
        auto *node = cmark_iter_get_node(iterator);
        if (cmark_iter_get_event_type(iterator) != CMARK_EVENT_ENTER) continue;
        if (cmark_node_get_type(node) == CMARK_NODE_CODE_BLOCK) {
            auto content = QString::fromUtf8(cmark_node_get_literal(node));
            if (content.endsWith('\n')) content.chop(1);
            parsedBlocks.append({
                .startLine = cmark_node_get_start_line(node),
                .endLine = cmark_node_get_end_line(node),
                .block = {
                    .type = MarkdownBlock::Type::Code,
                    .content = content,
                    .language = QString::fromUtf8(cmark_node_get_fence_info(node)).section(' ', 0, 0)
                }
            });
            continue;
        }
        if (QString::fromUtf8(cmark_node_get_type_string(node)) != "table") continue;

        MarkdownTable table{};
        for (auto *row = cmark_node_first_child(node); row; row = cmark_node_next(row)) {
            QStringList cells{};
            for (auto *cell = cmark_node_first_child(row); cell; cell = cmark_node_next(cell)) {
                QString markdown{};
                for (auto *child = cmark_node_first_child(cell); child; child = cmark_node_next(child)) {
                    auto *content = cmark_render_commonmark(child, CMARK_OPT_NOBREAKS, 0);
                    auto text = QString::fromUtf8(content);
                    free(content);
                    text.chop(1);
                    markdown += text;
                }
                cells.append(markdown);
            }
            const auto type = QString::fromUtf8(cmark_node_get_type_string(row));
            if (type == "table_header" || cmark_gfm_extensions_get_table_row_is_header(row)) table.header = cells;
            else table.rows.append(cells);
        }
        const auto columns = cmark_gfm_extensions_get_table_columns(node);
        const auto *alignments = cmark_gfm_extensions_get_table_alignments(node);
        for (auto column = 0; column < columns; ++column) {
            table.alignments.append(
                alignments[column] == 'c' ? MarkdownTable::Alignment::Center : alignments[column] == 'r' ? MarkdownTable::Alignment::Right : MarkdownTable::Alignment::Left
            );
        }
        parsedBlocks.append({
            .startLine = cmark_node_get_start_line(node),
            .endLine = cmark_node_get_end_line(node),
            .block = {
                .type = MarkdownBlock::Type::Table,
                .table = std::move(table)
            }
        });
    }
    cmark_iter_free(iterator);
    cmark_node_free(document);
    cmark_parser_free(parser);

    const auto lines = s.split('\n', Qt::KeepEmptyParts);
    QList<MarkdownBlock> blocks{};
    qsizetype currentLine{};
    for (auto &parsedBlock: parsedBlocks) {
        const qsizetype startLine = parsedBlock.startLine - 1;
        const qsizetype endLine = parsedBlock.endLine;
        const auto content = lines.sliced(currentLine, startLine - currentLine).join('\n');
        if (!content.trimmed().isEmpty()) blocks.append({MarkdownBlock::Type::Markdown, content, {}});
        if (parsedBlock.block.type == MarkdownBlock::Type::Table) parsedBlock.block.content = lines.sliced(startLine, endLine - startLine).join('\n');
        blocks.append(std::move(parsedBlock.block));
        currentLine = endLine;
    }
    const auto content = lines.sliced(currentLine).join('\n');
    if (!content.trimmed().isEmpty()) blocks.append({MarkdownBlock::Type::Markdown, content, {}});
    return blocks;
}

template<>
QHtmlString uni_cast<QHtmlString, QString>(const QString &s, const int depth) {
    Q_UNUSED(depth);
    const auto md = s.toUtf8();

    cmark_gfm_core_extensions_ensure_registered();
    constexpr int options = CMARK_OPT_UNSAFE | CMARK_OPT_FOOTNOTES;
    cmark_parser *parser = cmark_parser_new(options);
    const char *extensions[] = {
        "footnotes",
        "table",
        "strikethrough",
        "autolink",
        "tasklist",
    };
    for (const auto *name: extensions) {
        auto *extension = cmark_find_syntax_extension(name);
        if (extension) cmark_parser_attach_syntax_extension(parser, extension);
    }
    cmark_parser_feed(parser, md.constData(), md.size());
    cmark_node *document = cmark_parser_finish(parser);
    char *htmlChar = cmark_render_html(document, options, cmark_parser_get_syntax_extensions(parser));

    QString d = htmlChar ? QString::fromUtf8(htmlChar) : QString();
    if (htmlChar) free(htmlChar);
    cmark_node_free(document);
    cmark_parser_free(parser);
    if (!g_globalManager) return d;

    d.replace("<pre>", R"(<pre style='font-family: Consolas, monospace; background-color: @hover; border: 1px solid @stroke; padding: 8px; white-space: pre-wrap;'>)");
    d.replace("<pre ", R"(<pre style='font-family: Consolas, monospace; background-color: @hover; border: 1px solid @stroke; padding: 8px; white-space: pre-wrap;' )");
    d.replace("<code>", R"(<code style='font-family: Consolas, monospace; background-color: @hover; border: 1px solid @stroke; padding: 2px 4px; white-space: pre-wrap;'>)");
    d.replace("<code ", R"(<code style='font-family: Consolas, monospace; background-color: @hover; border: 1px solid @stroke; padding: 2px 4px; white-space: pre-wrap;' )");
    d.replace("<table>", R"(<table style='border-collapse: collapse; margin: 8px 0;'>)");
    d.replace("<table ", R"(<table style='border-collapse: collapse; margin: 8px 0;' )");
    d.replace("<td>", R"(<td style='border: 1px solid @stroke; padding: 4px 8px;'>)");
    d.replace("<td ", R"(<td style='border: 1px solid @stroke; padding: 4px 8px;' )");
    d.replace("<th>", R"(<th style='border: 1px solid @stroke; padding: 4px 8px; background-color: @hover; font-weight: 600;'>)");
    d.replace("<th ", R"(<th style='border: 1px solid @stroke; padding: 4px 8px; background-color: @hover; font-weight: 600;' )");
    d.replace("@hover", g_globalManager->backHoverGet());
    d.replace("@stroke", g_globalManager->strokeGet());
    return d;
}

template<>
QFullHtmlString uni_cast<QFullHtmlString, QString>(const QString &s, const int depth) {
    Q_UNUSED(depth);
    const auto md = s.toUtf8();

    cmark_gfm_core_extensions_ensure_registered();
    constexpr int options = CMARK_OPT_UNSAFE | CMARK_OPT_FOOTNOTES;
    cmark_parser *parser = cmark_parser_new(options);
    const char *extensions[] = {
        "footnotes",
        "table",
        "strikethrough",
        "autolink",
        "tasklist",
    };
    for (const auto *name: extensions) {
        auto *extension = cmark_find_syntax_extension(name);
        if (extension) cmark_parser_attach_syntax_extension(parser, extension);
    }
    cmark_parser_feed(parser, md.constData(), md.size());
    cmark_node *document = cmark_parser_finish(parser);
    char *htmlChar = cmark_render_html(document, options, cmark_parser_get_syntax_extensions(parser));

    const QString body = htmlChar ? QString::fromUtf8(htmlChar) : QString();
    if (htmlChar) free(htmlChar);
    cmark_node_free(document);
    cmark_parser_free(parser);

    auto d = QString(R"(<!doctype html>
<html>
<head>
<meta charset="utf-8">
<style>
html, body {
    margin: 0;
    min-height: 100%;
    color: @fore;
    background: @back;
    font-family: "Segoe UI", sans-serif;
    font-size: 14px;
}
body {
    padding: 12px;
}
a {
    color: @link;
}
pre, code {
    font-family: Consolas, monospace;
}
code {
    padding: 2px 4px;
    border: 1px solid @stroke;
    border-radius: 4px;
    background: @hover;
}
pre {
    overflow: auto;
    padding: 8px;
    background: @hover;
}
pre code {
    padding: 0;
    border: 0;
    background: transparent;
}
table {
    border-collapse: collapse;
    margin: 8px 0;
}
th, td {
    border: 1px solid @stroke;
    padding: 4px 8px;
}
th {
    background: @hover;
    font-weight: 600;
}
tr:nth-child(even) td {
    background: @selected;
}
img {
    max-width: 100%;
}
.mermaid {
    overflow: auto;
}
.mermaid svg {
    max-width: 100%;
}
.hljs {
    background: transparent;
}
</style>
<link rel="stylesheet" href="http://unicomm/@highlightTheme">
<script src="http://unicomm/mermaid.min.js"></script>
<script src="http://unicomm/highlight.min.js"></script>
<script>
document.addEventListener("DOMContentLoaded", async () => {
    const blocks = document.querySelectorAll("pre > code.language-mermaid, pre > code.mermaid");

    if (window.hljs) {
        document.querySelectorAll("pre > code").forEach((code) => {
            if (code.classList.contains("language-mermaid") || code.classList.contains("mermaid")) return;
            hljs.highlightElement(code);
        });
    }

    if (window.mermaid && blocks.length > 0) {
        blocks.forEach((code) => {
            const diagram = document.createElement("div");
            diagram.className = "mermaid";
            diagram.textContent = code.textContent;
            code.parentElement.replaceWith(diagram);
        });

        mermaid.initialize({
            startOnLoad: false,
            securityLevel: "loose",
            theme: "@mermaidTheme"
        });
        await mermaid.run({ querySelector: ".mermaid" });
    }
});
</script>
</head>
<body>@body</body>
</html>)");
    d.replace("@fore", g_globalManager->foreGet());
    d.replace("@back", g_globalManager->backGet());
    d.replace("@link", g_globalManager->brandLinkGet());
    d.replace("@hover", g_globalManager->backHoverGet());
    d.replace("@stroke", g_globalManager->strokeGet());
    d.replace("@selected", g_globalManager->backSelectedGet());
    d.replace("@mermaidTheme", g_globalManager->themeGet() == Theme::Light ? "default" : "dark");
    d.replace("@highlightTheme", g_globalManager->themeGet() == Theme::Light ? "highlight-light.min.css" : "highlight-dark.min.css");
    d.replace("@body", body);
    return d;
}

template<>
QDuration uni_cast<QDuration, qint64>(const qint64 &s, const int depth) {
    Q_UNUSED(depth);
    if (s >= 60000) return QStringLiteral("%1m %2s").arg(s / 60000).arg(s % 60000 / 1000);
    if (s >= 1000) return QStringLiteral("%1s").arg(static_cast<double>(s) / 1000, 0, 'f', 1);
    return QStringLiteral("%1ms").arg(s);
}

template<>
QCompactNumber uni_cast<QCompactNumber, qint64>(const qint64 &s, const int depth) {
    Q_UNUSED(depth);
    if (s >= 1000000) return QStringLiteral("%1m").arg(static_cast<double>(s) / 1000000, 0, 'f', 1);
    if (s >= 1000) return QStringLiteral("%1k").arg(static_cast<double>(s) / 1000, 0, 'f', 1);
    return QString::number(s);
}

template<>
QBluetoothUuidNormalized uni_cast<QBluetoothUuidNormalized, QString>(const QString &s, const int depth) {
    Q_UNUSED(depth);
    if (s.size() == 4 || s.size() == 8) {
        bool valid{};
        const auto value = s.toUInt(&valid, 16);
        if (!valid || s.compare(QString::number(value, 16).rightJustified(s.size(), QChar('0')), Qt::CaseInsensitive) != 0) return QStringLiteral("");
        return QString("%1-0000-1000-8000-00805f9b34fb").arg(value, 8, 16, QChar('0'));
    }
    const auto uuid = QUuid::fromString(s);
    if (uuid.isNull()) return QStringLiteral("");
    return uuid.toString(QUuid::WithoutBraces);
}

template<>
QBluetoothUuidShort uni_cast<QBluetoothUuidShort, QString>(const QString &s, const int depth) {
    Q_UNUSED(depth);
    const auto uuid = uni_cast<QBluetoothUuidNormalized>(s).value;
    if (!uuid.endsWith("-0000-1000-8000-00805f9b34fb", Qt::CaseInsensitive)) return uuid;
    const auto prefix = uuid.first(8).toUpper();
    return QStringLiteral("0x") + (prefix.startsWith("0000") ? prefix.sliced(4) : prefix);
}

template<>
QBluetoothUuidName uni_cast<QBluetoothUuidName, QString>(const QString &s, const int depth) {
    Q_UNUSED(depth);
    const auto uuid = QUuid::fromString(uni_cast<QBluetoothUuidNormalized>(s).value);
    const QUuid bluetoothUuid{uuid.data1, 0x0000, 0x1000, 0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb};
    if (uuid.data1 > 0xffff || uuid != bluetoothUuid) return QStringLiteral("");

    // Bluetooth SIG Assigned Numbers snapshot: 2026-11-13.
    // https://bitbucket.org/bluetooth-SIG/public/src/main/assigned_numbers/uuids/
    static const QHash<quint16, QString> names = {
        {0x1800, QStringLiteral("GAP")},
        {0x1801, QStringLiteral("GATT")},
        {0x1802, QStringLiteral("Immediate Alert")},
        {0x1803, QStringLiteral("Link Loss")},
        {0x1804, QStringLiteral("Tx Power")},
        {0x1805, QStringLiteral("Current Time")},
        {0x1806, QStringLiteral("Reference Time Update")},
        {0x1807, QStringLiteral("Next DST Change")},
        {0x1808, QStringLiteral("Glucose")},
        {0x1809, QStringLiteral("Health Thermometer")},
        {0x180A, QStringLiteral("Device Information")},
        {0x180D, QStringLiteral("Heart Rate")},
        {0x180E, QStringLiteral("Phone Alert Status")},
        {0x180F, QStringLiteral("Battery")},
        {0x1810, QStringLiteral("Blood Pressure")},
        {0x1811, QStringLiteral("Alert Notification")},
        {0x1812, QStringLiteral("Human Interface Device")},
        {0x1813, QStringLiteral("Scan Parameters")},
        {0x1814, QStringLiteral("Running Speed and Cadence")},
        {0x1815, QStringLiteral("Automation IO")},
        {0x1816, QStringLiteral("Cycling Speed and Cadence")},
        {0x1818, QStringLiteral("Cycling Power")},
        {0x1819, QStringLiteral("Location and Navigation")},
        {0x181A, QStringLiteral("Environmental Sensing")},
        {0x181B, QStringLiteral("Body Composition")},
        {0x181C, QStringLiteral("User Data")},
        {0x181D, QStringLiteral("Weight Scale")},
        {0x181E, QStringLiteral("Bond Management")},
        {0x181F, QStringLiteral("Continuous Glucose Monitoring")},
        {0x1820, QStringLiteral("Internet Protocol Support")},
        {0x1821, QStringLiteral("Indoor Positioning")},
        {0x1822, QStringLiteral("Pulse Oximeter")},
        {0x1823, QStringLiteral("HTTP Proxy")},
        {0x1824, QStringLiteral("Transport Discovery")},
        {0x1825, QStringLiteral("Object Transfer")},
        {0x1826, QStringLiteral("Fitness Machine")},
        {0x1827, QStringLiteral("Mesh Provisioning")},
        {0x1828, QStringLiteral("Mesh Proxy")},
        {0x1829, QStringLiteral("Reconnection Configuration")},
        {0x183A, QStringLiteral("Insulin Delivery")},
        {0x183B, QStringLiteral("Binary Sensor")},
        {0x183C, QStringLiteral("Emergency Configuration")},
        {0x183D, QStringLiteral("Authorization Control")},
        {0x183E, QStringLiteral("Physical Activity Monitor")},
        {0x183F, QStringLiteral("Elapsed Time")},
        {0x1840, QStringLiteral("Generic Health Sensor")},
        {0x1843, QStringLiteral("Audio Input Control")},
        {0x1844, QStringLiteral("Volume Control")},
        {0x1845, QStringLiteral("Volume Offset Control")},
        {0x1846, QStringLiteral("Coordinated Set Identification")},
        {0x1847, QStringLiteral("Device Time")},
        {0x1848, QStringLiteral("Media Control")},
        {0x1849, QStringLiteral("Generic Media Control")},
        {0x184A, QStringLiteral("Constant Tone Extension")},
        {0x184B, QStringLiteral("Telephone Bearer")},
        {0x184C, QStringLiteral("Generic Telephone Bearer")},
        {0x184D, QStringLiteral("Microphone Control")},
        {0x184E, QStringLiteral("Audio Stream Control")},
        {0x184F, QStringLiteral("Broadcast Audio Scan")},
        {0x1850, QStringLiteral("Published Audio Capabilities")},
        {0x1851, QStringLiteral("Basic Audio Announcement")},
        {0x1852, QStringLiteral("Broadcast Audio Announcement")},
        {0x1853, QStringLiteral("Common Audio")},
        {0x1854, QStringLiteral("Hearing Access")},
        {0x1855, QStringLiteral("Telephony and Media Audio")},
        {0x1856, QStringLiteral("Public Broadcast Announcement")},
        {0x1857, QStringLiteral("Electronic Shelf Label")},
        {0x1858, QStringLiteral("Gaming Audio")},
        {0x1859, QStringLiteral("Mesh Proxy Solicitation")},
        {0x185A, QStringLiteral("Industrial Measurement Device")},
        {0x185B, QStringLiteral("Ranging")},
        {0x185C, QStringLiteral("HID ISO")},
        {0x185D, QStringLiteral("Cookware")},
        {0x185E, QStringLiteral("Voice Assistant")},
        {0x185F, QStringLiteral("Generic Voice Assistant")},
        {0x1860, QStringLiteral("Tire Pressure Monitoring System")},
        {0x2800, QStringLiteral("Primary Service")},
        {0x2801, QStringLiteral("Secondary Service")},
        {0x2802, QStringLiteral("Include")},
        {0x2803, QStringLiteral("Characteristic")},
        {0x2900, QStringLiteral("Characteristic Extended Properties")},
        {0x2901, QStringLiteral("Characteristic User Description")},
        {0x2902, QStringLiteral("Client Characteristic Configuration")},
        {0x2903, QStringLiteral("Server Characteristic Configuration")},
        {0x2904, QStringLiteral("Characteristic Presentation Format")},
        {0x2905, QStringLiteral("Characteristic Aggregate Format")},
        {0x2906, QStringLiteral("Valid Range")},
        {0x2907, QStringLiteral("External Report Reference")},
        {0x2908, QStringLiteral("Report Reference")},
        {0x2909, QStringLiteral("Number of Digitals")},
        {0x290A, QStringLiteral("Value Trigger Setting")},
        {0x290B, QStringLiteral("Environmental Sensing Configuration")},
        {0x290C, QStringLiteral("Environmental Sensing Measurement")},
        {0x290D, QStringLiteral("Environmental Sensing Trigger Setting")},
        {0x290E, QStringLiteral("Time Trigger Setting")},
        {0x290F, QStringLiteral("Complete BR-EDR Transport Block Data")},
        {0x2910, QStringLiteral("Observation Schedule")},
        {0x2911, QStringLiteral("Valid Range and Accuracy")},
        {0x2912, QStringLiteral("Measurement Description")},
        {0x2913, QStringLiteral("Manufacturer Limits")},
        {0x2914, QStringLiteral("Process Tolerances")},
        {0x2915, QStringLiteral("IMD Trigger Setting")},
        {0x2916, QStringLiteral("Cooking Sensor Info")},
        {0x2917, QStringLiteral("Cooking Trigger Setting")},
        {0x2A00, QStringLiteral("Device Name")},
        {0x2A01, QStringLiteral("Appearance")},
        {0x2A02, QStringLiteral("Peripheral Privacy Flag")},
        {0x2A03, QStringLiteral("Reconnection Address")},
        {0x2A04, QStringLiteral("Peripheral Preferred Connection Parameters")},
        {0x2A05, QStringLiteral("Service Changed")},
        {0x2A06, QStringLiteral("Alert Level")},
        {0x2A07, QStringLiteral("Tx Power Level")},
        {0x2A08, QStringLiteral("Date Time")},
        {0x2A09, QStringLiteral("Day of Week")},
        {0x2A0A, QStringLiteral("Day Date Time")},
        {0x2A0C, QStringLiteral("Exact Time 256")},
        {0x2A0D, QStringLiteral("DST Offset")},
        {0x2A0E, QStringLiteral("Time Zone")},
        {0x2A0F, QStringLiteral("Local Time Information")},
        {0x2A11, QStringLiteral("Time with DST")},
        {0x2A12, QStringLiteral("Time Accuracy")},
        {0x2A13, QStringLiteral("Time Source")},
        {0x2A14, QStringLiteral("Reference Time Information")},
        {0x2A16, QStringLiteral("Time Update Control Point")},
        {0x2A17, QStringLiteral("Time Update State")},
        {0x2A18, QStringLiteral("Glucose Measurement")},
        {0x2A19, QStringLiteral("Battery Level")},
        {0x2A1C, QStringLiteral("Temperature Measurement")},
        {0x2A1D, QStringLiteral("Temperature Type")},
        {0x2A1E, QStringLiteral("Intermediate Temperature")},
        {0x2A21, QStringLiteral("Measurement Interval")},
        {0x2A22, QStringLiteral("Boot Keyboard Input Report")},
        {0x2A23, QStringLiteral("System ID")},
        {0x2A24, QStringLiteral("Model Number String")},
        {0x2A25, QStringLiteral("Serial Number String")},
        {0x2A26, QStringLiteral("Firmware Revision String")},
        {0x2A27, QStringLiteral("Hardware Revision String")},
        {0x2A28, QStringLiteral("Software Revision String")},
        {0x2A29, QStringLiteral("Manufacturer Name String")},
        {0x2A2A, QStringLiteral("IEEE 11073-20601 Regulatory Certification Data List")},
        {0x2A2B, QStringLiteral("Current Time")},
        {0x2A2C, QStringLiteral("Magnetic Declination")},
        {0x2A31, QStringLiteral("Scan Refresh")},
        {0x2A32, QStringLiteral("Boot Keyboard Output Report")},
        {0x2A33, QStringLiteral("Boot Mouse Input Report")},
        {0x2A34, QStringLiteral("Glucose Measurement Context")},
        {0x2A35, QStringLiteral("Blood Pressure Measurement")},
        {0x2A36, QStringLiteral("Intermediate Cuff Pressure")},
        {0x2A37, QStringLiteral("Heart Rate Measurement")},
        {0x2A38, QStringLiteral("Body Sensor Location")},
        {0x2A39, QStringLiteral("Heart Rate Control Point")},
        {0x2A3F, QStringLiteral("Alert Status")},
        {0x2A40, QStringLiteral("Ringer Control Point")},
        {0x2A41, QStringLiteral("Ringer Setting")},
        {0x2A42, QStringLiteral("Alert Category ID Bit Mask")},
        {0x2A43, QStringLiteral("Alert Category ID")},
        {0x2A44, QStringLiteral("Alert Notification Control Point")},
        {0x2A45, QStringLiteral("Unread Alert Status")},
        {0x2A46, QStringLiteral("New Alert")},
        {0x2A47, QStringLiteral("Supported New Alert Category")},
        {0x2A48, QStringLiteral("Supported Unread Alert Category")},
        {0x2A49, QStringLiteral("Blood Pressure Feature")},
        {0x2A4A, QStringLiteral("HID Information")},
        {0x2A4B, QStringLiteral("Report Map")},
        {0x2A4C, QStringLiteral("HID Control Point")},
        {0x2A4D, QStringLiteral("Report")},
        {0x2A4E, QStringLiteral("Protocol Mode")},
        {0x2A4F, QStringLiteral("Scan Interval Window")},
        {0x2A50, QStringLiteral("PnP ID")},
        {0x2A51, QStringLiteral("Glucose Feature")},
        {0x2A52, QStringLiteral("Record Access Control Point")},
        {0x2A53, QStringLiteral("RSC Measurement")},
        {0x2A54, QStringLiteral("RSC Feature")},
        {0x2A55, QStringLiteral("SC Control Point")},
        {0x2A5A, QStringLiteral("Aggregate")},
        {0x2A5B, QStringLiteral("CSC Measurement")},
        {0x2A5C, QStringLiteral("CSC Feature")},
        {0x2A5D, QStringLiteral("Sensor Location")},
        {0x2A5E, QStringLiteral("PLX Spot-Check Measurement")},
        {0x2A5F, QStringLiteral("PLX Continuous Measurement")},
        {0x2A60, QStringLiteral("PLX Features")},
        {0x2A63, QStringLiteral("Cycling Power Measurement")},
        {0x2A64, QStringLiteral("Cycling Power Vector")},
        {0x2A65, QStringLiteral("Cycling Power Feature")},
        {0x2A66, QStringLiteral("Cycling Power Control Point")},
        {0x2A67, QStringLiteral("Location and Speed")},
        {0x2A68, QStringLiteral("Navigation")},
        {0x2A69, QStringLiteral("Position Quality")},
        {0x2A6A, QStringLiteral("LN Feature")},
        {0x2A6B, QStringLiteral("LN Control Point")},
        {0x2A6C, QStringLiteral("Elevation")},
        {0x2A6D, QStringLiteral("Pressure")},
        {0x2A6E, QStringLiteral("Temperature")},
        {0x2A6F, QStringLiteral("Humidity")},
        {0x2A70, QStringLiteral("True Wind Speed")},
        {0x2A71, QStringLiteral("True Wind Direction")},
        {0x2A72, QStringLiteral("Apparent Wind Speed")},
        {0x2A73, QStringLiteral("Apparent Wind Direction")},
        {0x2A74, QStringLiteral("Gust Factor")},
        {0x2A75, QStringLiteral("Pollen Concentration")},
        {0x2A76, QStringLiteral("UV Index")},
        {0x2A77, QStringLiteral("Irradiance")},
        {0x2A78, QStringLiteral("Rainfall")},
        {0x2A79, QStringLiteral("Wind Chill")},
        {0x2A7A, QStringLiteral("Heat Index")},
        {0x2A7B, QStringLiteral("Dew Point")},
        {0x2A7D, QStringLiteral("Descriptor Value Changed")},
        {0x2A7E, QStringLiteral("Aerobic Heart Rate Lower Limit")},
        {0x2A7F, QStringLiteral("Aerobic Threshold")},
        {0x2A80, QStringLiteral("Age")},
        {0x2A81, QStringLiteral("Anaerobic Heart Rate Lower Limit")},
        {0x2A82, QStringLiteral("Anaerobic Heart Rate Upper Limit")},
        {0x2A83, QStringLiteral("Anaerobic Threshold")},
        {0x2A84, QStringLiteral("Aerobic Heart Rate Upper Limit")},
        {0x2A85, QStringLiteral("Date of Birth")},
        {0x2A86, QStringLiteral("Date of Threshold Assessment")},
        {0x2A87, QStringLiteral("Email Address")},
        {0x2A88, QStringLiteral("Fat Burn Heart Rate Lower Limit")},
        {0x2A89, QStringLiteral("Fat Burn Heart Rate Upper Limit")},
        {0x2A8A, QStringLiteral("First Name")},
        {0x2A8B, QStringLiteral("Five Zone Heart Rate Limits")},
        {0x2A8C, QStringLiteral("Gender")},
        {0x2A8D, QStringLiteral("Heart Rate Max")},
        {0x2A8E, QStringLiteral("Height")},
        {0x2A8F, QStringLiteral("Hip Circumference")},
        {0x2A90, QStringLiteral("Last Name")},
        {0x2A91, QStringLiteral("Maximum Recommended Heart Rate")},
        {0x2A92, QStringLiteral("Resting Heart Rate")},
        {0x2A93, QStringLiteral("Sport Type for Aerobic and Anaerobic Thresholds")},
        {0x2A94, QStringLiteral("Three Zone Heart Rate Limits")},
        {0x2A95, QStringLiteral("Two Zone Heart Rate Limits")},
        {0x2A96, QStringLiteral("VO2 Max")},
        {0x2A97, QStringLiteral("Waist Circumference")},
        {0x2A98, QStringLiteral("Weight")},
        {0x2A99, QStringLiteral("Database Change Increment")},
        {0x2A9A, QStringLiteral("User Index")},
        {0x2A9B, QStringLiteral("Body Composition Feature")},
        {0x2A9C, QStringLiteral("Body Composition Measurement")},
        {0x2A9D, QStringLiteral("Weight Measurement")},
        {0x2A9E, QStringLiteral("Weight Scale Feature")},
        {0x2A9F, QStringLiteral("User Control Point")},
        {0x2AA0, QStringLiteral("Magnetic Flux Density - 2D")},
        {0x2AA1, QStringLiteral("Magnetic Flux Density - 3D")},
        {0x2AA2, QStringLiteral("Language")},
        {0x2AA3, QStringLiteral("Barometric Pressure Trend")},
        {0x2AA4, QStringLiteral("Bond Management Control Point")},
        {0x2AA5, QStringLiteral("Bond Management Feature")},
        {0x2AA6, QStringLiteral("Central Address Resolution")},
        {0x2AA7, QStringLiteral("CGM Measurement")},
        {0x2AA8, QStringLiteral("CGM Feature")},
        {0x2AA9, QStringLiteral("CGM Status")},
        {0x2AAA, QStringLiteral("CGM Session Start Time")},
        {0x2AAB, QStringLiteral("CGM Session Run Time")},
        {0x2AAC, QStringLiteral("CGM Specific Ops Control Point")},
        {0x2AAD, QStringLiteral("Indoor Positioning Configuration")},
        {0x2AAE, QStringLiteral("Latitude")},
        {0x2AAF, QStringLiteral("Longitude")},
        {0x2AB0, QStringLiteral("Local North Coordinate")},
        {0x2AB1, QStringLiteral("Local East Coordinate")},
        {0x2AB2, QStringLiteral("Floor Number")},
        {0x2AB3, QStringLiteral("Altitude")},
        {0x2AB4, QStringLiteral("Uncertainty")},
        {0x2AB5, QStringLiteral("Location Name")},
        {0x2AB6, QStringLiteral("URI")},
        {0x2AB7, QStringLiteral("HTTP Headers")},
        {0x2AB8, QStringLiteral("HTTP Status Code")},
        {0x2AB9, QStringLiteral("HTTP Entity Body")},
        {0x2ABA, QStringLiteral("HTTP Control Point")},
        {0x2ABB, QStringLiteral("HTTPS Security")},
        {0x2ABC, QStringLiteral("TDS Control Point")},
        {0x2ABD, QStringLiteral("OTS Feature")},
        {0x2ABE, QStringLiteral("Object Name")},
        {0x2ABF, QStringLiteral("Object Type")},
        {0x2AC0, QStringLiteral("Object Size")},
        {0x2AC1, QStringLiteral("Object First-Created")},
        {0x2AC2, QStringLiteral("Object Last-Modified")},
        {0x2AC3, QStringLiteral("Object ID")},
        {0x2AC4, QStringLiteral("Object Properties")},
        {0x2AC5, QStringLiteral("Object Action Control Point")},
        {0x2AC6, QStringLiteral("Object List Control Point")},
        {0x2AC7, QStringLiteral("Object List Filter")},
        {0x2AC8, QStringLiteral("Object Changed")},
        {0x2AC9, QStringLiteral("Resolvable Private Address Only")},
        {0x2ACC, QStringLiteral("Fitness Machine Feature")},
        {0x2ACD, QStringLiteral("Treadmill Data")},
        {0x2ACE, QStringLiteral("Cross Trainer Data")},
        {0x2ACF, QStringLiteral("Step Climber Data")},
        {0x2AD0, QStringLiteral("Stair Climber Data")},
        {0x2AD1, QStringLiteral("Rower Data")},
        {0x2AD2, QStringLiteral("Indoor Bike Data")},
        {0x2AD3, QStringLiteral("Training Status")},
        {0x2AD4, QStringLiteral("Supported Speed Range")},
        {0x2AD5, QStringLiteral("Supported Inclination Range")},
        {0x2AD6, QStringLiteral("Supported Resistance Level Range")},
        {0x2AD7, QStringLiteral("Supported Heart Rate Range")},
        {0x2AD8, QStringLiteral("Supported Power Range")},
        {0x2AD9, QStringLiteral("Fitness Machine Control Point")},
        {0x2ADA, QStringLiteral("Fitness Machine Status")},
        {0x2ADB, QStringLiteral("Mesh Provisioning Data In")},
        {0x2ADC, QStringLiteral("Mesh Provisioning Data Out")},
        {0x2ADD, QStringLiteral("Mesh Proxy Data In")},
        {0x2ADE, QStringLiteral("Mesh Proxy Data Out")},
        {0x2AE0, QStringLiteral("Average Current")},
        {0x2AE1, QStringLiteral("Average Voltage")},
        {0x2AE2, QStringLiteral("Boolean")},
        {0x2AE3, QStringLiteral("Chromatic Distance from Planckian")},
        {0x2AE4, QStringLiteral("Chromaticity Coordinates")},
        {0x2AE5, QStringLiteral("Chromaticity in CCT and Duv Values")},
        {0x2AE6, QStringLiteral("Chromaticity Tolerance")},
        {0x2AE7, QStringLiteral("CIE 13.3-1995 Color Rendering Index")},
        {0x2AE8, QStringLiteral("Coefficient")},
        {0x2AE9, QStringLiteral("Correlated Color Temperature")},
        {0x2AEA, QStringLiteral("Count 16")},
        {0x2AEB, QStringLiteral("Count 24")},
        {0x2AEC, QStringLiteral("Country Code")},
        {0x2AED, QStringLiteral("Date UTC")},
        {0x2AEE, QStringLiteral("Electric Current")},
        {0x2AEF, QStringLiteral("Electric Current Range")},
        {0x2AF0, QStringLiteral("Electric Current Specification")},
        {0x2AF1, QStringLiteral("Electric Current Statistics")},
        {0x2AF2, QStringLiteral("Energy")},
        {0x2AF3, QStringLiteral("Energy in a Period of Day")},
        {0x2AF4, QStringLiteral("Event Statistics")},
        {0x2AF5, QStringLiteral("Fixed String 16")},
        {0x2AF6, QStringLiteral("Fixed String 24")},
        {0x2AF7, QStringLiteral("Fixed String 36")},
        {0x2AF8, QStringLiteral("Fixed String 8")},
        {0x2AF9, QStringLiteral("Generic Level")},
        {0x2AFA, QStringLiteral("Global Trade Item Number")},
        {0x2AFB, QStringLiteral("Illuminance")},
        {0x2AFC, QStringLiteral("Luminous Efficacy")},
        {0x2AFD, QStringLiteral("Luminous Energy")},
        {0x2AFE, QStringLiteral("Luminous Exposure")},
        {0x2AFF, QStringLiteral("Luminous Flux")},
        {0x2B00, QStringLiteral("Luminous Flux Range")},
        {0x2B01, QStringLiteral("Luminous Intensity")},
        {0x2B02, QStringLiteral("Mass Flow")},
        {0x2B03, QStringLiteral("Perceived Lightness")},
        {0x2B04, QStringLiteral("Percentage 8")},
        {0x2B05, QStringLiteral("Power")},
        {0x2B06, QStringLiteral("Power Specification")},
        {0x2B07, QStringLiteral("Relative Runtime in a Current Range")},
        {0x2B08, QStringLiteral("Relative Runtime in a Generic Level Range")},
        {0x2B09, QStringLiteral("Relative Value in a Voltage Range")},
        {0x2B0A, QStringLiteral("Relative Value in an Illuminance Range")},
        {0x2B0B, QStringLiteral("Relative Value in a Period of Day")},
        {0x2B0C, QStringLiteral("Relative Value in a Temperature Range")},
        {0x2B0D, QStringLiteral("Temperature 8")},
        {0x2B0E, QStringLiteral("Temperature 8 in a Period of Day")},
        {0x2B0F, QStringLiteral("Temperature 8 Statistics")},
        {0x2B10, QStringLiteral("Temperature Range")},
        {0x2B11, QStringLiteral("Temperature Statistics")},
        {0x2B12, QStringLiteral("Time Decihour 8")},
        {0x2B13, QStringLiteral("Time Exponential 8")},
        {0x2B14, QStringLiteral("Time Hour 24")},
        {0x2B15, QStringLiteral("Time Millisecond 24")},
        {0x2B16, QStringLiteral("Time Second 16")},
        {0x2B17, QStringLiteral("Time Second 8")},
        {0x2B18, QStringLiteral("Voltage")},
        {0x2B19, QStringLiteral("Voltage Specification")},
        {0x2B1A, QStringLiteral("Voltage Statistics")},
        {0x2B1B, QStringLiteral("Volume Flow")},
        {0x2B1C, QStringLiteral("Chromaticity Coordinate")},
        {0x2B1D, QStringLiteral("RC Feature")},
        {0x2B1E, QStringLiteral("RC Settings")},
        {0x2B1F, QStringLiteral("Reconnection Configuration Control Point")},
        {0x2B20, QStringLiteral("IDD Status Changed")},
        {0x2B21, QStringLiteral("IDD Status")},
        {0x2B22, QStringLiteral("IDD Annunciation Status")},
        {0x2B23, QStringLiteral("IDD Features")},
        {0x2B24, QStringLiteral("IDD Status Reader Control Point")},
        {0x2B25, QStringLiteral("IDD Command Control Point")},
        {0x2B26, QStringLiteral("IDD Command Data")},
        {0x2B27, QStringLiteral("IDD Record Access Control Point")},
        {0x2B28, QStringLiteral("IDD History Data")},
        {0x2B29, QStringLiteral("Client Supported Features")},
        {0x2B2A, QStringLiteral("Database Hash")},
        {0x2B2B, QStringLiteral("BSS Control Point")},
        {0x2B2C, QStringLiteral("BSS Response")},
        {0x2B2D, QStringLiteral("Emergency ID")},
        {0x2B2E, QStringLiteral("Emergency Text")},
        {0x2B2F, QStringLiteral("ACS Status")},
        {0x2B30, QStringLiteral("ACS Data In")},
        {0x2B31, QStringLiteral("ACS Data Out Notify")},
        {0x2B32, QStringLiteral("ACS Data Out Indicate")},
        {0x2B33, QStringLiteral("ACS Control Point")},
        {0x2B34, QStringLiteral("Enhanced Blood Pressure Measurement")},
        {0x2B35, QStringLiteral("Enhanced Intermediate Cuff Pressure")},
        {0x2B36, QStringLiteral("Blood Pressure Record")},
        {0x2B37, QStringLiteral("Registered User")},
        {0x2B38, QStringLiteral("BR-EDR Handover Data")},
        {0x2B39, QStringLiteral("Bluetooth SIG Data")},
        {0x2B3A, QStringLiteral("Server Supported Features")},
        {0x2B3B, QStringLiteral("Physical Activity Monitor Features")},
        {0x2B3C, QStringLiteral("General Activity Instantaneous Data")},
        {0x2B3D, QStringLiteral("General Activity Summary Data")},
        {0x2B3E, QStringLiteral("CardioRespiratory Activity Instantaneous Data")},
        {0x2B3F, QStringLiteral("CardioRespiratory Activity Summary Data")},
        {0x2B40, QStringLiteral("Step Counter Activity Summary Data")},
        {0x2B41, QStringLiteral("Sleep Activity Instantaneous Data")},
        {0x2B42, QStringLiteral("Sleep Activity Summary Data")},
        {0x2B43, QStringLiteral("Physical Activity Monitor Control Point")},
        {0x2B44, QStringLiteral("Physical Activity Current Session")},
        {0x2B45, QStringLiteral("Physical Activity Session Descriptor")},
        {0x2B46, QStringLiteral("Preferred Units")},
        {0x2B47, QStringLiteral("High Resolution Height")},
        {0x2B48, QStringLiteral("Middle Name")},
        {0x2B49, QStringLiteral("Stride Length")},
        {0x2B4A, QStringLiteral("Handedness")},
        {0x2B4B, QStringLiteral("Device Wearing Position")},
        {0x2B4C, QStringLiteral("Four Zone Heart Rate Limits")},
        {0x2B4D, QStringLiteral("High Intensity Exercise Threshold")},
        {0x2B4E, QStringLiteral("Activity Goal")},
        {0x2B4F, QStringLiteral("Sedentary Interval Notification")},
        {0x2B50, QStringLiteral("Caloric Intake")},
        {0x2B51, QStringLiteral("TMAP Role")},
        {0x2B77, QStringLiteral("Audio Input State")},
        {0x2B78, QStringLiteral("Gain Settings Attribute")},
        {0x2B79, QStringLiteral("Audio Input Type")},
        {0x2B7A, QStringLiteral("Audio Input Status")},
        {0x2B7B, QStringLiteral("Audio Input Control Point")},
        {0x2B7C, QStringLiteral("Audio Input Description")},
        {0x2B7D, QStringLiteral("Volume State")},
        {0x2B7E, QStringLiteral("Volume Control Point")},
        {0x2B7F, QStringLiteral("Volume Flags")},
        {0x2B80, QStringLiteral("Volume Offset State")},
        {0x2B81, QStringLiteral("Audio Location")},
        {0x2B82, QStringLiteral("Volume Offset Control Point")},
        {0x2B83, QStringLiteral("Audio Output Description")},
        {0x2B84, QStringLiteral("Set Identity Resolving Key")},
        {0x2B85, QStringLiteral("Coordinated Set Size")},
        {0x2B86, QStringLiteral("Set Member Lock")},
        {0x2B87, QStringLiteral("Set Member Rank")},
        {0x2B88, QStringLiteral("Encrypted Data Key Material")},
        {0x2B89, QStringLiteral("Apparent Energy 32")},
        {0x2B8A, QStringLiteral("Apparent Power")},
        {0x2B8B, QStringLiteral("Live Health Observations")},
        {0x2B8C, QStringLiteral("CO\\textsubscript{2} Concentration")},
        {0x2B8D, QStringLiteral("Cosine of the Angle")},
        {0x2B8E, QStringLiteral("Device Time Feature")},
        {0x2B8F, QStringLiteral("Device Time Parameters")},
        {0x2B90, QStringLiteral("Device Time")},
        {0x2B91, QStringLiteral("Device Time Control Point")},
        {0x2B92, QStringLiteral("Time Change Log Data")},
        {0x2B93, QStringLiteral("Media Player Name")},
        {0x2B94, QStringLiteral("Media Player Icon Object ID")},
        {0x2B95, QStringLiteral("Media Player Icon URL")},
        {0x2B96, QStringLiteral("Track Changed")},
        {0x2B97, QStringLiteral("Track Title")},
        {0x2B98, QStringLiteral("Track Duration")},
        {0x2B99, QStringLiteral("Track Position")},
        {0x2B9A, QStringLiteral("Playback Speed")},
        {0x2B9B, QStringLiteral("Seeking Speed")},
        {0x2B9C, QStringLiteral("Current Track Segments Object ID")},
        {0x2B9D, QStringLiteral("Current Track Object ID")},
        {0x2B9E, QStringLiteral("Next Track Object ID")},
        {0x2B9F, QStringLiteral("Parent Group Object ID")},
        {0x2BA0, QStringLiteral("Current Group Object ID")},
        {0x2BA1, QStringLiteral("Playing Order")},
        {0x2BA2, QStringLiteral("Playing Orders Supported")},
        {0x2BA3, QStringLiteral("Media State")},
        {0x2BA4, QStringLiteral("Media Control Point")},
        {0x2BA5, QStringLiteral("Media Control Point Opcodes Supported")},
        {0x2BA6, QStringLiteral("Search Results Object ID")},
        {0x2BA7, QStringLiteral("Search Control Point")},
        {0x2BA8, QStringLiteral("Energy 32")},
        {0x2BAD, QStringLiteral("Constant Tone Extension Enable")},
        {0x2BAE, QStringLiteral("Advertising Constant Tone Extension Minimum Length")},
        {0x2BAF, QStringLiteral("Advertising Constant Tone Extension Minimum Transmit Count")},
        {0x2BB0, QStringLiteral("Advertising Constant Tone Extension Transmit Duration")},
        {0x2BB1, QStringLiteral("Advertising Constant Tone Extension Interval")},
        {0x2BB2, QStringLiteral("Advertising Constant Tone Extension PHY")},
        {0x2BB3, QStringLiteral("Bearer Provider Name")},
        {0x2BB4, QStringLiteral("Bearer UCI")},
        {0x2BB5, QStringLiteral("Bearer Technology")},
        {0x2BB6, QStringLiteral("Bearer URI Schemes Supported List")},
        {0x2BB7, QStringLiteral("Bearer Signal Strength")},
        {0x2BB8, QStringLiteral("Bearer Signal Strength Reporting Interval")},
        {0x2BB9, QStringLiteral("Bearer List Current Calls")},
        {0x2BBA, QStringLiteral("Content Control ID")},
        {0x2BBB, QStringLiteral("Status Flags")},
        {0x2BBC, QStringLiteral("Incoming Call Target Bearer URI")},
        {0x2BBD, QStringLiteral("Call State")},
        {0x2BBE, QStringLiteral("Call Control Point")},
        {0x2BBF, QStringLiteral("Call Control Point Optional Opcodes")},
        {0x2BC0, QStringLiteral("Termination Reason")},
        {0x2BC1, QStringLiteral("Incoming Call")},
        {0x2BC2, QStringLiteral("Call Friendly Name")},
        {0x2BC3, QStringLiteral("Mute")},
        {0x2BC4, QStringLiteral("Sink ASE")},
        {0x2BC5, QStringLiteral("Source ASE")},
        {0x2BC6, QStringLiteral("ASE Control Point")},
        {0x2BC7, QStringLiteral("Broadcast Audio Scan Control Point")},
        {0x2BC8, QStringLiteral("Broadcast Receive State")},
        {0x2BC9, QStringLiteral("Sink PAC")},
        {0x2BCA, QStringLiteral("Sink Audio Locations")},
        {0x2BCB, QStringLiteral("Source PAC")},
        {0x2BCC, QStringLiteral("Source Audio Locations")},
        {0x2BCD, QStringLiteral("Available Audio Contexts")},
        {0x2BCE, QStringLiteral("Supported Audio Contexts")},
        {0x2BCF, QStringLiteral("Ammonia Concentration")},
        {0x2BD0, QStringLiteral("Carbon Monoxide Concentration")},
        {0x2BD1, QStringLiteral("Methane Concentration")},
        {0x2BD2, QStringLiteral("Nitrogen Dioxide Concentration")},
        {0x2BD3, QStringLiteral("Non-Methane Volatile Organic Compounds Concentration")},
        {0x2BD4, QStringLiteral("Ozone Concentration")},
        {0x2BD5, QStringLiteral("Particulate Matter - PM1 Concentration")},
        {0x2BD6, QStringLiteral("Particulate Matter - PM2.5 Concentration")},
        {0x2BD7, QStringLiteral("Particulate Matter - PM10 Concentration")},
        {0x2BD8, QStringLiteral("Sulfur Dioxide Concentration")},
        {0x2BD9, QStringLiteral("Sulfur Hexafluoride Concentration")},
        {0x2BDA, QStringLiteral("Hearing Aid Features")},
        {0x2BDB, QStringLiteral("Hearing Aid Preset Control Point")},
        {0x2BDC, QStringLiteral("Active Preset Index")},
        {0x2BDD, QStringLiteral("Stored Health Observations")},
        {0x2BDE, QStringLiteral("Fixed String 64")},
        {0x2BDF, QStringLiteral("High Temperature")},
        {0x2BE0, QStringLiteral("High Voltage")},
        {0x2BE1, QStringLiteral("Light Distribution")},
        {0x2BE2, QStringLiteral("Light Output")},
        {0x2BE3, QStringLiteral("Light Source Type")},
        {0x2BE4, QStringLiteral("Noise")},
        {0x2BE5, QStringLiteral("Relative Runtime in a Correlated Color Temperature Range")},
        {0x2BE6, QStringLiteral("Time Second 32")},
        {0x2BE7, QStringLiteral("VOC Concentration")},
        {0x2BE8, QStringLiteral("Voltage Frequency")},
        {0x2BE9, QStringLiteral("Battery Critical Status")},
        {0x2BEA, QStringLiteral("Battery Health Status")},
        {0x2BEB, QStringLiteral("Battery Health Information")},
        {0x2BEC, QStringLiteral("Battery Information")},
        {0x2BED, QStringLiteral("Battery Level Status")},
        {0x2BEE, QStringLiteral("Battery Time Status")},
        {0x2BEF, QStringLiteral("Estimated Service Date")},
        {0x2BF0, QStringLiteral("Battery Energy Status")},
        {0x2BF1, QStringLiteral("Observation Schedule Changed")},
        {0x2BF2, QStringLiteral("Elapsed Time")},
        {0x2BF3, QStringLiteral("Health Sensor Features")},
        {0x2BF4, QStringLiteral("GHS Control Point")},
        {0x2BF5, QStringLiteral("LE GATT Security Levels")},
        {0x2BF6, QStringLiteral("ESL Address")},
        {0x2BF7, QStringLiteral("AP Sync Key Material")},
        {0x2BF8, QStringLiteral("ESL Response Key Material")},
        {0x2BF9, QStringLiteral("ESL Current Absolute Time")},
        {0x2BFA, QStringLiteral("ESL Display Information")},
        {0x2BFB, QStringLiteral("ESL Image Information")},
        {0x2BFC, QStringLiteral("ESL Sensor Information")},
        {0x2BFD, QStringLiteral("ESL LED Information")},
        {0x2BFE, QStringLiteral("ESL Control Point")},
        {0x2BFF, QStringLiteral("UDI for Medical Devices")},
        {0x2C00, QStringLiteral("GMAP Role")},
        {0x2C01, QStringLiteral("UGG Features")},
        {0x2C02, QStringLiteral("UGT Features")},
        {0x2C03, QStringLiteral("BGS Features")},
        {0x2C04, QStringLiteral("BGR Features")},
        {0x2C05, QStringLiteral("Percentage 8 Steps")},
        {0x2C06, QStringLiteral("Acceleration")},
        {0x2C07, QStringLiteral("Force")},
        {0x2C08, QStringLiteral("Linear Position")},
        {0x2C09, QStringLiteral("Rotational Speed")},
        {0x2C0A, QStringLiteral("Length")},
        {0x2C0B, QStringLiteral("Torque")},
        {0x2C0C, QStringLiteral("IMD Status")},
        {0x2C0D, QStringLiteral("IMDS Descriptor Value Changed")},
        {0x2C0E, QStringLiteral("First Use Date")},
        {0x2C0F, QStringLiteral("Life Cycle Data")},
        {0x2C10, QStringLiteral("Work Cycle Data")},
        {0x2C11, QStringLiteral("Service Cycle Data")},
        {0x2C12, QStringLiteral("IMD Control")},
        {0x2C13, QStringLiteral("IMD Historical Data")},
        {0x2C14, QStringLiteral("RAS Features")},
        {0x2C15, QStringLiteral("Real-time Ranging Data")},
        {0x2C16, QStringLiteral("On-demand Ranging Data")},
        {0x2C17, QStringLiteral("RAS Control Point")},
        {0x2C18, QStringLiteral("Ranging Data Ready")},
        {0x2C19, QStringLiteral("Ranging Data Overwritten")},
        {0x2C1A, QStringLiteral("Coordinated Set Name")},
        {0x2C1B, QStringLiteral("Humidity 8")},
        {0x2C1C, QStringLiteral("Illuminance 16")},
        {0x2C1D, QStringLiteral("Acceleration - 3D")},
        {0x2C1E, QStringLiteral("Precise Acceleration - 3D")},
        {0x2C1F, QStringLiteral("Acceleration Detection Status")},
        {0x2C20, QStringLiteral("Door/Window Status")},
        {0x2C21, QStringLiteral("Pushbutton Status 8")},
        {0x2C22, QStringLiteral("Contact Status 8")},
        {0x2C23, QStringLiteral("HID ISO Properties")},
        {0x2C24, QStringLiteral("LE HID Operation Mode")},
        {0x2C25, QStringLiteral("Cookware Description")},
        {0x2C26, QStringLiteral("Recipe Control")},
        {0x2C27, QStringLiteral("Recipe Parameters")},
        {0x2C28, QStringLiteral("Cooking Step Status")},
        {0x2C29, QStringLiteral("Cooking Zone Capabilities")},
        {0x2C2A, QStringLiteral("Cooking Zone Desired Cooking Conditions")},
        {0x2C2B, QStringLiteral("Cooking Zone Actual Cooking Conditions")},
        {0x2C2C, QStringLiteral("Cookware Sensor Data")},
        {0x2C2D, QStringLiteral("Cookware Sensor Aggregate")},
        {0x2C2E, QStringLiteral("Cooking Temperature")},
        {0x2C2F, QStringLiteral("Cooking Zone Perceived Power")},
        {0x2C30, QStringLiteral("Kitchen Appliance Airflow")},
        {0x2C31, QStringLiteral("Voice Assistant Name")},
        {0x2C32, QStringLiteral("Voice Assistant UUID")},
        {0x2C33, QStringLiteral("Voice Assistant Service Control Point")},
        {0x2C34, QStringLiteral("Installed Location")},
        {0x2C35, QStringLiteral("Voice Assistant Session State")},
        {0x2C36, QStringLiteral("Voice Assistant Session Flag")},
        {0x2C37, QStringLiteral("Voice Assistant Supported Languages")},
        {0x2C38, QStringLiteral("Voice Assistant Supported Features")},
        {0x2C39, QStringLiteral("HID SCI Mode")},
        {0x2C3A, QStringLiteral("HID SCI Information")},
        {0x2C3B, QStringLiteral("Tire Pressure")},
        {0x2C3C, QStringLiteral("Tire Temperature")},
        {0x2C3D, QStringLiteral("Tire Acceleration")},
        {0x2C3E, QStringLiteral("TPMS Properties")},
        {0x2C3F, QStringLiteral("TPMS Duty Cycle")},
        {0x2C40, QStringLiteral("TPMS Position")},
        {0x2C41, QStringLiteral("TPMS Signing Key")},
        {0x2C42, QStringLiteral("Cookware Device ID")}
    };
    return names.value(static_cast<quint16>(uuid.data1), QStringLiteral(""));
}

// qt -> suffix
template<>
ModbusCRC uni_cast<ModbusCRC, QByteArray>(const QByteArray &s, const int depth) {
    Q_UNUSED(depth);
    static constexpr quint16 table[256] = {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
    };

    quint16 crc = 0xFFFF;
    for (const unsigned char byte: s) {
        const quint8 nTemp = static_cast<quint8>(byte) ^ static_cast<quint8>(crc & 0xFF);
        crc = crc >> 8 ^ table[nTemp];
    }

    QByteArray checksum;
    checksum.reserve(2);
    checksum.append(static_cast<char>(crc & 0xFF));
    checksum.append(static_cast<char>(crc >> 8 & 0xFF));
    return checksum;
}

template<>
ModbusLRC uni_cast<ModbusLRC, QByteArray>(const QByteArray &s, const int depth) {
    Q_UNUSED(depth);
    if (s.isEmpty()) return QByteArray{"00\r\n"};

    quint8 lrc = 0x00;
    for (const unsigned char byte: QByteArray::fromHex(s)) {
        lrc += byte;
    }
    lrc = static_cast<quint8>(-lrc);

    QByteArray checksum;
    checksum.reserve(4);
    checksum.append(QString("%1").arg(lrc, 2, 16, QChar('0')).toUpper().toLatin1());
    checksum.append('\r');
    checksum.append('\n');
    return checksum;
}

// vterm -> qt
template<>
QColor uni_cast<QColor, VTermColor>(const VTermScreen *vts, const VTermColor &s, const int depth) {
    Q_UNUSED(depth);
    VTermColor d = s;
    vterm_screen_convert_color_to_rgb(vts, &d);
    return {d.rgb.red, d.rgb.green, d.rgb.blue};
}

template<>
TerminalCell uni_cast<TerminalCell, VTermScreenCell>(const VTermScreen *vts, const VTermScreenCell &s, const int depth) {
    Q_UNUSED(depth);
    TerminalCell d{};
    d.width = static_cast<int>(s.width);

    if (s.chars[0] == UINT32_MAX) {
        d.text = QString{};
        d.width = 0;
    } else if (s.chars[0] == 0 || s.chars[0] == ' ') {
        d.text = ' ';
    } else {
        int length = 0;
        while (length < VTERM_MAX_CHARS_PER_CELL && s.chars[length] != 0 && s.chars[length] != UINT32_MAX) ++length;
        d.text = QString::fromUcs4(s.chars, length);
    }

    d.bold = s.attrs.bold;
    d.underline = s.attrs.underline != VTERM_UNDERLINE_OFF;
    d.italic = s.attrs.italic;
    d.strike = s.attrs.strike;

    d.foreground = uni_cast<QColor>(vts, s.fg);
    d.background = uni_cast<QColor>(vts, s.bg);

    d.uri = s.uri;
    d.dim = s.attrs.dim;
    d.overline = s.attrs.overline;

    if (s.attrs.reverse) std::swap(d.foreground, d.background);
    if (d.dim) {
        d.foreground = QColor(
            (d.foreground.red() * 55 + d.background.red() * 45) / 100,
            (d.foreground.green() * 55 + d.background.green() * 45) / 100,
            (d.foreground.blue() * 55 + d.background.blue() * 45) / 100
        );
    }
    if (s.attrs.conceal) d.foreground = d.background;

    return d;
}

// qt-> vterm
template<>
VTermColor uni_cast<VTermColor, QColor>(const QColor &s, const int depth) {
    Q_UNUSED(depth);
    VTermColor d{};
    vterm_color_rgb(&d, s.red(), s.green(), s.blue());
    return d;
}

template<>
VTermButton uni_cast<VTermButton, int>(const int &s, const int depth) {
    Q_UNUSED(depth);
    switch (s) {
        case Qt::LeftButton:
            return 1;
        case Qt::MiddleButton:
            return 2;
        case Qt::RightButton:
            return 3;
        default:
            return 0;
    }
}

template<>
VTermKey uni_cast<VTermKey, int>(const int &s, const int depth) {
    Q_UNUSED(depth);
    int d = VTERM_KEY_NONE;
    switch (s) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            d = VTERM_KEY_ENTER;
            break;
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            d = VTERM_KEY_TAB;
            break;
        case Qt::Key_Backspace:
            d = VTERM_KEY_BACKSPACE;
            break;
        case Qt::Key_Escape:
            d = VTERM_KEY_ESCAPE;
            break;
        case Qt::Key_Up:
            d = VTERM_KEY_UP;
            break;
        case Qt::Key_Down:
            d = VTERM_KEY_DOWN;
            break;
        case Qt::Key_Left:
            d = VTERM_KEY_LEFT;
            break;
        case Qt::Key_Right:
            d = VTERM_KEY_RIGHT;
            break;
        case Qt::Key_Insert:
            d = VTERM_KEY_INS;
            break;
        case Qt::Key_Delete:
            d = VTERM_KEY_DEL;
            break;
        case Qt::Key_Home:
            d = VTERM_KEY_HOME;
            break;
        case Qt::Key_End:
            d = VTERM_KEY_END;
            break;
        case Qt::Key_PageUp:
            d = VTERM_KEY_PAGEUP;
            break;
        case Qt::Key_PageDown:
            d = VTERM_KEY_PAGEDOWN;
            break;
        default:
            if (s >= Qt::Key_F1 && s <= Qt::Key_F35) {
                d = VTERM_KEY_FUNCTION(s - Qt::Key_F1 + 1);
            }
            break;
    }
    return static_cast<VTermKey>(d);
}

template<>
VTermModifier uni_cast<VTermModifier, int>(const int &s, const int depth) {
    Q_UNUSED(depth);
    int d = VTERM_MOD_NONE;
    if (s & Qt::ShiftModifier) d |= VTERM_MOD_SHIFT;
    if (s & Qt::AltModifier) d |= VTERM_MOD_ALT;
    if (s & Qt::ControlModifier) d |= VTERM_MOD_CTRL;
    return static_cast<VTermModifier>(d);
}
