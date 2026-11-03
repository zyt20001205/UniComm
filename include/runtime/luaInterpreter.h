#ifndef UNICOMM_LUAINTERPRETER_H
#define UNICOMM_LUAINTERPRETER_H

#include <QMap>
#include <QObject>
#include <QUrl>
#include <sol/state.hpp>

class QEventLoop;
class QJsonObject;
class QStandardItemModel;
class Data;
class IO;
class Key;
class Mouse;
class String;
class Thread;

class LuaInterpreter final : public QObject {
    Q_OBJECT

public:
    explicit LuaInterpreter(const QVariantMap &luaSession , QObject *parent = nullptr);

    [[nodiscard]] QJsonObject start(const QString &script);

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

    void newMessageDialog(const QEventLoop *eventloop, const QString &threadId, const QString &text) const;

    void startThread(const QUrl &documentUrl, int mode, QString &threadId, int startLine, int startCharacter, int endLine, int endCharacter);

    void stopThread(const QString &threadId);

    void setValue(const QString &documentUrl, const QString &expression, const QString &value, const QString &type);

    void quitLoop();

private:
    static void luaRunHook(lua_State *L, lua_Debug *ar);

    static void luaDebugHook(lua_State *L, lua_Debug *ar);

    sol::state m_lua{};
    QVariantMap m_luaSession{};
    Data *m_data{};
    IO *m_io{};
    Key *m_key{};
    Mouse *m_mouse{};
    String *m_string{};
    Thread *m_thread{};
};

#endif //UNICOMM_LUAINTERPRETER_H
