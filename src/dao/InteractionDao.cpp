#include "dao/InteractionDao.h"

#include "db/Database.h"
#include "model/Models.h"
#include "util/Time.h"

namespace InteractionDao {

// ---------------- 收藏 ----------------
bool isFavorite(long long userId, const std::string& type, long long targetId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM favorites WHERE user_id = ? AND target_type = ? AND "
        "target_id = ?",
        {std::to_string(userId), type, std::to_string(targetId)});
    return row.is_null() ? false : (row.value("c", 0LL) > 0);
}

bool addFavorite(long long userId, const std::string& type, long long targetId) {
    auto& db = Database::instance();
    db.execute("INSERT OR IGNORE INTO favorites (user_id, target_type, target_id, created_at) "
               "VALUES (?, ?, ?, ?)",
               {std::to_string(userId), type, std::to_string(targetId), timeutil::nowStr()});
    return true;
}

bool removeFavorite(long long userId, const std::string& type, long long targetId) {
    return Database::instance().execute(
               "DELETE FROM favorites WHERE user_id = ? AND target_type = ? AND target_id = ?",
               {std::to_string(userId), type, std::to_string(targetId)}) >= 0;
}

nlohmann::json listFavorites(long long userId, const std::string& type) {
    auto& db = Database::instance();
    if (type == "merchant") {
        return db.query(
            "SELECT f.id, f.target_id AS merchant_id, f.created_at, m.name AS merchant_name, "
            "m.logo, m.area, m.status, m.price_min, m.price_max, "
            "c.name AS category_name FROM favorites f "
            "JOIN merchants m ON m.id = f.target_id AND f.target_type = 'merchant' "
            "LEFT JOIN categories c ON c.id = m.category_id "
            "WHERE f.user_id = ? ORDER BY f.id DESC",
            {std::to_string(userId)});
    }
    // service
    return db.query(
        "SELECT f.id, f.target_id AS service_id, s.name AS service_name, s.price, s.status, "
        "m.id AS merchant_id, m.name AS merchant_name FROM favorites f "
        "JOIN services s ON s.id = f.target_id AND f.target_type = 'service' "
        "JOIN merchants m ON m.id = s.merchant_id "
        "WHERE f.user_id = ? ORDER BY f.id DESC",
        {std::to_string(userId)});
}

long long countFavorites(const std::string& type, long long targetId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM favorites WHERE target_type = ? AND target_id = ?",
        {type, std::to_string(targetId)});
    return row.is_null() ? 0 : row.value("c", 0LL);
}

// ---------------- 关注 ----------------
bool isFollowing(long long userId, long long merchantId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM follows WHERE user_id = ? AND merchant_id = ?",
        {std::to_string(userId), std::to_string(merchantId)});
    return row.is_null() ? false : (row.value("c", 0LL) > 0);
}

bool addFollow(long long userId, long long merchantId) {
    Database::instance().execute(
        "INSERT OR IGNORE INTO follows (user_id, merchant_id, created_at) VALUES (?, ?, ?)",
        {std::to_string(userId), std::to_string(merchantId), timeutil::nowStr()});
    return true;
}

bool removeFollow(long long userId, long long merchantId) {
    return Database::instance().execute(
               "DELETE FROM follows WHERE user_id = ? AND merchant_id = ?",
               {std::to_string(userId), std::to_string(merchantId)}) >= 0;
}

nlohmann::json listFollows(long long userId) {
    return Database::instance().query(
        "SELECT f.id, f.merchant_id, f.created_at, m.name AS merchant_name, m.logo, m.area, "
        "m.status, m.business_hours, c.name AS category_name, m.intro FROM follows f "
        "JOIN merchants m ON m.id = f.merchant_id "
        "LEFT JOIN categories c ON c.id = m.category_id "
        "WHERE f.user_id = ? ORDER BY f.id DESC",
        {std::to_string(userId)});
}

long long countFollowers(long long merchantId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM follows WHERE merchant_id = ?", {std::to_string(merchantId)});
    return row.is_null() ? 0 : row.value("c", 0LL);
}

// ---------------- 浏览历史 ----------------
bool addHistory(long long userId, long long merchantId) {
    return Database::instance().execute(
               "INSERT INTO browse_history (user_id, merchant_id, viewed_at) VALUES (?, ?, ?)",
               {std::to_string(userId), std::to_string(merchantId), timeutil::nowStr()}) >= 0;
}

nlohmann::json listHistory(long long userId, int limit) {
    return Database::instance().query(
        "SELECT h.id, h.merchant_id, h.viewed_at, m.name AS merchant_name, m.logo, m.area, "
        "c.name AS category_name FROM browse_history h "
        "JOIN merchants m ON m.id = h.merchant_id "
        "LEFT JOIN categories c ON c.id = m.category_id "
        "WHERE h.user_id = ? ORDER BY h.id DESC LIMIT ?",
        {std::to_string(userId), std::to_string(limit)});
}

// ---------------- 消费记录 ----------------
bool addConsumption(long long userId, const nlohmann::json& rec) {
    auto& db = Database::instance();
    std::string svcId = jsonStr(rec, "service_id");
    std::string pkgId = jsonStr(rec, "package_id");
    std::string now = timeutil::nowStr();
    std::string consumeTime = jsonStr(rec, "consume_time");
    if (consumeTime.empty()) consumeTime = now;
    std::string merchantId = jsonStr(rec, "merchant_id");
    std::string amount = std::to_string(jsonNum(rec, "amount"));

    long affected;
    if (svcId.empty() && pkgId.empty()) {
        affected = db.execute(
            "INSERT INTO consumption_records (user_id, merchant_id, service_id, package_id, "
            "amount, consume_time, created_at) VALUES (?, ?, NULL, NULL, ?, ?, ?)",
            {std::to_string(userId), merchantId, amount, consumeTime, now});
    } else if (!svcId.empty() && pkgId.empty()) {
        affected = db.execute(
            "INSERT INTO consumption_records (user_id, merchant_id, service_id, package_id, "
            "amount, consume_time, created_at) VALUES (?, ?, ?, NULL, ?, ?, ?)",
            {std::to_string(userId), merchantId, svcId, amount, consumeTime, now});
    } else if (svcId.empty()) {  // 仅套餐
        affected = db.execute(
            "INSERT INTO consumption_records (user_id, merchant_id, service_id, package_id, "
            "amount, consume_time, created_at) VALUES (?, ?, NULL, ?, ?, ?, ?)",
            {std::to_string(userId), merchantId, pkgId, amount, consumeTime, now});
    } else {  // 服务 + 套餐
        affected = db.execute(
            "INSERT INTO consumption_records (user_id, merchant_id, service_id, package_id, "
            "amount, consume_time, created_at) VALUES (?, ?, ?, ?, ?, ?, ?)",
            {std::to_string(userId), merchantId, svcId, pkgId, amount, consumeTime, now});
    }
    return affected >= 0;
}

nlohmann::json listConsumptions(long long userId, int page, int size, long long& total) {
    auto& db = Database::instance();
    auto totalRow = db.queryOne(
        "SELECT COUNT(*) AS c FROM consumption_records WHERE user_id = ?",
        {std::to_string(userId)});
    total = totalRow.is_null() ? 0 : totalRow.value("c", 0LL);
    int offset = (page - 1) * size;
    return db.query(
        "SELECT cr.id, cr.merchant_id, cr.service_id, cr.package_id, cr.amount, "
        "cr.consume_time, cr.created_at, m.name AS merchant_name, "
        "s.name AS service_name, p.name AS package_name FROM consumption_records cr "
        "JOIN merchants m ON m.id = cr.merchant_id "
        "LEFT JOIN services s ON s.id = cr.service_id "
        "LEFT JOIN packages p ON p.id = cr.package_id "
        "WHERE cr.user_id = ? ORDER BY cr.id DESC LIMIT ? OFFSET ?",
        {std::to_string(userId), std::to_string(size), std::to_string(offset)});
}

}  // namespace InteractionDao
