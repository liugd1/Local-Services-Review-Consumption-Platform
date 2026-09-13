#include "dao/UserDao.h"

#include "db/Database.h"
#include "util/Time.h"

User UserDao::findByUsername(const std::string& username) {
    auto row = Database::instance().queryOne("SELECT * FROM users WHERE username = ?", {username});
    return row.is_null() ? User{} : User::fromJson(row);
}

User UserDao::findById(long long id) {
    auto row = Database::instance().queryOne("SELECT * FROM users WHERE id = ?",
                                             {std::to_string(id)});
    return row.is_null() ? User{} : User::fromJson(row);
}

long long UserDao::insert(const User& u) {
    auto row = Database::instance().queryOne(
        "INSERT INTO users (username, password_hash, salt, role, nickname, phone, avatar, "
        "status, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) RETURNING id",
        {u.username, u.passwordHash, u.salt, u.role, u.nickname, u.phone, u.avatar, u.status,
         u.createdAt});
    return row.is_null() ? 0 : row.value("id", 0LL);
}

bool UserDao::updateProfile(long long id, const std::string& nickname, const std::string& phone,
                            const std::string& avatar) {
    return Database::instance().execute(
               "UPDATE users SET nickname = ?, phone = ?, avatar = ? WHERE id = ?",
               {nickname, phone, avatar, std::to_string(id)}) >= 0;
}

bool UserDao::updateStatus(long long id, const std::string& status) {
    return Database::instance().execute("UPDATE users SET status = ? WHERE id = ?",
                                        {status, std::to_string(id)}) >= 0;
}

bool UserDao::updateRole(long long id, const std::string& role) {
    return Database::instance().execute("UPDATE users SET role = ? WHERE id = ?",
                                        {role, std::to_string(id)}) >= 0;
}

void UserDao::upsertPreferences(long long userId, const std::string& cats,
                                const std::string& priceRange, const std::string& area) {
    Database::instance().execute(
        "INSERT INTO user_preferences (user_id, prefer_categories, price_range, area) "
        "VALUES (?, ?, ?, ?) "
        "ON CONFLICT(user_id) DO UPDATE SET prefer_categories = excluded.prefer_categories, "
        "price_range = excluded.price_range, area = excluded.area",
        {std::to_string(userId), cats, priceRange, area});
}

nlohmann::json UserDao::getPreferences(long long userId) {
    auto row = Database::instance().queryOne(
        "SELECT user_id, prefer_categories, price_range, area FROM user_preferences WHERE user_id = ?",
        {std::to_string(userId)});
    if (!row.is_null()) return row;
    return {{"user_id", userId}, {"prefer_categories", ""}, {"price_range", ""}, {"area", ""}};
}

nlohmann::json UserDao::list(int page, int size, long long& total) {
    auto& db = Database::instance();
    auto totalRow = db.queryOne("SELECT COUNT(*) AS c FROM users");
    total = totalRow.is_null() ? 0 : totalRow.value("c", 0LL);
    int offset = (page - 1) * size;
    return db.query(
        "SELECT id, username, role, nickname, phone, avatar, status, created_at "
        "FROM users ORDER BY id LIMIT ? OFFSET ?",
        {std::to_string(size), std::to_string(offset)});
}

bool UserDao::createSession(const std::string& token, long long userId,
                            const std::string& expiresAt) {
    return Database::instance().execute(
               "INSERT INTO sessions (token, user_id, created_at, expires_at) VALUES (?, ?, ?, ?)",
               {token, std::to_string(userId), timeutil::nowStr(), expiresAt}) >= 0;
}

nlohmann::json UserDao::findSessionUser(const std::string& token) {
    return Database::instance().queryOne(
        "SELECT s.token, u.id AS user_id, u.username, u.role, u.nickname, u.status, s.expires_at "
        "FROM sessions s JOIN users u ON u.id = s.user_id WHERE s.token = ?",
        {token});
}

bool UserDao::deleteSession(const std::string& token) {
    return Database::instance().execute("DELETE FROM sessions WHERE token = ?", {token}) >= 0;
}

void UserDao::deleteExpiredSessions() {
    Database::instance().execute("DELETE FROM sessions WHERE expires_at <= ?",
                                 {timeutil::nowStr()});
}
