#include "util/uniCast.h"

#include <QDir>
#include <QMovie>
#include <QTime>
#include <sol/state_view.hpp>
#include <sol/table_core.hpp>
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
            if (s.is<int>()) {
                return s.as<int>();
            }
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
    auto *document = cmark_parse_document(markdown.constData(), markdown.size(), CMARK_OPT_FOOTNOTES);

    struct CodeBlock {
        int startLine{};
        int endLine{};
        QString content{};
        QString language{};
    };

    QList<CodeBlock> codeBlocks{};
    auto *iterator = cmark_iter_new(document);
    while (cmark_iter_next(iterator) != CMARK_EVENT_DONE) {
        auto *node = cmark_iter_get_node(iterator);
        if (cmark_iter_get_event_type(iterator) != CMARK_EVENT_ENTER || cmark_node_get_type(node) != CMARK_NODE_CODE_BLOCK) continue;
        auto content = QString::fromUtf8(cmark_node_get_literal(node));
        if (content.endsWith('\n')) content.chop(1);
        codeBlocks.append({
            .startLine = cmark_node_get_start_line(node),
            .endLine = cmark_node_get_end_line(node),
            .content = content,
            .language = QString::fromUtf8(cmark_node_get_fence_info(node)).section(' ', 0, 0)
        });
    }
    cmark_iter_free(iterator);
    cmark_node_free(document);

    const auto lines = s.split('\n', Qt::KeepEmptyParts);
    QList<MarkdownBlock> blocks{};
    qsizetype currentLine{};
    for (const auto &codeBlock: codeBlocks) {
        const qsizetype startLine = codeBlock.startLine - 1;
        const qsizetype endLine = codeBlock.endLine;
        const auto content = lines.sliced(currentLine, startLine - currentLine).join('\n');
        if (!content.trimmed().isEmpty()) blocks.append({MarkdownBlock::Type::Markdown, content, {}});
        blocks.append({MarkdownBlock::Type::Code, codeBlock.content, codeBlock.language});
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
QLifetime uni_cast<QLifetime, qint64>(const qint64 &s, const int depth) {
    Q_UNUSED(depth);
    const qint64 totalSeconds = s / 1000;
    const qint64 days = totalSeconds / 86400;
    const int millisecondsOfDay = static_cast<int>(totalSeconds % 86400 * 1000);
    const auto time = QTime::fromMSecsSinceStartOfDay(millisecondsOfDay);
    return QStringLiteral("%1d %2")
        .arg(days)
        .arg(time.toString(QStringLiteral("hh'h' mm'm' ss's'")));
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
