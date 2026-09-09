#include "api/mouse.h"

#include <QCursor>
#include <QThread>
#include <sol/error.hpp>
#include <windows.h>

Mouse::Mouse(QObject *parent)
    : QObject(parent) {
}

Mouse::~Mouse() {
    const auto release = [](const Button button, const bool pressed) {
        if (!pressed) return;
        try {
            inputSend(buttonFlag(button, false));
        } catch (...) {
        }
    };
    release(Button::Left, m_leftPressed);
    release(Button::Right, m_rightPressed);
    release(Button::Middle, m_middlePressed);
}

QPoint Mouse::position() {
    return QCursor::pos();
}

void Mouse::move(const int x, const int y) {
    QCursor::setPos(x, y);
}

void Mouse::down(const std::string &button) {
    const auto value = buttonGet(button);
    auto &pressed = buttonState(value);
    if (pressed) return;
    inputSend(buttonFlag(value, true));
    pressed = true;
}

void Mouse::up(const std::string &button) {
    const auto value = buttonGet(button);
    inputSend(buttonFlag(value, false));
    buttonState(value) = false;
}

void Mouse::click(const int x, const int y, const std::string &button) {
    move(x, y);
    down(button);
    up(button);
}

void Mouse::doubleClick(const int x, const int y, const std::string &button) {
    click(x, y, button);
    QThread::msleep(GetDoubleClickTime() / 2);
    click(x, y, button);
}

void Mouse::scroll(const int x, const int y, const int steps) {
    move(x, y);
    if (steps == 0) return;
    inputSend(MOUSEEVENTF_WHEEL, static_cast<unsigned long>(steps * WHEEL_DELTA));
}

// private
Mouse::Button Mouse::buttonGet(const std::string &button) {
    const auto value = QString::fromStdString(button).toLower();
    if (value == "left") return Button::Left;
    if (value == "right") return Button::Right;
    if (value == "middle") return Button::Middle;
    throw sol::error("invalid mouse button: " + button);
}

unsigned long Mouse::buttonFlag(const Button button, const bool down) {
    switch (button) {
        case Button::Left: return down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        case Button::Right: return down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        case Button::Middle: return down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        default: return 0;
    }
}

bool &Mouse::buttonState(const Button button) {
    switch (button) {
        case Button::Left: return m_leftPressed;
        case Button::Right: return m_rightPressed;
        case Button::Middle: return m_middlePressed;
        default: return m_leftPressed;
    }
}

void Mouse::inputSend(const unsigned long flags, const unsigned long data) {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    input.mi.mouseData = data;
    if (SendInput(1, &input, sizeof(INPUT)) != 1) throw sol::error(QString("failed to send mouse input: %1").arg(GetLastError()).toStdString());
}
