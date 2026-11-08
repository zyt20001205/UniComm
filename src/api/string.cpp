#include "api/string.h"

#include <QCryptographicHash>
#include <sol/table_core.hpp>

#include "util/uniCast.h"

String::String(QObject *parent)
    : QObject(parent) {
}

std::string String::toBase64(const std::string &str) {
    const auto qba = QByteArray::fromStdString(str).toBase64();
    return {qba.data(), static_cast<std::string::size_type>(qba.size())};
}

std::string String::fromBase64(const std::string &str) {
    const auto qba = QByteArray::fromBase64(QByteArray::fromStdString(str));
    return {qba.data(), static_cast<std::string::size_type>(qba.size())};
}

std::string String::toHex(const std::string &str, const char separator) {
    const auto qba = QByteArray::fromStdString(str).toHex(separator).toUpper();
    return {qba.data(), static_cast<std::string::size_type>(qba.size())};
}

std::string String::fromHex(const std::string &str) {
    const auto qba = QByteArray::fromHex(QByteArray::fromStdString(str));
    return {qba.data(), static_cast<std::string::size_type>(qba.size())};
}

std::string String::toJson(const sol::table &value) {
    const auto json = QJsonDocument::fromVariant(uni_cast<QVariant>(sol::object(value))).toJson(QJsonDocument::Compact);
    return {json.constData(), static_cast<std::string::size_type>(json.size())};
}

sol::table String::fromJson(const sol::this_state ts, const std::string &str) {
    QJsonParseError error{};
    const auto document = QJsonDocument::fromJson(QByteArray::fromStdString(str), &error);
    if (error.error != QJsonParseError::NoError) throw sol::error(QString("invalid JSON at offset %1: %2").arg(error.offset).arg(error.errorString()).toStdString());
    return uni_cast<sol::object>(ts, document.toVariant()).as<sol::table>();
}

std::string String::hash(const std::string &data, const std::string &algorithm) {
    QCryptographicHash::Algorithm method{};
    if (algorithm == "MD5") method = QCryptographicHash::Md5;
    else if (algorithm == "SHA-1") method = QCryptographicHash::Sha1;
    else if (algorithm == "SHA-256") method = QCryptographicHash::Sha256;
    else if (algorithm == "SHA-512") method = QCryptographicHash::Sha512;
    else throw sol::error("unsupported hash algorithm: " + algorithm);

    const auto digest = QCryptographicHash::hash(QByteArrayView(data), method);
    if (digest.isEmpty()) throw sol::error("failed to hash using " + algorithm);
    return digest.toStdString();
}
