#pragma once
// 用户业务逻辑：注册 / 登录 / 会话 / 个人信息 / 偏好 / 管理端用户管理
#include <nlohmann/json.hpp>
#include <string>

class UserService {
public:
    static nlohmann::json registerUser(const std::string& username, const std::string& password,
                                       const std::string& nickname);
    static nlohmann::json login(const std::string& username, const std::string& password);
    static void logout(const std::string& token);

    static nlohmann::json getProfile(long long userId);
    static void updateProfile(long long userId, const nlohmann::json& body);
    static void updatePreferences(long long userId, const nlohmann::json& body);

    // 管理端
    static nlohmann::json listUsers(int page, int size);
    static void setUserStatus(long long targetUserId, const std::string& status);

    // 启动时确保 admin 账号存在（admin / admin123）
    static void ensureAdmin();
};
