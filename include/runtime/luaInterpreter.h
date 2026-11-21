#ifndef UNICOMM_LUAINTERPRETER_H
#define UNICOMM_LUAINTERPRETER_H

#include <sol/state.hpp>
#include <variant>

#include "api/thread.h"
#include "types/logTypes.h"
#include "types/runtimeTypes.h"

class QJsonObject;
class IO;
class Key;
class Mouse;
class String;
class Ui;

class LuaInterpreter final : public QObject {
    Q_OBJECT

public:
    explicit LuaInterpreter(const QVariantMap &luaSession , QObject *parent = nullptr);

    [[nodiscard]] QJsonObject start(const std::variant<QString, LuaClosure> &entry);

    void stop();

    void inputWrite(const QByteArray &data) const;

    void interpreterStateSet(InterpreterState::Value state);

    void watchExpressionsSet(const QHash<QUrl, QVariantHash> &expressions);

    void watchValuesRefresh();

signals:
    void appendLog(LogType::Value type, const QString &prefix, const QString &message);

    void writeTerminal(const QString &id, const QByteArray &data);

    void setCallStack(const QString &threadId, const QVariantList &frames);

    void refreshWatchValues();

    void setWatchValues(const QString &threadId, const QUrl &documentUrl, const QVariantHash &results);

    void setWatchValue(const QString &expression, const QString &value, LuaType::Value type);

    void quitLoop();

private:
    static void callStackSet(lua_State *L, lua_Debug *ar);

    [[nodiscard]] static QVariantHash watchValueGet(const sol::object &object, const QString &expression, const QSet<QString> &open, WatchSource::Value source = WatchSource::Expression);

    [[nodiscard]] QVariantHash watchValuesGet(lua_State *L, lua_Debug *ar, const QUrl &documentUrl) const;

    void watchValueSet(lua_State *L, lua_Debug *ar, const QString &expression, const QString &value, LuaType::Value type) const;

    static void luaRunHook(lua_State *L, lua_Debug *ar);

    static void luaDebugHook(lua_State *L, lua_Debug *ar);

    sol::state m_lua{};
    QVariantMap m_luaSession{};
    bool m_watchValuesRefresh{};
    IO *m_io{};
    Key *m_key{};
    Mouse *m_mouse{};
    String *m_string{};
    Ui *m_ui{};
};

#endif //UNICOMM_LUAINTERPRETER_H
