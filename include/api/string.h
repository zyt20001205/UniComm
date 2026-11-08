#ifndef UNICOMM_STRING_H
#define UNICOMM_STRING_H

#include <QObject>
#include <sol/object.hpp>

class String final : public QObject {
    Q_OBJECT

public:
    explicit String(QObject *parent = nullptr);

    ~String() override = default;

    [[nodiscard]] static std::string toBase64(const std::string &str);

    [[nodiscard]] static std::string fromBase64(const std::string &str);

    [[nodiscard]] static std::string toHex(const std::string &str, char separator);

    [[nodiscard]] static std::string fromHex(const std::string &str);

    [[nodiscard]] static std::string toJson(const sol::table &value);

    [[nodiscard]] static sol::table fromJson(sol::this_state ts, const std::string &str);

    [[nodiscard]] static std::string hash(const std::string &data, const std::string &algorithm);

    [[nodiscard]] static std::string encrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &algorithm);

    [[nodiscard]] static std::string decrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &algorithm);

private:
    [[nodiscard]] static std::string crypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &algorithm, bool encrypt);
};

#endif //UNICOMM_STRING_H
