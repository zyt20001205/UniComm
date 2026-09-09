#include "api/key.h"

#include <ranges>
#include <sol/error.hpp>
#include <windows.h>

Key::Key(QObject *parent)
    : QObject(parent),
      m_keyHash{
          {"BACKSPACE", {VK_BACK, false}},
          {"TAB", {VK_TAB, false}},
          {"ENTER", {VK_RETURN, false}},
          {"SHIFT", {VK_SHIFT, false}},
          {"CTRL", {VK_CONTROL, false}},
          {"ALT", {VK_MENU, false}},
          {"PAUSE", {VK_PAUSE, false}},
          {"CAPSLOCK", {VK_CAPITAL, false}},
          {"ESC", {VK_ESCAPE, false}},
          {"SPACE", {VK_SPACE, false}},
          {"PAGEUP", {VK_PRIOR, true}},
          {"PAGEDOWN", {VK_NEXT, true}},
          {"END", {VK_END, true}},
          {"HOME", {VK_HOME, true}},
          {"LEFT", {VK_LEFT, true}},
          {"UP", {VK_UP, true}},
          {"RIGHT", {VK_RIGHT, true}},
          {"DOWN", {VK_DOWN, true}},
          {"PRINTSCREEN", {VK_SNAPSHOT, true}},
          {"INSERT", {VK_INSERT, true}},
          {"DELETE", {VK_DELETE, true}}
      } {
}

Key::~Key() {
    for (auto key : std::views::reverse(m_pressed)) {
        try {
            inputSend(key, false);
        } catch (...) {
        }
    }
}

void Key::down(const std::string &key) {
    const auto code = keyGet(key);
    if (m_pressed.contains(code)) return;
    inputSend(code, true);
    m_pressed.append(code);
}

void Key::up(const std::string &key) {
    const auto code = keyGet(key);
    if (!m_pressed.contains(code)) return;
    inputSend(code, false);
    m_pressed.removeOne(code);
}

void Key::tap(const std::string &key) {
    const auto code = keyGet(key);
    if (m_pressed.contains(code)) throw sol::error("key is already pressed: " + key);
    inputSend(code, true);
    m_pressed.append(code);
    inputSend(code, false);
    m_pressed.removeOne(code);
}

void Key::type(const std::string &text) {
    for (const wchar_t character: QString::fromStdString(text).toStdWString()) inputSend(character);
}

// private
Key::KeyCode Key::keyGet(const std::string &key) const {
    const auto name = QString::fromStdString(key).toUpper();
    if (name.size() == 1) {
        const auto character = name.front();
        if ((character >= '0' && character <= '9') || (character >= 'A' && character <= 'Z')) return {character.unicode(), false};
    }
    const auto code = m_keyHash.constFind(name);
    if (code == m_keyHash.cend()) throw sol::error("invalid key: " + key);
    return code.value();
}

void Key::inputSend(const KeyCode &key, const bool down) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key.virtualKey;
    if (key.extended) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    if (!down) input.ki.dwFlags |= KEYEVENTF_KEYUP;
    if (SendInput(1, &input, sizeof(INPUT)) != 1) throw sol::error(QString("failed to send key input: %1").arg(GetLastError()).toStdString());
}

void Key::inputSend(const wchar_t character) {
    INPUT inputs[2]{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wScan = character;
    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
    inputs[1] = inputs[0];
    inputs[1].ki.dwFlags |= KEYEVENTF_KEYUP;
    if (SendInput(2, inputs, sizeof(INPUT)) != 2) throw sol::error(QString("failed to send text input: %1").arg(GetLastError()).toStdString());
}
