#include "api/string.h"

#include <QCryptographicHash>
#include <openssl/evp.h>
#include <sol/table_core.hpp>

#include <limits>
#include <memory>

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

std::string String::encrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &algorithm) {
    return crypt(data, key, iv, algorithm, true);
}

std::string String::decrypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &algorithm) {
    return crypt(data, key, iv, algorithm, false);
}

std::string String::crypt(const std::string &data, const std::string &key, const std::string &iv, const std::string &algorithm, const bool encrypt) {
    const EVP_CIPHER *cipher{};
    if (algorithm == "AES-128-CBC") cipher = EVP_aes_128_cbc();
    else if (algorithm == "AES-192-CBC") cipher = EVP_aes_192_cbc();
    else if (algorithm == "AES-256-CBC") cipher = EVP_aes_256_cbc();
    else throw sol::error("unsupported cipher algorithm: " + algorithm);

    const auto keyLength = static_cast<std::size_t>(EVP_CIPHER_get_key_length(cipher));
    const auto ivLength = static_cast<std::size_t>(EVP_CIPHER_get_iv_length(cipher));
    if (key.size() != keyLength) throw sol::error(algorithm + ": key must be " + std::to_string(keyLength) + " bytes");
    if (iv.size() != ivLength) throw sol::error(algorithm + ": IV must be " + std::to_string(ivLength) + " bytes");

    const auto blockSize = static_cast<std::size_t>(EVP_CIPHER_get_block_size(cipher));
    if (data.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) - blockSize) throw sol::error(algorithm + ": data is too large");

    const std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> context(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!context) throw sol::error(algorithm + ": failed to create cipher context");
    const auto *keyBytes = reinterpret_cast<const unsigned char *>(key.data());
    const auto *ivBytes = reinterpret_cast<const unsigned char *>(iv.data());
    if (EVP_CipherInit_ex2(context.get(), cipher, keyBytes, ivBytes, encrypt, nullptr) != 1) throw sol::error(algorithm + ": failed to initialize cipher");

    std::string result(data.size() + blockSize, '\0');
    auto *output = reinterpret_cast<unsigned char *>(result.data());
    const auto *input = reinterpret_cast<const unsigned char *>(data.data());
    int size{}, finalSize{};
    if (EVP_CipherUpdate(context.get(), output, &size, input, static_cast<int>(data.size())) != 1 || EVP_CipherFinal_ex(context.get(), output + size, &finalSize) != 1) {
        throw sol::error(algorithm + (encrypt ? ": encryption failed" : ": decryption failed"));
    }
    result.resize(static_cast<std::size_t>(size) + static_cast<std::size_t>(finalSize));
    return result;
}
