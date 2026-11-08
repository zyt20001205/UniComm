#include "api/thread.h"

#include <QCborMap>
#include <QThread>
#include <sol/sol.hpp>
#include <sol/utility/is_integer.hpp>

#include "globals.h"
#include "runtime/threadpoolModule.h"

std::string Thread::start(const sol::object &fn, const QUrl &documentUrl, const int mode) {
    LuaClosure closure(fn);
    QString threadId{};
    QMetaObject::invokeMethod(g_threadpool, [&threadId, &documentUrl, mode, &closure] {
        threadId = g_threadpool->threadStart(documentUrl, mode, &closure);
    }, Qt::BlockingQueuedConnection);
    if (threadId.isEmpty()) throw sol::error("could not start thread");
    return threadId.toStdString();
}

void Thread::stop(const std::string &threadId) {
    QMetaObject::invokeMethod(g_threadpool, [id = QString::fromStdString(threadId)] { g_threadpool->threadStop(id); });
}

void Thread::sleep(const int ms) {
    int elapsed = 0;
    while (elapsed < ms) {
        constexpr int INTERVAL = 100;
        if (QThread::currentThread()->isInterruptionRequested()) return;
        const int sleepTime = qMin(INTERVAL, ms - elapsed);
        QThread::msleep(sleepTime);
        elapsed += sleepTime;
    }
}

LuaClosure::LuaClosure(const sol::object &fn) {
    if (fn.get_type() != sol::type::function) throw sol::error("thread.start expects a Lua function");
    const auto function = fn.as<sol::protected_function>();
    auto *L = function.lua_state();
    const auto frame = sol::stack::push_pop(function);
    const int functionIndex = frame.index_of(function);
    if (lua_iscfunction(L, functionIndex)) throw sol::error("thread.start expects a Lua function");
    // Each active table iterator keeps its table and key on the Lua stack.
    if (!lua_checkstack(L, MAX_DEPTH * 2 + LUA_MINSTACK)) throw sol::error("not enough stack space to copy thread captures");

    m_bytecode = function.dump();
    for (int slot = 1; const char *name = lua_getupvalue(L, functionIndex, slot); ++slot) {
        const auto value = sol::stack::pop<sol::object>(L);
        const sol::string_view captureName(name);
        if (captureName == "(no name)") throw sol::error("thread.start cannot copy unnamed captures");
        if (captureName == "_ENV") {
            const auto globals = sol::state_view(L).globals();
            const auto globalFrame = sol::stack::push_pop(globals);
            const auto captureFrame = sol::stack::push_pop(value);
            if (!lua_rawequal(L, -1, -2)) throw sol::error("thread.start cannot copy a custom environment");
            m_envSlot = slot;
            m_captures.append(QCborValue::Null);
            continue;
        }
        m_captures.append(snapshot(value, name));
    }
}

std::optional<std::string> LuaClosure::call(sol::state_view lua) const {
    const auto loaded = lua.load_buffer(m_bytecode.data(), m_bytecode.size(), "=(thread)", sol::load_mode::binary);
    if (!loaded.valid()) return loaded.get<sol::error>().what();
    // Loading sets the first upvalue to globals even when it is not _ENV.
    int slot = 0;
    for (const auto &value: m_captures) {
        if (++slot == m_envSlot) lua.globals().push();
        else restore(lua, value).push();
        lua_setupvalue(lua.lua_state(), loaded.stack_index(), slot);
    }

    const auto result = loaded.get<sol::protected_function>()();
    if (!result.valid()) return result.get<sol::error>().what();
    return std::nullopt;
}

QCborValue LuaClosure::snapshot(const sol::object &value, const std::string &label, const int depth) {
    const auto type = value.get_type();
    switch (type) {
        case sol::type::nil:
            return QCborValue::Null;
        case sol::type::boolean:
            return value.as<bool>();
        case sol::type::number:
            if (sol::utility::is_integer(value)) return value.as<lua_Integer>();
            return value.as<double>();
        case sol::type::string: {
            const auto bytes = value.as<std::string>();
            return QByteArray::fromStdString(bytes);
        }
        case sol::type::table: {
            if (depth == MAX_DEPTH) throw sol::error(label + " contains a cyclic or too deeply nested table");
            const auto table = value.as<sol::table>();
            if (table.get<sol::object>(sol::metatable_key).valid()) throw sol::error(label + " contains a table with a metatable");
            QCborMap result{};
            for (const auto &[key, element]: table) {
                if (key.get_type() != sol::type::string && !sol::utility::is_integer(key)) throw sol::error(label + " must use string or integer table keys");
                result.insert(snapshot(key, label, depth + 1), snapshot(element, label, depth + 1));
            }
            return result;
        }
        default:
            throw sol::error(label + " cannot copy Lua " + lua_typename(value.lua_state(), static_cast<int>(type)));
    }
}

sol::object LuaClosure::restore(sol::state_view lua, const QCborValue &value) {
    switch (value.type()) {
        case QCborValue::Null:
            return sol::make_object(lua, sol::nil);
        case QCborValue::False:
        case QCborValue::True:
            return sol::make_object(lua, value.toBool());
        case QCborValue::Integer:
            return sol::make_object(lua, value.toInteger());
        case QCborValue::Double:
            return sol::make_object(lua, value.toDouble());
        case QCborValue::ByteArray:
            return sol::make_object(lua, value.toByteArray().toStdString());
        case QCborValue::Map: {
            const auto map = value.toMap();
            auto table = lua.create_table();
            for (auto it = map.cbegin(); it != map.cend(); ++it) table.raw_set(restore(lua, it.key()), restore(lua, it.value()));
            return sol::make_object(lua, table);
        }
        default:
            throw sol::error("invalid thread snapshot");
    }
}
