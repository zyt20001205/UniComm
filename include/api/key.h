#ifndef UNICOMM_KEY_H
#define UNICOMM_KEY_H

#include <QHash>
#include <QList>
#include <QObject>

class Key final : public QObject {
    Q_OBJECT

public:
    explicit Key(QObject *parent = nullptr);

    ~Key() override;

    void down(const std::string &key);

    void up(const std::string &key);

    void tap(const std::string &key);

    static void type(const std::string &text);

private:
    struct KeyCode {
        quint16 virtualKey{};
        bool extended{};

        bool operator==(const KeyCode &) const = default;
    };

    [[nodiscard]] KeyCode keyGet(const std::string &key) const;

    static void inputSend(const KeyCode &key, bool down);

    static void inputSend(wchar_t character);

    QHash<QString, KeyCode> m_keyHash{};
    QList<KeyCode> m_pressed{};
};

#endif //UNICOMM_KEY_H
