#ifndef UNICOMM_THREAD_H
#define UNICOMM_THREAD_H

#include <QCborArray>
#include <sol/bytecode.hpp>
#include <sol/forward.hpp>

#include <string>

class QUrl;

class Thread final {
public:
    [[nodiscard]] static std::string start(const sol::object &fn, const QUrl &documentUrl, int mode);

    static void stop(const std::string &threadId);

    static void sleep(int ms);
};

class LuaClosure final {
public:
    explicit LuaClosure(const sol::object &fn);

    [[nodiscard]] sol::protected_function_result call(sol::state_view &lua, const std::string &chunkName) const;

private:
    [[nodiscard]] static QCborValue snapshot(const sol::object &value, const std::string &label, int depth = 0);

    [[nodiscard]] static sol::object restore(sol::state_view lua, const QCborValue &value);

    static constexpr int MAX_DEPTH = 100;
    sol::bytecode m_bytecode{};
    QCborArray m_captures{};
    int m_envSlot = 0;
};

#endif //UNICOMM_THREAD_H
