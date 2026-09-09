#include "runtime/luaInterpreter.h"

#include <QStandardItemModel>
#include <QThread>
#include <QTimer>
#include <sol/sol.hpp>

#include <tuple>

#include "globals.h"
#include "api/data.h"
#include "api/ftp.h"
#include "api/http.h"
#include "api/imap.h"
#include "api/io.h"
#include "api/key.h"
#include "api/port.h"
#include "api/modbusAscii.h"
#include "api/modbusRtu.h"
#include "api/modbusTcp.h"
#include "api/mouse.h"
#include "api/mqtt.h"
#include "api/smtp.h"
#include "api/string.h"
#include "api/thread.h"
#include "core/globalManager.h"
#include "document/documentModule.h"
#include "util/luaUtils.h"
#include "util/uniCast.h"

// public
LuaInterpreter::LuaInterpreter(const QVariantMap &luaSession, QObject *parent)
    : QObject(parent),
      m_luaSession(luaSession),
      m_data(new Data(this)),
      m_io(new IO(luaSession.value("threadId").toString(), luaSession.value("mode").toInt() == InterpreterMode::Agent, this)),
      m_key(new Key(this)),
      m_mouse(new Mouse(this)),
      m_mqtt(new Mqtt(this)),
      m_string(new String(this)),
      m_thread(new Thread(this)) {
    // LuaStandard lib
    {
        m_lua.open_libraries();
        connect(m_io, &IO::writeTerminal, this, &LuaInterpreter::writeTerminal, Qt::DirectConnection);
        m_lua.set_function("print", [this](const sol::variadic_args &args) { m_io->print(args); });
        // add workspace to search path
        sol::table package = m_lua["package"];
        const std::string currentPath = package["path"];
        const QString workspacePath = QString("%1/?.lua").arg(m_luaSession["workspaceUrl"].toUrl().toLocalFile());
        const QString newPath = QString("%1;%2").arg(QString::fromStdString(currentPath), workspacePath);
        package["path"] = newPath.toStdString();
    }
    // Data lib (static)
    {
        auto database = m_lua.create_table();
        database.set_function("list", [](const sol::this_state ts) { return Data::databaseList(ts); });
        database.set_function("clear", [] { return Data::databaseClear(); });
        database.set_function("write", [](const std::string &key, const sol::object &value) { Data::databaseWrite(key, value); });
        m_lua["database"] = database;

        auto datatable = m_lua.create_table();
        datatable.set_function("list", [](const sol::this_state ts) { return Data::datatableList(ts); });
        datatable.set_function("clear", [] { return Data::datatableClear(); });
        datatable.set_function("write", [](const std::string &key, const sol::object &value) { Data::datatableWrite(key, value); });
        datatable.set_function("export", [](const sol::optional<std::string> &fileName) { Data::datatableExport(fileName.value_or("")); });
        m_lua["datatable"] = datatable;
    }
    // Ftp lib (instance)
    {
        m_lua.new_usertype<Ftp>(
            "Ftp",
            sol::no_constructor,
            sol::meta_function::garbage_collect, [](Ftp *) {
            },
            "login", &Ftp::login,
            "pwd", &Ftp::pwd,
            "cd", &Ftp::cd,
            "list", &Ftp::list,
            "stat", &Ftp::stat,
            "exists", &Ftp::exists,
            "mkdir", &Ftp::mkdir,
            "rmdir", &Ftp::rmdir,
            "delete", &Ftp::remove,
            "rename", &Ftp::rename,
            "download", &Ftp::download,
            "upload", &Ftp::upload,
            "quit", &Ftp::quit
        );
        auto ftp = m_lua.create_table();
        ftp.set_function("new", [this](const std::string &portName, const sol::optional<int> timeout) {
            auto *obj = new Ftp(this);
            obj->init(portName, timeout.value_or(30000));
            return obj;
        });
        m_lua["Ftp"] = ftp;
    }
    // Http lib (instance)
    {
        m_lua.new_usertype<Http>(
            "Http",
            sol::no_constructor,
            sol::meta_function::garbage_collect, [](Http *) {
            },
            "head", &Http::head,
            "delete", &Http::del,
            "get", &Http::get,
            "patch", &Http::patch,
            "post", &Http::post,
            "put", &Http::put
        );
        auto http = m_lua.create_table();
        http.set_function("new", [this](const std::string &portName, const sol::optional<int> timeout) {
            auto *obj = new Http(this);
            obj->init(portName, timeout.value_or(30000));
            return obj;
        });
        m_lua["Http"] = http;
    }
    // Imap lib (instance)
    {
        m_lua.new_usertype<Imap>(
            "Imap",
            sol::no_constructor,
            sol::meta_function::garbage_collect, [](Imap *) {
            },
            "login", &Imap::login,
            "receive", &Imap::receive,
            "logout", &Imap::logout
        );
        auto imap = m_lua.create_table();
        imap.set_function("new", [this](const std::string &portName, const sol::optional<int> timeout) {
            auto *obj = new Imap(this);
            obj->init(portName, timeout.value_or(1000));
            connect(obj, &Imap::appendLog, this, &LuaInterpreter::appendLog);
            return obj;
        });
        m_lua["Imap"] = imap;
    }
    // IO lib
    {
        auto io = m_lua["io"].get_or_create<sol::table>();
        io.set_function("message", [this](const std::string &text) { m_io->message(text); });
        io.set_function("speak", [](const std::string &text) { IO::speak(text); });
        m_lua["io"] = io;
        connect(m_io, &IO::newMessageDialog, this, [this](const QEventLoop *eventloop, const QString &text) {
            emit newMessageDialog(eventloop, m_luaSession["threadId"].toString(), text);
        });
    }
    // Key lib (static)
    {
        auto key = m_lua.create_table();
        key.set_function("tap", [this](const std::string &key) { m_key->tap(key); });
        key.set_function("type", [](const std::string &text) { Key::type(text); });
        m_lua["key"] = key;
    }
    // ModbusAscii lib (instance)
    {
        m_lua.new_usertype<ModbusAscii>(
            "ModbusAscii",
            sol::no_constructor,
            sol::meta_function::garbage_collect, [](ModbusAscii *) {
            },
            "readCoils", &ModbusAscii::readCoils,
            "readDiscreteInputs", &ModbusAscii::readDiscreteInputs,
            "readHoldingRegisters", &ModbusAscii::readHoldingRegisters,
            "readInputRegisters", &ModbusAscii::readInputRegisters,
            "writeSingleCoil", &ModbusAscii::writeSingleCoil,
            "writeSingleRegister", &ModbusAscii::writeSingleRegister,
            "writeMultipleCoils", &ModbusAscii::writeMultipleCoils,
            "writeMultipleRegisters", &ModbusAscii::writeMultipleRegisters
        );
        auto modbusAscii = m_lua.create_table();
        modbusAscii.set_function("new", [this](const std::string &portName, const int slaveAddr, const sol::optional<int> timeout) {
            auto *obj = new ModbusAscii(this);
            obj->init(portName, slaveAddr, timeout.value_or(1000));
            return obj;
        });
        m_lua["ModbusAscii"] = modbusAscii;
    }
    // ModbusRtu lib (instance)
    {
        m_lua.new_usertype<ModbusRtu>(
            "ModbusRtu",
            sol::no_constructor,
            sol::meta_function::garbage_collect, [](ModbusRtu *) {
            },
            "readCoils", &ModbusRtu::readCoils,
            "readDiscreteInputs", &ModbusRtu::readDiscreteInputs,
            "readHoldingRegisters", &ModbusRtu::readHoldingRegisters,
            "readInputRegisters", &ModbusRtu::readInputRegisters,
            "writeSingleCoil", &ModbusRtu::writeSingleCoil,
            "writeSingleRegister", &ModbusRtu::writeSingleRegister,
            "writeMultipleCoils", &ModbusRtu::writeMultipleCoils,
            "writeMultipleRegisters", &ModbusRtu::writeMultipleRegisters
        );
        auto modbusRtu = m_lua.create_table();
        modbusRtu.set_function("new", [this](const std::string &portName, const int slaveAddr, const sol::optional<int> timeout) {
            auto *obj = new ModbusRtu(this);
            obj->init(portName, slaveAddr, timeout.value_or(1000));
            return obj;
        });
        m_lua["ModbusRtu"] = modbusRtu;
    }
    // ModbusTcp lib (instance)
    {
        m_lua.new_usertype<ModbusTcp>(
            "ModbusTcp",
            sol::no_constructor,
            sol::meta_function::garbage_collect, [](ModbusTcp *) {
            },
            "readCoils", &ModbusTcp::readCoils,
            "readDiscreteInputs", &ModbusTcp::readDiscreteInputs,
            "readHoldingRegisters", &ModbusTcp::readHoldingRegisters,
            "readInputRegisters", &ModbusTcp::readInputRegisters,
            "writeSingleCoil", &ModbusTcp::writeSingleCoil,
            "writeSingleRegister", &ModbusTcp::writeSingleRegister,
            "writeMultipleCoils", &ModbusTcp::writeMultipleCoils,
            "writeMultipleRegisters", &ModbusTcp::writeMultipleRegisters
        );
        auto modbusTcp = m_lua.create_table();
        modbusTcp.set_function("new", [this](const std::string &portName, const int transactionId, const int unitId, const sol::optional<int> timeout) {
            auto *obj = new ModbusTcp(this);
            obj->init(portName, transactionId, unitId, timeout.value_or(1000));
            return obj;
        });
        m_lua["ModbusTcp"] = modbusTcp;
    }
    // Mouse lib
    {
        auto mouse = m_lua.create_table();
        mouse.set_function("position", [] {
            const auto position = Mouse::position();
            return std::make_tuple(position.x(), position.y());
        });
        mouse.set_function("move", [](const int x, const int y) { Mouse::move(x, y); });
        mouse.set_function("down", [this](const sol::optional<std::string> &button) { m_mouse->down(button.value_or("left")); });
        mouse.set_function("up", [this](const sol::optional<std::string> &button) { m_mouse->up(button.value_or("left")); });
        mouse.set_function("click", [this](const int x, const int y, const sol::optional<std::string> &button) { m_mouse->click(x, y, button.value_or("left")); });
        mouse.set_function("doubleClick", [this](const int x, const int y, const sol::optional<std::string> &button) { m_mouse->doubleClick(x, y, button.value_or("left")); });
        mouse.set_function("scroll", [](const int x, const int y, const int steps) { Mouse::scroll(x, y, steps); });
        m_lua["mouse"] = mouse;
    }
    // Mqtt lib (instance)
    {
        m_lua.new_usertype<Mqtt>(
            "Mqtt",
            sol::no_constructor,
            sol::meta_function::garbage_collect, [](Mqtt *) {
            },
            "options", sol::readonly_property(&Mqtt::optionsProxy),
            "connect", &Mqtt::connect
        );
        auto mqtt = m_lua.create_table();
        mqtt.set_function("new", [this](const std::string &portName, const sol::optional<int> timeout) {
            auto *obj = new Mqtt(this);
            obj->init(portName, timeout.value_or(1000));
            return obj;
        });
        m_lua["Mqtt"] = mqtt;
    }
    // Port lib (registry + instance)
    {
        m_lua.new_usertype<Port>(
            "Port",
            sol::no_constructor,
            sol::meta_function::garbage_collect, [](Port *) {
            },
            "info", &Port::info,
            "open", &Port::open,
            "close", &Port::close,
            "clear", &Port::clear,
            "write", [](const Port &port, const std::string &data, const sol::optional<std::string> &peerIp) { port.write(data, peerIp.value_or("")); },
            "read", [](const Port &port, const sol::this_state ts, const sol::optional<int> length, const sol::optional<int> timeout, const sol::optional<std::string> &peerIp) {
                return port.read(ts, length.value_or(0), timeout.value_or(0), peerIp.value_or(""));
            },
            "readUntil", [](const Port &port, const sol::this_state ts, const sol::optional<std::string> &text, const sol::optional<int> timeout,
                            const sol::optional<std::string> &peerIp) {
                return port.readUntil(ts, text.value_or("\r\n"), timeout.value_or(0), peerIp.value_or(""));
            }
        );
        auto port = m_lua.create_table();
        auto portType = m_lua.create_table();
        portType["SerialPort"] = PortType::SerialPort;
        portType["TcpClient"] = PortType::TcpClient;
        portType["TcpServer"] = PortType::TcpServer;
        portType["SslClient"] = PortType::SslClient;
        portType["SslServer"] = PortType::SslServer;
        port["Type"] = portType;
        port.set_function("list", [](const sol::this_state ts) { return Port::list(ts); });
        port.set_function("create", [this](const sol::table &config) { return Port::create(config, this); });
        port.set_function("get", [this](const std::string &portName) { return Port::get(portName, this); });
        port.set_function("remove", [](const std::string &portName) { Port::remove(portName); });
        m_lua["Port"] = port;
    }
    // Smtp lib (instance)
    {
        m_lua.new_usertype<Smtp>(
            "Smtp",
            sol::no_constructor,
            sol::meta_function::garbage_collect, [](Smtp *) {
            },
            "authLogin", &Smtp::authLogin,
            "ehlo", &Smtp::ehlo,
            "send", &Smtp::send,
            "quit", &Smtp::quit
        );
        auto smtp = m_lua.create_table();
        smtp.set_function("new", [this](const std::string &portName, const sol::optional<int> timeout) {
            auto *obj = new Smtp(this);
            obj->init(portName, timeout.value_or(1000));
            return obj;
        });
        m_lua["Smtp"] = smtp;
    }
    // String lib (static)
    {
        auto string = m_lua["string"].get_or_create<sol::table>();
        string.set_function("toBase64", [](const std::string &str) { return String::toBase64(str); });
        string.set_function("fromBase64", [](const std::string &str) { return String::fromBase64(str); });
        string.set_function("toHex", [](const std::string &ba, const sol::optional<char> separator) { return String::toHex(ba, separator.value_or('\0')); });
        string.set_function("fromHex", [](const std::string &str) { return String::fromHex(str); });
        m_lua["string"] = string;
    }
    // Thread lib (static)
    {
        sol::table thread = m_lua.create_table();
        thread.set_function("start", [this](const sol::this_state ts, const std::string &documentPath) { return m_thread->start(ts, documentPath); });
        thread.set_function("stop", [this](const std::string &threadId) { m_thread->stop(threadId); });
        thread.set_function("sleep", [](const int ms) { Thread::sleep(ms); });
        m_lua["thread"] = thread;
        connect(m_thread, &Thread::startThread, this, &LuaInterpreter::startThread);
        connect(m_thread, &Thread::stopThread, this, &LuaInterpreter::stopThread);
    }
}

QJsonObject LuaInterpreter::start(const QString &script) {
    lua_State *L = m_lua.lua_state();
    m_io->redirect(L);
    // load session
    m_lua["session"] = &m_luaSession;
    // set hook
    if (m_luaSession["mode"] == InterpreterMode::Run) {
        // set run hook
        lua_sethook(L, &luaRunHook, LUA_MASKCOUNT, 100);
    } else {
        m_luaSession.insert("this", QVariant::fromValue(this));
        // set debug hook
        lua_sethook(L, &luaDebugHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
    }
    // frontend
    emit deleteMarker(m_luaSession["documentUrl"].toUrl(), ScintillaMarker::Debug, -1);

    const QString filePath = "@" + m_luaSession["documentUrl"].toUrl().toLocalFile();
    const sol::protected_function_result result = m_lua.safe_script(
        script.toStdString(),
        sol::script_pass_on_error,
        filePath.toStdString()
    );
    if (!result.valid()) {
        const sol::error err = result;
        m_io->stdErr(QString::fromStdString(err.what()).toUtf8() + '\n');
    }
    // frontend
    emit deleteMarker(m_luaSession["documentUrl"].toUrl(), ScintillaMarker::Debug, -1);
    // remove terminate hook
    lua_sethook(L, nullptr, 0, 0);
    return m_io->finish();
}

void LuaInterpreter::inputWrite(const QByteArray &data) const {
    m_io->stdIn(data);
}

void LuaInterpreter::stateSet(const int state) {
    m_luaSession["state"] = state;
    if (state == Debug::StepOver || state == Debug::StepOut) {
        m_luaSession["baseDepth"] = m_luaSession["currentDepth"].toInt();
    }
    if (state != Debug::Pause) emit quitLoop();
}

void LuaInterpreter::stackSet(lua_State *L, lua_Debug *ar) {
    sol::state_view lua(L);
    QVariantMap &session = *lua["session"].get<QVariantMap *>();
    auto *This = qvariant_cast<LuaInterpreter *>(session["this"]);
    auto *callStackModel = new QStandardItemModel(); // NOLINT
    int level = 0;
    while (lua_getstack(L, level, ar)) {
        lua_getinfo(L, "nSl", ar);
        const QUrl documentUrl = QUrl::fromLocalFile(QString::fromUtf8(ar->source + 1));
        const int line = ar->currentline;
        const QVariantHash position = {
            {"documentUrl", documentUrl},
            {"line", line}
        };
        auto *fileItem = new QStandardItem(documentUrl.fileName()); // NOLINT
        fileItem->setData(position, Qt::WhatsThisRole);
        auto *lineItem = new QStandardItem(QString::number(line)); // NOLINT
        auto *nameItem = new QStandardItem(ar->name ? ar->name : "main"); // NOLINT
        callStackModel->insertRow(0, {fileItem, lineItem, nameItem});
        level++;
    }
    emit This->insertCallStack(session["threadId"].toString(), callStackModel);
}

void LuaInterpreter::watchSet(lua_State *L, lua_Debug *ar) {
    lua_getstack(L, 0, ar);
    lua_getinfo(L, "S", ar);
    QUrl currentUrl;
    if (ar->source[0] == '@' && ar->source[1] != '\0') {
        currentUrl = QUrl::fromLocalFile(QString::fromUtf8(ar->source + 1));
    }
    sol::state_view lua(L);
    QHash<QString, int> watchHash{};
    for (int i = 0; i < g_watchStandardItemModel->rowCount(); ++i) {
        const QString url = g_watchStandardItemModel->item(i, 0)->data(Qt::WhatsThisRole).toString();
        if (url == currentUrl) {
            const QString key = g_watchStandardItemModel->item(i, 0)->text();
            watchHash.insert(key, i);
        }
    }
    if (!watchHash.isEmpty()) {
        // create env table
        sol::environment env(lua, sol::create);
        sol::table mt = lua.create_table();
        mt[sol::meta_function::index] = lua.globals();
        env[sol::metatable_key] = mt;
        // load upvalues
        if (lua_getinfo(L, "f", ar)) {
            const char *name = nullptr;
            int i = 1;
            while ((name = lua_getupvalue(L, -1, i++)) != nullptr) {
                if (name[0] != '(') env[name] = sol::object(L, -1);
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
        // load locals
        const char *name = nullptr;
        int i = 1;
        while ((name = lua_getlocal(L, ar, i++)) != nullptr) {
            if (name[0] != '(') env[name] = sol::object(L, -1);
            lua_pop(L, 1);
        }
        // eval
        for (auto it = watchHash.constBegin(); it != watchHash.constEnd(); ++it) {
            const auto result = lua.safe_script("return " + it.key().toStdString(), env, sol::script_pass_on_error);
            QString val = "nil";
            QString type = "nil";
            if (result.valid()) {
                sol::object obj = result;
                type = lua_typename(L, static_cast<int>(obj.get_type()));
                val = uni_cast<QString>(obj);
            }
            g_watchStandardItemModel->item(it.value(), 1)->setText(val);
            g_watchStandardItemModel->item(it.value(), 1)->setData(type, Qt::WhatsThisRole);
        }
    }
}

void LuaInterpreter::valueSet(const QString &documentUrl, const QString &expression, const QString &value, const QString &type) {
    emit setValue(documentUrl, expression, value, type);
}

// private
void LuaInterpreter::luaRunHook(lua_State *L, lua_Debug *ar) {
    // check if thread interruption is requested
    if (QThread::currentThread()->isInterruptionRequested()) {
        luaL_error(L, "terminated");
    }
}

void LuaInterpreter::luaDebugHook(lua_State *L, lua_Debug *ar) {
    // handle pause/terminate request
    QThread *thread = QThread::currentThread();
    thread->eventDispatcher()->processEvents(QEventLoop::AllEvents);

    sol::state_view lua(L);
    QVariantMap &session = *lua["session"].get<QVariantMap *>();
    auto *This = qvariant_cast<LuaInterpreter *>(session["this"]);
    emit This->deleteMarker(session["currentUrl"].toUrl(), ScintillaMarker::Debug, -1);
    if (ar->event == LUA_HOOKCALL) {
        session["currentDepth"] = session["currentDepth"].toInt() + 1;
    } else if (ar->event == LUA_HOOKRET) {
        session["currentDepth"] = session["currentDepth"].toInt() - 1;
    } else if (ar->event == LUA_HOOKLINE) {
        // get file info
        lua_getinfo(L, "Sl", ar);
        QUrl currentUrl;
        if (ar->source[0] == '@' && ar->source[1] != '\0') {
            currentUrl = QUrl::fromLocalFile(QString::fromUtf8(ar->source + 1));
        }
        const int currentLine = ar->currentline;
        // watch refresh
        if (g_globalManager->refreshGet()) {
            watchSet(L, ar);
            g_globalManager->refreshSet(false);
        }
        // debug state machine
        if (session["state"].toInt() == Debug::Resume && g_breakpoints.contains(currentUrl.toString())) {
            if (g_breakpoints[currentUrl].contains(currentLine)) {
                // enabled check
                if (!g_breakpoints[currentUrl][currentLine]["enabled"].toBool()) return;
                // conditional breakpoint
                {
                    QString condition = g_breakpoints[currentUrl][currentLine]["condition"].toString();
                    if (condition.isEmpty()) {
                        session["state"] = Debug::Pause;
                    } else {
                        const int base = lua_gettop(L);
                        if (!condition.trimmed().startsWith("return ")) condition = "return " + condition;
                        // create env table
                        sol::environment env(lua, sol::create);
                        sol::table mt = lua.create_table();
                        mt[sol::meta_function::index] = lua.globals();
                        env[sol::metatable_key] = mt;
                        // load upvalues
                        if (lua_getinfo(L, "f", ar)) {
                            const char *name = nullptr;
                            int i = 1;
                            while ((name = lua_getupvalue(L, -1, i++)) != nullptr) {
                                if (name[0] != '(') env[name] = sol::object(L, -1);
                                lua_pop(L, 1);
                            }
                            lua_pop(L, 1);
                        }
                        // load locals
                        const char *name = nullptr;
                        int i = 1;
                        while ((name = lua_getlocal(L, ar, i++)) != nullptr) {
                            if (name[0] != '(') {
                                env[name] = sol::object(L, -1);
                            }
                            lua_pop(L, 1);
                        }
                        // judge condition
                        const sol::protected_function_result condition_result = lua.safe_script(
                            condition.toStdString(),
                            env,
                            sol::script_pass_on_error
                        );
                        if (condition_result.valid()) {
                            const sol::object result = condition_result;
                            if (result.as<bool>()) session["state"] = Debug::Pause;
                        } else {
                            const sol::error err = condition_result;
                            This->m_io->stdErr(QString::fromStdString(err.what()).toUtf8() + '\n');
                        }
                        lua_settop(L, base);
                    }
                }
                // TODO: log/count
            }
        } else if (session["state"].toInt() == Debug::StepOver && session["currentDepth"].toInt() == session["baseDepth"].toInt()) session["state"] = Debug::Pause;
        else if (session["state"].toInt() == Debug::StepOut && session["currentDepth"].toInt() < session["baseDepth"].toInt()) session["state"] = Debug::Pause;
        else if (session["state"].toInt() == Debug::StepInto) session["state"] = Debug::Pause;
        else if (session["state"].toInt() == Debug::RunToCursor && g_cursorPosition["url"].toUrl() == currentUrl && g_cursorPosition["line"].toInt() == currentLine)
            session["state"] = Debug::Pause;
        if (session["state"].toInt() == Debug::Pause) {
            // url handle
            emit This->openDocument(currentUrl);
            if (currentUrl != session["currentUrl"].toUrl()) session["currentUrl"] = currentUrl;
            // line handle
            emit This->addMarker(currentUrl, ScintillaMarker::Debug, currentLine - 1, -1);
            // call stack handle
            stackSet(L, ar);
            // watch handle
            watchSet(L, ar);
            // hold thread
            QEventLoop loop;
            connect(This, &LuaInterpreter::quitLoop, &loop, &QEventLoop::quit);
            connect(This, &LuaInterpreter::setValue, This,
                    [This, L, ar, currentUrl](const QString &documentUrl, const QString &expression, const QString &value, const QString &type) {
                        disconnect(This, &LuaInterpreter::setValue, This, nullptr);
                        if (currentUrl != documentUrl) {
                            This->m_io->stdErr("Hot update failed: Not in the current file scope\n");
                            return;
                        }
                        bool updated = false;
                        // try local value
                        const char *localValue = nullptr;
                        int local_i = 1;
                        while ((localValue = lua_getlocal(L, ar, local_i)) != nullptr) {
                            if (expression == localValue) {
                                lua_pushvariant(L, value, type);
                                lua_setlocal(L, ar, local_i);
                                lua_pop(L, 1);
                                updated = true;
                                emit This->appendLog(LogLevel::Info, QString("Hot update executed: local %1 = %2").arg(expression, value), "");
                                break;
                            }
                            lua_pop(L, 1);
                            local_i++;
                        }
                        // try up value
                        if (!updated) {
                            if (lua_getinfo(L, "f", ar)) {
                                const char *upValue = nullptr;
                                int upvalue_i = 1;
                                while ((upValue = lua_getupvalue(L, -1, upvalue_i)) != nullptr) {
                                    if (expression == upValue) {
                                        lua_pushvariant(L, value, type);
                                        lua_setupvalue(L, -3, upvalue_i);
                                        lua_pop(L, 1);
                                        updated = true;
                                        emit This->appendLog(LogLevel::Info, QString("Hot update executed: upvalue %1 = %2").arg(expression, value), "");
                                        break;
                                    }
                                    lua_pop(L, 1);
                                    upvalue_i++;
                                }
                                lua_pop(L, 1);
                            }
                        }
                        // value not found
                        if (!updated) {
                            This->m_io->stdErr(QString("Hot update failed: variable '%1' not found\n").arg(expression).toUtf8());
                        } else {
                            // watch handle
                            watchSet(L, ar);
                        }
                    });
            loop.exec();
        }
        if (QThread::currentThread()->isInterruptionRequested()) {
            luaL_error(L, "terminated");
        }
    }
}
