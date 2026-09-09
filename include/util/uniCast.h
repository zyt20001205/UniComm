#ifndef UNICOMM_UNICAST_H
#define UNICOMM_UNICAST_H

#include <QColor>
#include <QJsonObject>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <sol/object.hpp>
#include <vterm.h>
#include <vterm_keycodes.h>

template<typename D, typename S>
[[nodiscard]] D uni_cast(const S& s, int depth = 0);

template<typename D, typename S>
[[nodiscard]] D uni_cast(sol::this_state ts, const S& s, int depth = 0);

template<typename D, typename S>
[[nodiscard]] D uni_cast(const VTermScreen *vts, const S& s, int depth = 0);

// lua -> qt
struct LUrl {
    QString value;
    LUrl(QString s) : value(std::move(s)) {}
    operator QString() const { return value; }
};

struct LPath {
    QString value;
    LPath(QString s) : value(std::move(s)) {}
    operator QString() const { return value; }
};

template<>
[[nodiscard]] QUrl uni_cast<QUrl, LUrl>(const LUrl &s, int depth);

template<>
[[nodiscard]] QUrl uni_cast<QUrl, LPath>(const LPath &s, int depth);

// sol -> qt
template<>
[[nodiscard]] QString uni_cast<QString, sol::object>(const sol::object &s, int depth);

template<>
[[nodiscard]] QVariant uni_cast<QVariant, sol::object>(const sol::object &s, int depth);

template<>
[[nodiscard]] QJsonObject uni_cast<QJsonObject, sol::table>(const sol::table &s, int depth);

template<>
[[nodiscard]] QVariantList uni_cast<QVariantList, sol::variadic_args>(const sol::variadic_args &s, int depth);

// qt -> sol
template<>
[[nodiscard]] sol::object uni_cast<sol::object, QVariant>(sol::this_state ts, const QVariant &s, int depth);

template<>
[[nodiscard]] sol::object uni_cast<sol::object, QVariantMap>(sol::this_state ts, const QVariantMap &s, int depth);

template<>
[[nodiscard]] sol::object uni_cast<sol::object, QVariantList>(sol::this_state ts, const QVariantList &s, int depth);

template<>
[[nodiscard]] sol::object uni_cast<sol::object, QVariantHash>(sol::this_state ts, const QVariantHash &s, int depth);

template<>
[[nodiscard]] sol::table uni_cast<sol::table, QSet<QString>>(sol::this_state ts, const QSet<QString> &s, int depth);

template<>
[[nodiscard]] sol::table uni_cast<sol::table, QList<QVariant>>(sol::this_state ts, const QList<QVariant> &s, int depth);

// qt -> qt
struct MarkdownTable {
    struct Alignment {
        enum {
            Left = Qt::AlignLeft,
            Center = Qt::AlignHCenter,
            Right = Qt::AlignRight
        };
    };

    QStringList header{};
    QList<QStringList> rows{};
    QList<int> alignments{};

    bool operator==(const MarkdownTable &) const = default;
};

struct MarkdownBlock {
    struct Type {
        enum {
            Markdown,
            Code,
            Table
        };
    };

    int type{};
    QString content{};
    QString language{};
    MarkdownTable table{};
};

struct QFileIcon {
    QUrl value;
    QFileIcon(QUrl s) : value(std::move(s)) {}
    operator QUrl() const { return value; }
};

struct QHtmlString {
    QString value;
    QHtmlString(QString s) : value(std::move(s)) {}
    operator QString() const { return value; }
};

struct QFullHtmlString {
    QString value;
    QFullHtmlString(QString s) : value(std::move(s)) {}
    operator QString() const { return value; }
};

struct QLifetime {
    QString value;
    QLifetime(QString s) : value(std::move(s)) {}
    operator QString() const { return value; }
};

struct QDuration {
    QString value;
    QDuration(QString s) : value(std::move(s)) {}
    operator QString() const { return value; }
};

struct QCompactNumber {
    QString value;
    QCompactNumber(QString s) : value(std::move(s)) {}
    operator QString() const { return value; }
};

template<>
[[nodiscard]] QList<MarkdownBlock> uni_cast<QList<MarkdownBlock>, QString>(const QString &s, int depth);

template<>
[[nodiscard]] QFileIcon uni_cast<QFileIcon, QUrl>(const QUrl &s, int depth);

template<>
[[nodiscard]] QIcon uni_cast<QIcon, QUrl>(const QUrl &s, int depth);

template<>
[[nodiscard]] QHtmlString uni_cast<QHtmlString, QString>(const QString &s, int depth);

template<>
[[nodiscard]] QFullHtmlString uni_cast<QFullHtmlString, QString>(const QString &s, int depth);

template<>
[[nodiscard]] QLifetime uni_cast<QLifetime, qint64>(const qint64 &s, int depth);

template<>
[[nodiscard]] QDuration uni_cast<QDuration, qint64>(const qint64 &s, int depth);

template<>
[[nodiscard]] QCompactNumber uni_cast<QCompactNumber, qint64>(const qint64 &s, int depth);

// qt -> suffix
struct ModbusCRC {
    QByteArray value;
    ModbusCRC(QByteArray s) : value(std::move(s)) {}
    operator QByteArray() const { return value; }
};

template<>
[[nodiscard]] ModbusCRC uni_cast<ModbusCRC, QByteArray>(const QByteArray &s, int depth);

struct ModbusLRC {
    QByteArray value;
    ModbusLRC(QByteArray s) : value(std::move(s)) {}
    operator QByteArray() const { return value; }
};

template<>
[[nodiscard]] ModbusLRC uni_cast<ModbusLRC, QByteArray>(const QByteArray &s, int depth);

// vterm -> qt
struct TerminalCell;

template<>
[[nodiscard]] QColor uni_cast<QColor, VTermColor>(const VTermScreen *vts, const VTermColor &s, int depth);

template<>
[[nodiscard]] TerminalCell uni_cast<TerminalCell, VTermScreenCell>(const VTermScreen *vts, const VTermScreenCell &s, int depth);

// qt-> vterm
struct VTermButton {
    int value;
    VTermButton(int s) : value(s) {}
    operator int() const { return value; }
};

template<>
[[nodiscard]] VTermColor uni_cast<VTermColor, QColor>(const QColor &s, int depth);

template<>
[[nodiscard]] VTermButton uni_cast<VTermButton, int>(const int &s, int depth);

template<>
[[nodiscard]] VTermKey uni_cast<VTermKey, int>(const int &s, int depth);

template<>
[[nodiscard]] VTermModifier uni_cast<VTermModifier, int>(const int &s, int depth);

#endif //UNICOMM_UNICAST_H
