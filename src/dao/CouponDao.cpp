#include "dao/CouponDao.h"

#include <map>
#include <random>
#include <string>

#include "db/Database.h"
#include "model/Models.h"
#include "util/Time.h"

namespace CouponDao {

namespace {
std::string genCode() {
    static std::mt19937_64 gen(std::random_device{}());
    static const char* d = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    std::string code = "CX";
    for (int i = 0; i < 8; i++) code += d[gen() % 32];
    return code;
}
}  // namespace

long long create(long long merchantId, const nlohmann::json& body) {
    auto row = Database::instance().queryOne(
        "INSERT INTO coupons (merchant_id, type, name, face_value, threshold, discount_rate, "
        "total, received, used, start_time, end_time, scope, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, 0, 0, ?, ?, ?, 'draft', ?) RETURNING id",
        {std::to_string(merchantId), jsonStr(body, "type", "coupon"), jsonStr(body, "name"),
         std::to_string(jsonNum(body, "face_value")), std::to_string(jsonNum(body, "threshold")),
         std::to_string(jsonNum(body, "discount_rate")),
         std::to_string(jsonInt(body, "total")), jsonStr(body, "start_time"),
         jsonStr(body, "end_time"), jsonStr(body, "scope"), timeutil::nowStr()});
    return row.is_null() ? 0 : row.value("id", 0LL);
}

nlohmann::json byId(long long id) {
    auto row = Database::instance().queryOne(
        "SELECT c.*, m.name AS merchant_name FROM coupons c "
        "JOIN merchants m ON m.id = c.merchant_id WHERE c.id = ?",
        {std::to_string(id)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

nlohmann::json byCode(const std::string& code) {
    auto row = Database::instance().queryOne(
        "SELECT cu.id AS claim_id, cu.coupon_id, cu.status AS claim_status, cu.received_at, "
        "cu.used_time, c.*, m.name AS merchant_name, u.id AS user_id, "
        "u.nickname AS claim_user "
        "FROM coupon_user cu JOIN coupons c ON c.id = cu.coupon_id "
        "JOIN merchants m ON m.id = c.merchant_id JOIN users u ON u.id = cu.user_id "
        "WHERE cu.code = ?",
        {code});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

nlohmann::json listByMerchant(long long merchantId) {
    return Database::instance().query(
        "SELECT c.*, m.name AS merchant_name FROM coupons c "
        "JOIN merchants m ON m.id = c.merchant_id "
        "WHERE c.merchant_id = ? AND c.deleted_at IS NULL ORDER BY c.id DESC",
        {std::to_string(merchantId)});
}

nlohmann::json listPublished(long long merchantId) {
    std::string now = timeutil::nowStr();
    return Database::instance().query(
        "SELECT c.*, m.name AS merchant_name FROM coupons c "
        "JOIN merchants m ON m.id = c.merchant_id "
        "WHERE c.merchant_id = ? AND c.status = 'published' AND c.start_time <= ? "
        "AND c.end_time >= ? AND c.deleted_at IS NULL ORDER BY c.id DESC",
        {std::to_string(merchantId), now, now});
}

bool updateStatus(long long id, const std::string& status) {
    return Database::instance().execute("UPDATE coupons SET status = ? WHERE id = ?",
                                        {status, std::to_string(id)}) >= 0;
}

// ---------------- 优惠活动适用对象 ----------------
void setTargets(long long couponId, const nlohmann::json& targets) {
    auto& db = Database::instance();
    db.execute("DELETE FROM coupon_targets WHERE coupon_id = ?", {std::to_string(couponId)});
    if (!targets.is_array()) return;
    const std::string now = timeutil::nowStr();
    for (const auto& t : targets) {
        std::string type;
        long long id = 0;
        if (t.is_object()) {
            type = jsonStr(t, "target_type");
            id = t.value("target_id", 0LL);
        } else if (t.is_string()) {
            // 兼容 "service:12" 形式
            auto s = t.get<std::string>();
            auto pos = s.find(':');
            if (pos != std::string::npos) {
                type = s.substr(0, pos);
                try { id = std::stoll(s.substr(pos + 1)); } catch (...) { id = 0; }
            }
        }
        if (type != "service" && type != "package") continue;
        if (id <= 0) continue;
        db.execute(
            "INSERT OR REPLACE INTO coupon_targets (coupon_id, target_type, target_id, created_at) "
            "VALUES (?, ?, ?, ?)",
            {std::to_string(couponId), type, std::to_string(id), now});
    }
}

nlohmann::json listTargets(long long couponId) {
    return Database::instance().query(
        "SELECT ct.target_type, ct.target_id, "
        "CASE ct.target_type WHEN 'package' THEN p.name ELSE s.name END AS name, "
        "CASE ct.target_type WHEN 'package' THEN p.price ELSE s.price END AS price "
        "FROM coupon_targets ct "
        "LEFT JOIN packages p ON ct.target_type = 'package' AND p.id = ct.target_id "
        "LEFT JOIN services s ON ct.target_type = 'service' AND s.id = ct.target_id "
        "WHERE ct.coupon_id = ? ORDER BY ct.id",
        {std::to_string(couponId)});
}

nlohmann::json withTargets(const nlohmann::json& coupons, long long merchantId) {
    auto& db = Database::instance();
    auto rows = db.query(
        "SELECT ct.coupon_id, ct.target_type, ct.target_id, "
        "CASE ct.target_type WHEN 'package' THEN p.name ELSE s.name END AS name "
        "FROM coupon_targets ct "
        "JOIN coupons c ON c.id = ct.coupon_id "
        "LEFT JOIN packages p ON ct.target_type = 'package' AND p.id = ct.target_id "
        "LEFT JOIN services s ON ct.target_type = 'service' AND s.id = ct.target_id "
        "WHERE c.merchant_id = ? ORDER BY ct.id",
        {std::to_string(merchantId)});
    std::map<long long, nlohmann::json> grouped;
    for (auto& r : rows) grouped[r.value("coupon_id", 0LL)].push_back(r);

    nlohmann::json out = nlohmann::json::array();
    for (auto& c : coupons) {
        nlohmann::json row = c;
        long long cid = c.value("id", 0LL);
        nlohmann::json list = grouped.count(cid) ? grouped[cid] : nlohmann::json::array();
        std::string text;
        for (auto& t : list) {
            if (!text.empty()) text += "、";
            text += (jsonStr(t, "target_type") == "package" ? "套餐：" : "服务：") + jsonStr(t, "name");
        }
        row["targets"] = list;
        row["targets_text"] = text;
        row["scope_desc"] = text.empty() ? "全场通用" : text;
        out.push_back(row);
    }
    return out;
}

bool isApplicable(long long couponId, const std::string& itemType, long long itemId) {
    auto& db = Database::instance();
    auto cnt = db.queryOne("SELECT COUNT(*) AS c FROM coupon_targets WHERE coupon_id = ?",
                           {std::to_string(couponId)});
    long long total = cnt.is_null() ? 0 : cnt.value("c", 0LL);
    if (total == 0) return true;  // 未绑定任何对象 → 全场通用
    auto hit = db.queryOne(
        "SELECT COUNT(*) AS c FROM coupon_targets WHERE coupon_id = ? AND target_type = ? "
        "AND target_id = ?",
        {std::to_string(couponId), itemType, std::to_string(itemId)});
    return !hit.is_null() && hit.value("c", 0LL) > 0;
}

nlohmann::json claimById(long long claimId) {
    auto row = Database::instance().queryOne(
        "SELECT cu.id AS claim_id, cu.coupon_id, cu.user_id, cu.code, cu.status AS claim_status, "
        "cu.received_at, cu.used_time, c.*, m.name AS merchant_name "
        "FROM coupon_user cu JOIN coupons c ON c.id = cu.coupon_id "
        "JOIN merchants m ON m.id = c.merchant_id WHERE cu.id = ?",
        {std::to_string(claimId)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

bool markClaimUsed(long long claimId) {
    auto& db = Database::instance();
    long changed = db.execute(
        "UPDATE coupon_user SET status = 'used', used_time = ? WHERE id = ? AND status = 'unused'",
        {timeutil::nowStr(), std::to_string(claimId)});
    if (changed != 1) return false;
    db.execute("UPDATE coupons SET used = used + 1 WHERE id = "
               "(SELECT coupon_id FROM coupon_user WHERE id = ?)",
               {std::to_string(claimId)});
    return true;
}

bool releaseClaim(long long claimId) {
    auto& db = Database::instance();
    long changed = db.execute(
        "UPDATE coupon_user SET status = 'unused', used_time = NULL WHERE id = ? "
        "AND status = 'used'",
        {std::to_string(claimId)});
    if (changed != 1) return false;
    db.execute("UPDATE coupons SET used = CASE WHEN used > 0 THEN used - 1 ELSE 0 END "
               "WHERE id = (SELECT coupon_id FROM coupon_user WHERE id = ?)",
               {std::to_string(claimId)});
    return true;
}

bool alreadyClaimed(long long couponId, long long userId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM coupon_user WHERE coupon_id = ? AND user_id = ?",
        {std::to_string(couponId), std::to_string(userId)});
    return row.is_null() ? false : (row.value("c", 0LL) > 0);
}

// 领取：数据库写串行化下“先扣后插”，防超发且对重复领取自动补偿
int tryReceive(long long couponId, long long userId) {
    auto& db = Database::instance();
    std::string now = timeutil::nowStr();
    long changed = db.execute(
        "UPDATE coupons SET received = received + 1 WHERE id = ? AND status = 'published' "
        "AND start_time <= ? AND end_time >= ? AND (total = 0 OR received < total)",
        {std::to_string(couponId), now, now});
    if (changed != 1) return 0;  // 未发布 / 过期 / 已领完

    long ins = db.execute(
        "INSERT OR IGNORE INTO coupon_user (coupon_id, user_id, code, status, received_at) "
        "VALUES (?, ?, ?, 'unused', ?)",
        {std::to_string(couponId), std::to_string(userId), genCode(), now});
    if (ins == 0) {  // 重复领取：回滚计数
        db.execute("UPDATE coupons SET received = received - 1 WHERE id = ?",
                   {std::to_string(couponId)});
        return 2;
    }
    return 1;
}

bool tryVerify(long long couponId, long long merchantId, long long claimId) {
    auto& db = Database::instance();
    std::string now = timeutil::nowStr();
    long changed = db.execute(
        "UPDATE coupon_user SET status = 'used', used_time = ? WHERE id = ? "
        "AND coupon_id = ? AND status = 'unused'",
        {now, std::to_string(claimId), std::to_string(couponId)});
    if (changed != 1) return false;
    db.execute("UPDATE coupons SET used = used + 1 WHERE id = ?", {std::to_string(couponId)});
    return true;
}

nlohmann::json listUserClaims(long long userId, const std::string& status) {
    auto& db = Database::instance();
    if (status.empty() || status == "all") {
        return db.query(
            "SELECT cu.id AS claim_id, cu.coupon_id, cu.code, cu.status AS claim_status, "
            "cu.received_at, cu.used_time, c.name, c.type, c.face_value, c.threshold, "
            "c.discount_rate, c.start_time, c.end_time, c.status AS coupon_status, "
            "c.merchant_id, m.name AS merchant_name FROM coupon_user cu "
            "JOIN coupons c ON c.id = cu.coupon_id JOIN merchants m ON m.id = c.merchant_id "
            "WHERE cu.user_id = ? AND cu.deleted_at IS NULL ORDER BY cu.id DESC",
            {std::to_string(userId)});
    }
    return db.query(
        "SELECT cu.id AS claim_id, cu.coupon_id, cu.code, cu.status AS claim_status, "
        "cu.received_at, cu.used_time, c.name, c.type, c.face_value, c.threshold, "
        "c.discount_rate, c.start_time, c.end_time, c.status AS coupon_status, "
        "c.merchant_id, m.name AS merchant_name FROM coupon_user cu "
        "JOIN coupons c ON c.id = cu.coupon_id JOIN merchants m ON m.id = c.merchant_id "
        "WHERE cu.user_id = ? AND cu.status = ? AND cu.deleted_at IS NULL ORDER BY cu.id DESC",
        {std::to_string(userId), status});
}

bool softDeleteClaim(long long claimId) {
    return Database::instance().execute(
               "UPDATE coupon_user SET deleted_at = ? WHERE id = ? AND deleted_at IS NULL",
               {timeutil::nowStr(), std::to_string(claimId)}) == 1;
}

bool softDeleteCoupon(long long couponId) {
    return Database::instance().execute(
               "UPDATE coupons SET deleted_at = ? WHERE id = ? AND deleted_at IS NULL",
               {timeutil::nowStr(), std::to_string(couponId)}) == 1;
}

nlohmann::json claimByUserAndCoupon(long long couponId, long long userId) {
    auto row = Database::instance().queryOne(
        "SELECT cu.*, c.*, m.name AS merchant_name FROM coupon_user cu "
        "JOIN coupons c ON c.id = cu.coupon_id JOIN merchants m ON m.id = c.merchant_id "
        "WHERE cu.coupon_id = ? AND cu.user_id = ?",
        {std::to_string(couponId), std::to_string(userId)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

}  // namespace CouponDao
