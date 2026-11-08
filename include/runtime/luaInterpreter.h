#ifndef UNICOMM_LUAINTERPRETER_H
#define UNICOMM_LUAINTERPRETER_H

#include <sol/state.hpp>
#include <variant>

#include "api/thread.h"

class QJsonObject;
class QStandardItemModel;
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

    void stop() const;

    void inputWrite(const QByteArray &data) const;

    void stateSet(int state);

    static void stackSet(lua_State *L, lua_Debug *ar);

    static void watchSet(lua_State *L, lua_Debug *ar);

    void valueSet(const QString &documentUrl, const QString &expression, const QString &value, const QString &type);

signals:
    void writeTerminal(const QString &id, const QByteArray &data);

    void openDocument(const QUrl &documentUrl);

    void addMarker(const QUrl &documentUrl, int type, int line, int time);

    void deleteMarker(const QUrl &documentUrl, int type, int line);

    void insertCallStack(const QString &threadId, QStandardItemModel *callStackModel);

    void appendLog(int type, const QString &prefix, const QString &message);

    void setValue(const QString &documentUrl, const QString &expression, const QString &value, const QString &type);

    void quitLoop();

private:
    static void luaRunHook(lua_State *L, lua_Debug *ar);

    static void luaDebugHook(lua_State *L, lua_Debug *ar);

    sol::state m_lua{};
    QVariantMap m_luaSession{};
    IO *m_io{};
    Key *m_key{};
    Mouse *m_mouse{};
    String *m_string{};
    Ui *m_ui{};
};

#endif //UNICOMM_LUAINTERPRETER_H
