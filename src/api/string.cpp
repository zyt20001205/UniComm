#include "api/string.h"

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
