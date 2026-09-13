#pragma once
// 密码加盐哈希 / 随机 Token 生成
#include <picosha2.h>

#include <random>
#include <string>
#include <vector>

namespace crypto {

inline std::string toHex(const std::vector<unsigned char>& bytes) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (unsigned char b : bytes) {
        out.push_back(digits[b >> 4]);
        out.push_back(digits[b & 0x0F]);
    }
    return out;
}

// 生成 n 字节的随机数并转为 hex 字符串（输出长度 2n）
inline std::string randomHex(size_t byteCount) {
    static thread_local std::mt19937 gen(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, 255);
    std::vector<unsigned char> bytes(byteCount);
    for (auto& b : bytes) b = static_cast<unsigned char>(dist(gen));
    return toHex(bytes);
}

// 16 字节盐 -> 32 位 hex
inline std::string genSalt() { return randomHex(16); }

// 登录会话 Token：32 字节 -> 64 位 hex
inline std::string genToken() { return randomHex(32); }

inline std::string sha256Hex(const std::string& input) {
    std::vector<unsigned char> digest(picosha2::k_digest_size);
    picosha2::hash256(input, digest);
    return toHex(digest);
}

// 口令存储：sha256(password + ":" + salt)
inline std::string hashPassword(const std::string& password, const std::string& salt) {
    return sha256Hex(password + ":" + salt);
}

// 校验口令
inline bool verifyPassword(const std::string& password, const std::string& salt,
                           const std::string& expectedHash) {
    return hashPassword(password, salt) == expectedHash;
}

}  // namespace crypto
