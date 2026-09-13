#include "dao/StatDao.h"

#include <map>
#include <string>

#include "db/Database.h"
#include "model/Models.h"
#include "util/Time.h"

namespace StatDao {
namespace {

std::string todayStr() { return timeutil::nowStr().substr(0, 10); }

}  // namespace

nlohmann::json merchant(long long merchantId) {
    auto& db = Database::instance();

    auto m = db.queryOne(
        "SELECT m.*, c.name AS category_name, "
        "IFNULL((SELECT COUNT(*) FROM reviews r WHERE r.merchant_id = m.id AND r.status='visible' "
        " AND r.target_type='merchant'),0) AS review_count, "
        "IFNULL((SELECT ROUND(AVG(avg_score),1) FROM reviews r WHERE r.merchant_id = m.id AND "
        "r.status='visible' AND r.target_type='merchant'),0) AS avg_score, "
        "IFNULL((SELECT COUNT(*) FROM favorites f WHERE f.target_type='merchant' AND "
        "f.target_id=m.id),0) AS favorite_count, "
        "IFNULL((SELECT COUNT(*) FROM follows f WHERE f.merchant_id=m.id),0) AS follower_count, "
        "IFNULL((SELECT COUNT(*) FROM orders o JOIN packages p ON p.id=o.package_id WHERE "
        "p.merchant_id=m.id),0) AS order_count, "
        "IFNULL((SELECT ROUND(SUM(o.amount),2) FROM orders o JOIN packages p ON p.id=o.package_id "
        "WHERE p.merchant_id=m.id),0) AS order_amount, "
        "IFNULL((SELECT COUNT(*) FROM orders o JOIN packages p ON p.id=o.package_id WHERE "
        "p.merchant_id=m.id AND o.status='used'),0) AS order_used "
        "FROM merchants m WHERE m.id = ?",
        {std::to_string(merchantId)});
    nlohmann::json out = m.is_null() ? nlohmann::json::object() : m;

    auto csum = db.queryOne(
        "SELECT COUNT(*) AS total, IFNULL(SUM(received),0) AS received, "
        "IFNULL(SUM(used),0) AS used FROM coupons WHERE merchant_id = ?",
        {std::to_string(merchantId)});
    if (!csum.is_null()) out["coupons"] = csum;

    out["hot_services"] = db.query(
        "SELECT s.id, s.name, COUNT(*) AS times, ROUND(SUM(cr.amount),2) AS amount "
        "FROM consumption_records cr JOIN services s ON s.id = cr.service_id "
        "WHERE cr.merchant_id = ? GROUP BY s.id ORDER BY times DESC LIMIT 5",
        {std::to_string(merchantId)});

    // 近 7 日：每日消费/订单
    std::string since = timeutil::afterDaysStr(-6).substr(0, 10);
    std::map<std::string, nlohmann::json> day;
    auto cons = db.query(
        "SELECT substr(consume_time,1,10) AS d, COUNT(*) AS c, "
        "IFNULL(SUM(amount),0) AS amt FROM consumption_records "
        "WHERE merchant_id = ? AND consume_time >= ? GROUP BY d",
        {std::to_string(merchantId), since});
    for (auto& r : cons) {
        std::string d = jsonStr(r, "d");
        day[d]["date"] = d;
        day[d]["consumption"] = r.value("c", 0LL);
        day[d]["amount"] = r.value("amt", 0.0);
    }
    auto ord = db.query(
        "SELECT substr(o.created_at,1,10) AS d, COUNT(*) AS c FROM orders o "
        "JOIN packages p ON p.id = o.package_id "
        "WHERE p.merchant_id = ? AND o.created_at >= ? GROUP BY d",
        {std::to_string(merchantId), since});
    for (auto& r : ord) {
        std::string d = jsonStr(r, "d");
        day[d]["date"] = d;
        day[d]["orders"] = r.value("c", 0LL);
    }
    nlohmann::json days = nlohmann::json::array();
    for (auto& kv : day) days.push_back(kv.second);
    out["trend"] = days;
    return out;
}

nlohmann::json platform() {
    auto& db = Database::instance();
    nlohmann::json out;
    std::string today = todayStr();
    auto cnt = [&](const std::string& sql, const std::vector<std::string>& p = {}) {
        auto r = db.queryOne(sql, p);
        return r.is_null() ? 0 : r.value("c", 0LL);
    };

    out["merchants"] = {
        {"total", cnt("SELECT COUNT(*) AS c FROM merchants")},
        {"approved", cnt("SELECT COUNT(*) AS c FROM merchants WHERE status='approved'")},
        {"pending", cnt("SELECT COUNT(*) AS c FROM merchants WHERE status='pending'")},
        {"rejected", cnt("SELECT COUNT(*) AS c FROM merchants WHERE status='rejected'")}};
    out["users"] = {
        {"total", cnt("SELECT COUNT(*) AS c FROM users")},
        {"consumer", cnt("SELECT COUNT(*) AS c FROM users WHERE role='consumer'")},
        {"merchant", cnt("SELECT COUNT(*) AS c FROM users WHERE role='merchant'")},
        {"today_new", cnt("SELECT COUNT(*) AS c FROM users WHERE substr(created_at,1,10)=?",
                          {today})}};
    out["reviews"] = {
        {"visible", cnt("SELECT COUNT(*) AS c FROM reviews WHERE status='visible'")},
        {"pending_reports",
         cnt("SELECT COUNT(*) AS c FROM review_reports WHERE status='pending'") +
             cnt("SELECT COUNT(*) AS c FROM comment_reports WHERE status='pending'")}};
    auto os = db.queryOne(
        "SELECT COUNT(*) AS total, IFNULL(ROUND(SUM(amount),2),0) AS amount, "
        "IFNULL(SUM(status='used'),0) AS used FROM orders");
    out["orders"] = os.is_null()
                        ? nlohmann::json{{"total", 0}, {"amount", 0.0}, {"used", 0}}
                        : nlohmann::json{{"total", os.value("total", 0LL)},
                                         {"amount", os.value("amount", 0.0)},
                                         {"used", os.value("used", 0LL)}};
    auto cs = db.queryOne(
        "SELECT COUNT(*) AS total, IFNULL(ROUND(SUM(amount),2),0) AS amount FROM "
        "consumption_records");
    out["consumption"] = cs.is_null()
                             ? nlohmann::json{{"total", 0}, {"amount", 0.0}}
                             : nlohmann::json{{"total", cs.value("total", 0LL)},
                                              {"amount", cs.value("amount", 0.0)}};
    out["coupons"] = {{"claims", cnt("SELECT COUNT(*) AS c FROM coupon_user")},
                      {"used", cnt("SELECT COUNT(*) AS c FROM coupon_user WHERE status='used'")}};
    out["category_dist"] = db.query(
        "SELECT c.id, c.name, COUNT(m.id) AS cnt FROM categories c "
        "LEFT JOIN merchants m ON m.category_id = c.id AND m.status='approved' "
        "GROUP BY c.id ORDER BY cnt DESC, c.sort");
    out["trend"] = []() {
        auto& d = Database::instance();
        std::string since = timeutil::afterDaysStr(-6).substr(0, 10);
        std::map<std::string, nlohmann::json> day;
        auto ur = d.query(
            "SELECT substr(created_at,1,10) AS dd, COUNT(*) AS c FROM users "
            "WHERE created_at >= ? GROUP BY dd", {since});
        for (auto& r : ur) { std::string x = jsonStr(r, "dd"); day[x]["new_users"] = r.value("c", 0LL); }
        auto ors = d.query(
            "SELECT substr(created_at,1,10) AS dd, COUNT(*) AS c, IFNULL(SUM(amount),0) AS amt "
            "FROM orders WHERE created_at >= ? GROUP BY dd", {since});
        for (auto& r : ors) {
            std::string x = jsonStr(r, "dd");
            day[x]["orders"] = r.value("c", 0LL);
            day[x]["amount"] = r.value("amt", 0.0);
        }
        auto crs = d.query(
            "SELECT substr(consume_time,1,10) AS dd, IFNULL(SUM(amount),0) AS amt "
            "FROM consumption_records WHERE consume_time >= ? GROUP BY dd", {since});
        for (auto& r : crs) day[jsonStr(r, "dd")]["consume"] = r.value("amt", 0.0);
        nlohmann::json arr = nlohmann::json::array();
        for (auto& kv : day) { auto e = kv.second; e["date"] = kv.first; arr.push_back(e); }
        return arr;
    }();
    return out;
}

}  // namespace StatDao
