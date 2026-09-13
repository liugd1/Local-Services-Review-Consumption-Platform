#include "service/UserService.h"

#include <cctype>
#include <iostream>

#include "dao/UserDao.h"
#include "db/Database.h"
#include "util/BizError.h"
#include "util/Crypto.h"
#include "util/Time.h"

namespace {

bool isValidUsername(const std::string& s) {
    if (s.size() < 3 || s.size() > 32) return false;
    for (char c : s) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
    }
    return true;
}

}  // namespace

nlohmann::json UserService::registerUser(const std::string& username, const std::string& password,
                                         const std::string& nickname) {
    if (!isValidUsername(username)) throw BizError(resp::PARAM_ERROR, "用户名需为 3-32 位字母/数字/下划线");
    if (password.size() < 6 || password.size() > 64)
        throw BizError(resp::PARAM_ERROR, "密码长度需在 6-64 位之间");

    if (UserDao::findByUsername(username).valid())
        throw BizError(resp::CONFLICT, "用户名已存在");

    User u;
    u.username = username;
    u.salt = crypto::genSalt();
    u.passwordHash = crypto::hashPassword(password, u.salt);
    u.nickname = nickname.empty() ? username : nickname;
    u.createdAt = timeutil::nowStr();

    auto id = UserDao::insert(u);
    if (id == 0) throw BizError(resp::SERVER_ERROR, "注册失败：" + Database::instance().lastError());
    UserDao::upsertPreferences(id, "", "", "");

    return UserDao::findById(id).toPublicJson();
}

nlohmann::json UserService::login(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty())
        throw BizError(resp::PARAM_ERROR, "用户名和密码不能为空");

    auto u = UserDao::findByUsername(username);
    if (!u.valid() || !crypto::verifyPassword(password, u.salt, u.passwordHash))
        throw BizError(resp::UNAUTHORIZED, "用户名或密码错误");
    if (u.status != "active") throw BizError(resp::FORBIDDEN, "账号已被禁用，请联系管理员");

    auto token = crypto::genToken();
    auto expires = timeutil::afterDaysStr(7);
    if (!UserDao::createSession(token, u.id, expires))
        throw BizError(resp::SERVER_ERROR, "创建会话失败");

    return {{"token", token}, {"expires_at", expires}, {"user", u.toPublicJson()}};
}

void UserService::logout(const std::string& token) {
    if (!token.empty()) UserDao::deleteSession(token);
}

nlohmann::json UserService::getProfile(long long userId) {
    auto u = UserDao::findById(userId);
    if (!u.valid()) throw BizError(resp::NOT_FOUND, "用户不存在");
    return {{"user", u.toPublicJson()}, {"preferences", UserDao::getPreferences(userId)}};
}

void UserService::updateProfile(long long userId, const nlohmann::json& body) {
    auto u = UserDao::findById(userId);
    if (!u.valid()) throw BizError(resp::NOT_FOUND, "用户不存在");
    // 部分更新：仅当客户端显式传了对应字段才覆盖；nickname 不允许被清空
    auto it = body.find("nickname");
    if (it != body.end()) {
        auto nickname = jsonStr(body, "nickname");
        if (nickname.empty()) throw BizError(resp::PARAM_ERROR, "昵称不能为空");
        u.nickname = nickname;
    }
    if (body.contains("phone")) u.phone = jsonStr(body, "phone");
    if (body.contains("avatar")) u.avatar = jsonStr(body, "avatar");
    UserDao::updateProfile(userId, u.nickname, u.phone, u.avatar);
}

void UserService::updatePreferences(long long userId, const nlohmann::json& body) {
    // prefer_categories 支持数组 [1,2] 或字符串 "1,2"
    std::string cats;
    auto it = body.find("prefer_categories");
    if (it != body.end()) {
        if (it->is_array()) {
            for (const auto& x : *it) {
                if (!cats.empty()) cats += ",";
                if (x.is_number_integer()) {
                    cats += std::to_string(x.get<long long>());
                } else if (x.is_string()) {
                    cats += x.get<std::string>();
                } else {
                    cats += x.dump();
                }
            }
        } else {
            cats = jsonStr(body, "prefer_categories");
        }
    }
    UserDao::upsertPreferences(userId, cats, jsonStr(body, "price_range"), jsonStr(body, "area"));
}

nlohmann::json UserService::listUsers(int page, int size) {
    long long total = 0;
    auto rows = UserDao::list(page, size, total);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)}};
}

void UserService::setUserStatus(long long targetUserId, const std::string& status) {
    if (status != "active" && status != "disabled")
        throw BizError(resp::PARAM_ERROR, "status 仅支持 active / disabled");
    auto u = UserDao::findById(targetUserId);
    if (!u.valid()) throw BizError(resp::NOT_FOUND, "用户不存在");
    if (u.role == "admin") throw BizError(resp::FORBIDDEN, "不允许修改管理员状态");
    UserDao::updateStatus(targetUserId, status);
}

void UserService::ensureAdmin() {
    if (UserDao::findByUsername("admin").valid()) return;
    User admin;
    admin.username = "admin";
    admin.salt = crypto::genSalt();
    admin.passwordHash = crypto::hashPassword("admin123", admin.salt);
    admin.role = "admin";
    admin.nickname = "平台管理员";
    admin.createdAt = timeutil::nowStr();
    if (UserDao::insert(admin) > 0) {
        std::cout << "[init] admin account created (username=admin, password=admin123)" << std::endl;
    }
}
