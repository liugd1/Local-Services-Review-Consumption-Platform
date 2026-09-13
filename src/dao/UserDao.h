#pragma once
// 用户与会话数据访问层
#include <nlohmann/json.hpp>
#include <string>

#include "model/Models.h"

class UserDao {
public:
    // ---- 用户 ----
    static User findByUsername(const std::string& username);
    static User findById(long long id);
    static long long insert(const User& u);  // 返回新 id，失败返回 0
    static bool updateProfile(long long id, const std::string& nickname,
                              const std::string& phone, const std::string& avatar);
    static bool updateStatus(long long id, const std::string& status);
    static bool updateRole(long long id, const std::string& role);

    // ---- 消费偏好 ----
    static void upsertPreferences(long long userId, const std::string& cats,
                                  const std::string& priceRange, const std::string& area);
    static nlohmann::json getPreferences(long long userId);

    // ---- 管理端列表（分页）----
    static nlohmann::json list(int page, int size, long long& total);

    // ---- 会话（Token）----
    static bool createSession(const std::string& token, long long userId, const std::string& expiresAt);
    // 返回 session+user 联查行（token/user_id/role/status/expires_at...），无则 nullptr
    static nlohmann::json findSessionUser(const std::string& token);
    static bool deleteSession(const std::string& token);
    static void deleteExpiredSessions();
};
