#pragma once
// 用户实体
#include <nlohmann/json.hpp>

#include <string>

struct User {
    long long id = 0;
    std::string username;
    std::string passwordHash;
    std::string salt;
    std::string role = "consumer";
    std::string nickname;
    std::string phone;
    std::string avatar;
    std::string status = "active";
    std::string createdAt;

    static User fromJson(const nlohmann::json& j) {
        User u;
        u.id = j.value("id", 0LL);
        u.username = j.value("username", "");
        u.passwordHash = j.value("password_hash", "");
        u.salt = j.value("salt", "");
        u.role = j.value("role", "consumer");
        u.nickname = j.value("nickname", "");
        u.phone = j.value("phone", "");
        u.avatar = j.value("avatar", "");
        u.status = j.value("status", "active");
        u.createdAt = j.value("created_at", "");
        return u;
    }

    // 对外输出（剔除敏感字段 password_hash / salt）
    nlohmann::json toPublicJson() const {
        return {{"id", id},
                {"username", username},
                {"role", role},
                {"nickname", nickname},
                {"phone", phone},
                {"avatar", avatar},
                {"status", status},
                {"created_at", createdAt}};
    }

    bool valid() const { return id > 0; }
};

// 取 JSON 字符串字段（非字符串 / 缺失返回默认值）
inline std::string jsonStr(const nlohmann::json& j, const char* key,
                           const std::string& def = "") {
    auto it = j.find(key);
    if (it == j.end()) return def;
    if (it->is_string()) return it->get<std::string>();
    if (it->is_number_integer()) return std::to_string(it->get<long long>());
    if (it->is_number_float()) return it->dump();
    if (it->is_boolean()) return it->get<bool>() ? "true" : "false";
    return def;
}

// 取 JSON 数值字段（数字或数字字符串），解析失败返回默认值
inline double jsonNum(const nlohmann::json& j, const char* key, double def = 0.0) {
    auto it = j.find(key);
    if (it == j.end()) return def;
    if (it->is_number()) return it->get<double>();
    if (it->is_string()) {
        try {
            return std::stod(it->get<std::string>());
        } catch (...) {
        }
    }
    return def;
}

inline long long jsonInt(const nlohmann::json& j, const char* key, long long def = 0) {
    auto it = j.find(key);
    if (it == j.end()) return def;
    if (it->is_number_integer()) return it->get<long long>();
    if (it->is_number()) return static_cast<long long>(it->get<double>());
    if (it->is_string()) {
        try {
            return std::stoll(it->get<std::string>());
        } catch (...) {
        }
    }
    return def;
}

// JSON 数组或逗号分隔字符串 → 逗号分隔字符串
inline std::string jsonArrayToCsv(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end()) return "";
    if (it->is_string()) return it->get<std::string>();
    if (it->is_array()) {
        std::string out;
        for (const auto& x : *it) {
            if (!out.empty()) out += ",";
            if (x.is_string()) {
                out += x.get<std::string>();
            } else {
                out += x.dump();
            }
        }
        return out;
    }
    return "";
}
