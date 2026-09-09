#ifndef UNICOMM_MOUSE_H
#define UNICOMM_MOUSE_H

#include <QObject>
#include <QPoint>

class Mouse final : public QObject {
    Q_OBJECT

public:
    explicit Mouse(QObject *parent = nullptr);

    ~Mouse() override;

    [[nodiscard]] static QPoint position();

    static void move(int x, int y);

    void down(const std::string &button);

    void up(const std::string &button);

    void click(int x, int y, const std::string &button);

    void doubleClick(int x, int y, const std::string &button);

    static void scroll(int x, int y, int steps);

private:
    enum class Button {
        Left,
        Right,
        Middle
    };

    [[nodiscard]] static Button buttonGet(const std::string &button);

    [[nodiscard]] static unsigned long buttonFlag(Button button, bool down);

    [[nodiscard]] bool &buttonState(Button button);

    static void inputSend(unsigned long flags, unsigned long data = 0);

    bool m_leftPressed{};
    bool m_rightPressed{};
    bool m_middlePressed{};
};

#endif //UNICOMM_MOUSE_H
