#include "runtime/luaInterpreter.h"

#include <QDebug>
#include <QThread>
#include <QTimer>
#include <sol/sol.hpp>

#include <tuple>

#include "globals.h"
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
#include "api/ui/database.h"
#include "api/ui/image.h"
#include "api/ui/plot.h"
#include "api/ui/table.h"
#include "api/ui/ui.h"
#include "core/globalManager.h"
#include "debug/breakpointModule.h"
#include "util/uniCast.h"

// public
LuaInterpreter::LuaInterpreter(const QVariantMap &luaSession, QObject *parent)
    : QObject(parent),
      m_luaSession(luaSession),
      m_io(new IO(luaSession.value("threadId").toString(), this)),
      m_key(new Key(this)),
      m_mouse(new Mouse(this)),
      m_string(new String(this)),
      m_ui(new Ui(this)) {
    // LuaStandard lib
    {
        m_lua.open_libraries();
        connect(m_io, &IO::writeTerminal, this, &LuaInterpreter::writeTerminal, Qt::DirectConnection);
        m_lua.set_function("print", [this](const sol::variadic_args &args) { m_io->print(args); });
        // add workspace to search path
        sol::table package = m_lua["package"];
        const std::string currentPath = package["path"];
        const QString workspacePath = QString("%1/?.lua;%1/?/init.lua").arg(m_luaSession["workspaceUrl"].toUrl().toLocalFile());
        const QString newPath = QString("%1;%2").arg(QString::fromStdString(currentPath), workspacePath);
        package["path"] = newPath.toStdString();
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
            "request", &Http::request,
            "stream", &Http::stream
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
        io.set_function("speak", [](const std::string &text) { IO::speak(text); });
        m_lua["io"] = io;
    }
    // Key lib
    {
        auto key = m_lua.create_table();
        key.set_function("down", [this](const std::string &name) { m_key->down(name); });
        key.set_function("up", [this](const std::string &name) { m_key->up(name); });
        key.set_function("tap", [this](const std::string &name) { m_key->tap(name); });
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
            "connect", &Mqtt::connect,
            "subscribe", &Mqtt::subscribe,
            "publish", &Mqtt::publish,
            "disconnect", &Mqtt::disconnect,
            "consume", &Mqtt::consume
        );
        auto mqtt = m_lua.create_table();
        mqtt.set_function("new", [this](const std::string &portName, const sol::optional<int> timeout) {
            auto *obj = new Mqtt(this);
            obj->init(portName, timeout.value_or(30000));
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
            "write", sol::overload(
                [](const Port &port, const std::string &data, const std::string &serviceUuid, const std::string &characteristicUuid, const std::string &descriptorUuid) {
                    port.write(data, serviceUuid, characteristicUuid, descriptorUuid);
                },
                [](const Port &port, const std::string &data, const int mode, const std::string &serviceUuid, const std::string &characteristicUuid) {
                    port.write(data, mode, serviceUuid, characteristicUuid);
                },
                // References prevent this sol2 version from detecting optional arity.
                [](const Port &port, const std::string &data, const int messageType, const sol::optional<std::string> peerIp) {
                    port.write(data, messageType, peerIp.value_or(""));
                },
                [](const Port &port, const std::string &data, const sol::optional<std::string> peerIp) { port.write(data, WebSocketMessageType::Binary, peerIp.value_or("")); }
            ),
            "read", sol::overload(
                [](const Port &port, const sol::this_state ts, const std::string &serviceUuid, const std::string &characteristicUuid,
                   const sol::optional<std::string> descriptorUuid) {
                    return port.read(ts, serviceUuid, characteristicUuid, descriptorUuid ? std::make_optional(*descriptorUuid) : std::nullopt);
                },
                [](const Port &port, const sol::this_state ts, const int length, const int timeout, const std::string &serviceUuid, const std::string &characteristicUuid) {
                    return port.read(ts, length, timeout, serviceUuid, characteristicUuid);
                },
                [](const Port &port, const sol::this_state ts, const sol::optional<int> length, const sol::optional<int> timeout, const sol::optional<std::string> peerIp) {
                    return port.read(ts, length.value_or(0), timeout.value_or(0), peerIp.value_or(""), false);
                }
            ),
            "peek", sol::overload(
                [](const Port &port, const sol::this_state ts, const int length, const int timeout, const std::string &serviceUuid, const std::string &characteristicUuid) {
                    return port.read(ts, length, timeout, serviceUuid, characteristicUuid, true);
                },
                [](const Port &port, const sol::this_state ts, const sol::optional<int> length, const sol::optional<int> timeout, const sol::optional<std::string> peerIp) {
                    return port.read(ts, length.value_or(0), timeout.value_or(0), peerIp.value_or(""), true);
                }
            ),
            "readUntil", sol::overload(
                [](const Port &port, const sol::this_state ts, const std::string &text, const int timeout, const std::string &serviceUuid, const std::string &characteristicUuid) {
                    return port.readUntil(ts, text, timeout, serviceUuid, characteristicUuid);
                },
                [](const Port &port, const sol::this_state ts, const sol::optional<std::string> text, const sol::optional<int> timeout, const sol::optional<std::string> peerIp) {
                    return port.readUntil(ts, text.value_or("\r\n"), timeout.value_or(0), peerIp.value_or(""), false);
                }
            ),
            "peekUntil", sol::overload(
                [](const Port &port, const sol::this_state ts, const std::string &text, const int timeout, const std::string &serviceUuid, const std::string &characteristicUuid) {
                    return port.readUntil(ts, text, timeout, serviceUuid, characteristicUuid, true);
                },
                [](const Port &port, const sol::this_state ts, const sol::optional<std::string> text, const sol::optional<int> timeout, const sol::optional<std::string> peerIp) {
                    return port.readUntil(ts, text.value_or("\r\n"), timeout.value_or(0), peerIp.value_or(""), true);
                }
            ),
            "subscribe", &Port::subscribe,
            "unsubscribe", &Port::unsubscribe
        );
        auto port = m_lua.create_table();
        auto portType = m_lua.create_table();
        portType["SerialPort"] = PortType::SerialPort;
        portType["TcpClient"] = PortType::TcpClient;
        portType["TcpServer"] = PortType::TcpServer;
        portType["SslClient"] = PortType::SslClient;
        portType["SslServer"] = PortType::SslServer;
        portType["WebSocketClient"] = PortType::WebSocketClient;
        portType["WebSocketServer"] = PortType::WebSocketServer;
        portType["UdpSocket"] = PortType::UdpSocket;
        portType["BluetoothLe"] = PortType::BluetoothLe;
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
        string.set_function("toJson", [](const sol::table &value) { return String::toJson(value); });
        string.set_function("fromJson", [](const sol::this_state ts, const std::string &str) { return String::fromJson(ts, str); });
        string.set_function("hash", [](const std::string &data, const std::string &algorithm) { return String::hash(data, algorithm); });
        string.set_function("encrypt", [](const std::string &data, const std::string &key, const std::string &iv, const std::string &algorithm) {
            return String::encrypt(data, key, iv, algorithm);
        });
        string.set_function("decrypt", [](const std::string &data, const std::string &key, const std::string &iv, const std::string &algorithm) {
            return String::decrypt(data, key, iv, algorithm);
        });
        m_lua["string"] = string;
    }
    // Thread lib (static)
    {
        sol::table thread = m_lua.create_table();
        thread.set_function("start", [this](const sol::object &fn) {
            return Thread::start(fn, m_luaSession.value("documentUrl").toUrl(), m_luaSession.value("mode").value<RunMode::Value>());
        });
        thread.set_function("stop", [](const std::string &threadId) { Thread::stop(threadId); });
        thread.set_function("sleep", [](const int ms) { Thread::sleep(ms); });
        m_lua["thread"] = thread;
    }
    // UI lib
    {
        auto ui = m_lua.create_table();
        ui.new_usertype<UiImage>(
            "Image",
            sol::no_constructor,
            "__close", [](UiImage &viewer, const sol::object &) { viewer.close(); },
            "setData", &UiImage::setData,
            "setPath", &UiImage::setPath,
            "close", &UiImage::close
        );
        ui.new_usertype<UiDatabase>(
            "Database",
            sol::no_constructor,
            "__close", [](UiDatabase &database, const sol::object &) { database.close(); },
            "set", &UiDatabase::set,
            "close", &UiDatabase::close
        );
        ui.new_usertype<UiTable>(
            "Table",
            sol::no_constructor,
            "__close", [](UiTable &table, const sol::object &) { table.close(); },
            "append", &UiTable::append,
            "export", &UiTable::_export,
            "close", &UiTable::close
        );
        ui.new_usertype<UiPlot>(
            "Plot",
            sol::no_constructor,
            "__close", [](UiPlot &plot, const sol::object &) { plot.close(); },
            "append", &UiPlot::append,
            "close", &UiPlot::close
        );
        ui.set_function("message", [this](const sol::optional<std::string> &title, const std::string &text, const sol::optional<sol::table> &button) {
            return m_ui->message(title, text, button);
        });
        ui.set_function("input", [this](const sol::optional<std::string> &title, const std::string &text, const sol::optional<std::string> &value) {
            return m_ui->input(title, text, value);
        });
        ui.set_function("file", [this](const sol::optional<std::string> &title, const sol::optional<std::string> &mode) {
            return m_ui->file(title, mode);
        });
        ui.set_function("image", [this](const sol::optional<std::string> &title, const sol::optional<std::string> &source) {
            return m_ui->image(title, source);
        });
        ui.set_function("database", [this](const sol::optional<std::string> &title, const sol::table &keys) {
            return m_ui->database(title, keys);
        });
        ui.set_function("table", [this](const sol::optional<std::string> &title, const sol::table &keys) {
            return m_ui->table(title, keys);
        });
        ui.set_function("plot", [this](const sol::optional<std::string> &title, const sol::table &keys) {
            return m_ui->plot(title, keys);
        });
        m_lua["ui"] = ui;
    }
}

QJsonObject LuaInterpreter::start(const std::variant<QString, LuaClosure> &entry) {
    lua_State *L = m_lua.lua_state();
    m_io->redirect(L);
    // load session
    m_lua["session"] = &m_luaSession;
    // set hook
    if (m_luaSession.value("mode").value<RunMode::Value>() == RunMode::Run) {
        // set run hook
        lua_sethook(L, &luaRunHook, LUA_MASKCOUNT, 100);
    } else {
        m_luaSession.insert("this", QVariant::fromValue(this));
        // set debug hook
        lua_sethook(L, &luaDebugHook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
    }

    sol::protected_function_result result{};
    if (const auto *script = std::get_if<QString>(&entry)) {
        const auto chunkName = ("@" + m_luaSession.value("documentUrl").toUrl().toLocalFile()).toStdString();
        result = m_lua.safe_script(script->toStdString(), sol::script_pass_on_error, chunkName);
    } else {
        const auto chunkName = ("=" + m_luaSession.value("threadId").toString()).toStdString();
        result = std::get<LuaClosure>(entry).call(m_lua, chunkName);
    }
    const bool valid = result.valid();
    if (valid) m_ui->wait();
    emit m_ui->exit();
    if (!valid) {
        const sol::error err = result;
        m_io->stdErr(QString::fromStdString(err.what()).toUtf8() + '\n');
    }
    // remove terminate hook
    lua_sethook(L, nullptr, 0, 0);
    return m_io->finish();
}

void LuaInterpreter::stop() {
    emit quitLoop();
    emit m_ui->exit();
}

void LuaInterpreter::inputWrite(const QByteArray &data) const {
    m_io->stdIn(data);
}

void LuaInterpreter::interpreterStateSet(const InterpreterState::Value state) {
    m_luaSession["state"] = QVariant::fromValue(state);
    if (state == InterpreterState::StepOver || state == InterpreterState::StepOut) {
        m_luaSession["baseDepth"] = m_luaSession["currentDepth"].toInt();
    }
    if (state != InterpreterState::Pause) emit quitLoop();
}

void LuaInterpreter::watchExpressionsSet(const QHash<QUrl, QVariantHash> &expressions) {
    m_luaSession.insert("expressions", QVariant::fromValue(expressions));
}

void LuaInterpreter::watchValuesRefresh() {
    m_watchValuesRefresh = true;
    emit refreshWatchValues();
}

// private
void LuaInterpreter::callStackSet(lua_State *L, lua_Debug *ar) {
    sol::state_view lua(L);
    QVariantMap &session = *lua["session"].get<QVariantMap *>();
    auto *This = qvariant_cast<LuaInterpreter *>(session["this"]);
    QVariantList frames{};
    int level = 0;
    while (lua_getstack(L, level, ar)) {
        lua_getinfo(L, "nSl", ar);
        const QUrl documentUrl = ar->source[0] == '@' ? QUrl::fromLocalFile(QString::fromUtf8(ar->source + 1)) : QUrl{};
        frames.prepend(QVariantHash{
            {"kind", QString::fromUtf8(ar->what)},
            {"documentUrl", documentUrl},
            {"name", documentUrl.isEmpty() ? QString::fromUtf8(ar->short_src) : documentUrl.fileName()},
            {"line", ar->currentline > 0 ? ar->currentline - 1 : -1},
            {"functionName", QString::fromUtf8(ar->name ? ar->name : "")}
        });
        level++;
    }
    emit This->setCallStack(session["threadId"].toString(), frames);
}

QVariantHash LuaInterpreter::watchValueGet(const sol::object &object, const QString &expression, const QSet<QString> &open) {
    const auto type = object.valid() ? static_cast<LuaType::Value>(object.get_type()) : LuaType::Nil;
    QVariantHash result{{"type", QVariant::fromValue(type)}, {"value", object.valid() ? uni_cast<QString>(object) : QStringLiteral("nil")}};
    if (type != LuaType::Table || expression.isEmpty() || !open.contains(expression)) return result;

    // only visit children of expanded tables
    QVariantMap children{};
    // get a reference to string.format
    const sol::function format = sol::state_view(object.lua_state())["string"]["format"];
    for (const auto &[key, value]: object.as<sol::table>()) {
        const auto keyType = key.get_type();
        // build the full expression
        QString literal{};
        if (keyType == sol::type::string || keyType == sol::type::number || keyType == sol::type::boolean) {
            const auto quoted = QByteArray::fromStdString(format.call<std::string>("%q", key));
            if (quoted.isValidUtf8()) literal = QString::fromUtf8(quoted);
            else {
                // preserve binary keys across the QString round trip
                const auto bytes = QByteArray::fromStdString(key.as<std::string>());
                literal = QString::fromLatin1("\"\\x" + bytes.toHex(':').replace(":", "\\x") + '"');
            }
        }
        const auto path = literal.isEmpty() ? QString{} : expression + "[" + literal + "]";
        // format the display name
        QString display;
        if (literal.isEmpty()) {
            display = QString("[%1:0x%2]").arg(uni_cast<QString>(key)).arg(reinterpret_cast<quintptr>(key.pointer()), 0, 16);
        } else if (keyType == sol::type::string) {
            display = QString::fromStdString(key.as<std::string>());
            if (display.isEmpty()) display = literal;
        } else {
            display = "[" + QString::fromStdString(format.call<std::string>("%s", key)) + "]";
        }
        auto child = watchValueGet(value, path, open);
        child.insert("display", display);
        child.insert("expression", path);
        children.insert(path.isEmpty() ? display : path, child);
    }
    result.insert("children", children);
    return result;
}

QVariantHash LuaInterpreter::watchValuesGet(lua_State *L, lua_Debug *ar, const QUrl &documentUrl) const {
    const auto expressions = m_luaSession.value("expressions").value<QHash<QUrl, QVariantHash>>().value(documentUrl);
    if (expressions.isEmpty()) return {};
    sol::state_view lua(L);
    // load globals
    sol::environment env(lua, sol::create);
    for (const auto &[key, value]: lua.globals()) env[key] = value;
    // load upvalues
    if (lua_getinfo(L, "f", ar)) {
        for (int i = 1; const char *name = lua_getupvalue(L, -1, i); ++i) {
            if (name[0] != '(') env[name] = sol::object(L, -1);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    // load locals
    for (int i = 1; const char *name = lua_getlocal(L, ar, i); ++i) {
        if (name[0] != '(') env[name] = sol::object(L, -1);
        lua_pop(L, 1);
    }
    // evaluate expressions
    QVariantHash results{};
    for (const auto &[expression, expanded]: expressions.asKeyValueRange()) {
        sol::object object{};
        const auto result = lua.safe_script("return " + expression.toStdString(), env, sol::script_pass_on_error);
        if (result.valid()) object = result.get<sol::object>();
        results.insert(expression, watchValueGet(object, expression, expanded.value<QSet<QString>>()));
    }
    return results;
}

void LuaInterpreter::watchValueSet(lua_State *L, lua_Debug *ar, const QString &expression, const QString &value, const LuaType::Value type) const {
    // push new value
    bool valid = true;
    if (type == LuaType::Nil) {
        lua_pushnil(L);
    } else if (type == LuaType::Boolean) {
        valid = value == "true" || value == "false";
        if (valid) lua_pushboolean(L, value == "true");
    } else if (type == LuaType::Number) {
        const auto integer = value.toLongLong(&valid);
        if (valid) {
            lua_pushinteger(L, integer);
        } else {
            const auto number = value.toDouble(&valid);
            if (valid) lua_pushnumber(L, number);
        }
    } else if (type == LuaType::String) {
        const auto bytes = value.toUtf8();
        lua_pushlstring(L, bytes.constData(), bytes.size());
    } else {
        m_io->stdErr(QString("\x1b[33mHot update failed\x1b[39m for %1: unsupported value type\n").arg(expression).toUtf8());
        return;
    }
    if (!valid) {
        const auto expected = type == LuaType::Number ? "a number" : "a boolean";
        m_io->stdErr(QString("\x1b[33mHot update failed\x1b[39m for %1: expected %2, got %3\n").arg(expression, expected, QDebug::toString(value)).toUtf8());
        return;
    }
    // collect frame variables for Lua assignments
    sol::state_view lua(L);
    sol::environment env(lua, sol::create);
    int upvalue = 0;
    if (lua_getinfo(L, "f", ar)) {
        for (int i = 1; const char *name = lua_getupvalue(L, -1, i); ++i) {
            env[name] = sol::object(L, -1);
            if (expression == QString::fromUtf8(name)) upvalue = i;
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    int local = 0;
    for (int i = 1; const char *name = lua_getlocal(L, ar, i); ++i) {
        env[name] = sol::object(L, -1);
        if (expression == QString::fromUtf8(name)) local = i;
        lua_pop(L, 1);
    }
    const auto display = type == LuaType::String ? QDebug::toString(value) : value;
    if (local) {
        lua_setlocal(L, ar, local);
        m_io->stdOut(QString("\x1b[32mHot updated\x1b[39m: local %1 = %2\n").arg(expression, display).toUtf8());
        return;
    }
    if (upvalue) {
        lua_getinfo(L, "f", ar);
        lua_insert(L, -2);
        lua_setupvalue(L, -2, upvalue);
        lua_pop(L, 1);
        m_io->stdOut(QString("\x1b[32mHot updated\x1b[39m: upvalue %1 = %2\n").arg(expression, display).toUtf8());
        return;
    }

    // assign the field with the typed value as the chunk argument
    sol::object newValue(L, -1);
    lua_pop(L, 1);
    env[sol::metatable_key] = lua.create_table_with(sol::meta_function::new_index, +[](lua_State *state) -> int {
        return luaL_error(state, "unknown watch variable '%s'", lua_tostring(state, 2));
    });
    const auto chunk = lua.load(expression.toStdString() + " = ...", "=watch");
    if (chunk.valid()) {
        sol::protected_function assign = chunk;
        sol::set_environment(env, assign);
        const auto result = assign(newValue);
        if (result.valid()) {
            m_io->stdOut(QString("\x1b[32mHot updated\x1b[39m: field %1 = %2\n").arg(expression, display).toUtf8());
            return;
        }
        const sol::error error = result;
        m_io->stdErr(QString("\x1b[33mHot update failed\x1b[39m for %1: %2\n").arg(expression, QString::fromUtf8(error.what())).toUtf8());
    } else {
        const sol::error error = chunk;
        m_io->stdErr(QString("\x1b[33mHot update failed\x1b[39m for %1: %2\n").arg(expression, QString::fromUtf8(error.what())).toUtf8());
    }
}

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
        const int currentLine = ar->currentline - 1;
        // debug state machine
        const auto state = session.value("state");
        if (state == InterpreterState::Resume) {
            const auto breakpoint = g_globalManager->breakpointModuleGet()->breakpointGet(currentUrl, currentLine);
            if (breakpoint.value("enabled").toBool()) {
                // conditional breakpoint
                {
                    QString condition = breakpoint.value("condition").toString();
                    if (condition.isEmpty()) {
                        This->interpreterStateSet(InterpreterState::Pause);
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
                            if (result.as<bool>()) This->interpreterStateSet(InterpreterState::Pause);
                        } else {
                            const sol::error err = condition_result;
                            This->m_io->stdErr(QString::fromStdString(err.what()).toUtf8() + '\n');
                        }
                        lua_settop(L, base);
                    }
                }
                // TODO: log/count
            }
        } else if (state == InterpreterState::StepOver && session["currentDepth"].toInt() <= session["baseDepth"].toInt()) This->interpreterStateSet(InterpreterState::Pause);
        else if (state == InterpreterState::StepOut && session["currentDepth"].toInt() < session["baseDepth"].toInt()) This->interpreterStateSet(InterpreterState::Pause);
        else if (state == InterpreterState::StepInto) This->interpreterStateSet(InterpreterState::Pause);
        else if (state == InterpreterState::RunToCursor && g_cursorPosition["url"].toUrl() == currentUrl && g_cursorPosition["line"].toInt() == currentLine)
            This->interpreterStateSet(InterpreterState::Pause);
        if (session.value("state") == InterpreterState::Pause) {
            This->m_watchValuesRefresh = false;
            QEventLoop loop{};
            lua_Debug frame{};
            lua_getstack(L, 0, &frame);
            connect(This, &LuaInterpreter::quitLoop, &loop, &QEventLoop::quit);
            connect(This, &LuaInterpreter::refreshWatchValues, &loop, [&] {
                This->m_watchValuesRefresh = false;
                emit This->setWatchValues(session["threadId"].toString(), currentUrl, This->watchValuesGet(L, &frame, currentUrl));
            });
            connect(This, &LuaInterpreter::setWatchValue, &loop, [&](const QString &expression, const QString &value, const LuaType::Value type) {
                This->watchValueSet(L, &frame, expression, value, type);
                This->watchValuesRefresh();
            });
            callStackSet(L, ar);
            if (!thread->isInterruptionRequested()) loop.exec();
            emit This->setCallStack(session["threadId"].toString(), {});
        } else if (This->m_watchValuesRefresh) {
            This->m_watchValuesRefresh = false;
            emit This->setWatchValues(session["threadId"].toString(), currentUrl, This->watchValuesGet(L, ar, currentUrl));
        }
    }
    if (thread->isInterruptionRequested()) luaL_error(L, "terminated");
}
