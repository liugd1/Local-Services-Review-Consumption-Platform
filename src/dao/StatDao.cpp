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

    // 订单归属商户：套餐订单(package_id) / 服务订单(service_id) / 门店归属(stores.merchant_id)。
    // 三种形态都要计入经营统计，只 JOIN packages 会漏掉服务订单。
    const std::string orderFrom =
        "FROM orders o "
        "LEFT JOIN packages p ON p.id = o.package_id "
        "LEFT JOIN services s ON s.id = o.service_id "
        "LEFT JOIN stores st ON st.id = o.store_id "
        "WHERE IFNULL(p.merchant_id, IFNULL(s.merchant_id, st.merchant_id)) = m.id "
        "AND o.deleted_at IS NULL ";

    // 统计口径：订单数 = 用户下单量；成交额 = 未退款订单实付金额合计；核销量 = 已使用订单数
    auto m = db.queryOne(
        "SELECT m.*, c.name AS category_name, "
        "IFNULL((SELECT COUNT(*) FROM reviews r WHERE r.merchant_id = m.id AND r.status='visible' "
        " AND r.target_type='merchant'),0) AS review_count, "
        "IFNULL((SELECT ROUND(AVG(r.avg_score),1) FROM reviews r WHERE r.merchant_id = m.id AND "
        "r.status='visible' AND r.target_type='merchant'),0) AS avg_score, "
        "IFNULL((SELECT COUNT(*) FROM favorites f WHERE f.target_type='merchant' AND "
        "f.target_id=m.id),0) AS favorite_count, "
        "IFNULL((SELECT COUNT(*) FROM follows f WHERE f.merchant_id=m.id),0) AS follower_count, "
        "IFNULL((SELECT COUNT(*) " + orderFrom + "),0) AS order_count, "
        "IFNULL((SELECT ROUND(SUM(o.amount),2) " + orderFrom + " AND o.status <> 'refunded'),0) "
        "AS order_amount, "
        "IFNULL((SELECT COUNT(*) " + orderFrom + " AND o.status = 'used'),0) AS order_used, "
        "IFNULL((SELECT COUNT(*) " + orderFrom + " AND o.status = 'refunded'),0) AS order_refunded "
        "FROM merchants m LEFT JOIN categories c ON c.id = m.category_id WHERE m.id = ?",
        {std::to_string(merchantId)});
    nlohmann::json out = m.is_null() ? nlohmann::json::object() : m;

    auto csum = db.queryOne(
        "SELECT COUNT(*) AS total, IFNULL(SUM(received),0) AS received, "
        "IFNULL(SUM(used),0) AS used FROM coupons WHERE merchant_id = ?",
        {std::to_string(merchantId)});
    if (!csum.is_null()) out["coupons"] = csum;

    // 热门项目：服务消费 + 套餐消费合并排名（套餐是服务组合，二者都应计入「热门服务」）
    out["hot_services"] = db.query(
        "SELECT * FROM ("
        "  SELECT 'service' AS item_type, s.id AS id, s.name AS name, "
        "         COUNT(*) AS times, ROUND(SUM(cr.amount),2) AS amount "
        "  FROM consumption_records cr JOIN services s ON s.id = cr.service_id "
        "  WHERE cr.merchant_id = ? AND cr.deleted_at IS NULL GROUP BY s.id "
        "  UNION ALL "
        "  SELECT 'package' AS item_type, p.id AS id, p.name AS name, "
        "         COUNT(*) AS times, ROUND(SUM(cr.amount),2) AS amount "
        "  FROM consumption_records cr JOIN packages p ON p.id = cr.package_id "
        "  WHERE cr.merchant_id = ? AND cr.deleted_at IS NULL GROUP BY p.id"
        ") ORDER BY times DESC, amount DESC LIMIT 5",
        {std::to_string(merchantId), std::to_string(merchantId)});

    // 近 7 日：每日消费/订单（无数据日期补 0，保证趋势图连续、口径完整）
    const int kDays = 7;
    std::string since = timeutil::afterDaysStr(-(kDays - 1)).substr(0, 10);
    std::map<std::string, nlohmann::json> day;
    for (int i = kDays - 1; i >= 0; --i) {
        std::string d = timeutil::afterDaysStr(-i).substr(0, 10);
        day[d] = {{"date", d}, {"consumption", 0}, {"amount", 0.0}, {"orders", 0}};
    }
    auto cons = db.query(
        "SELECT substr(consume_time,1,10) AS d, COUNT(*) AS c, "
        "IFNULL(SUM(amount),0) AS amt FROM consumption_records "
        "WHERE merchant_id = ? AND consume_time >= ? AND deleted_at IS NULL GROUP BY d",
        {std::to_string(merchantId), since});
    for (auto& r : cons) {
        std::string d = jsonStr(r, "d");
        if (!day.count(d)) continue;
        day[d]["consumption"] = r.value("c", 0LL);
        day[d]["amount"] = r.value("amt", 0.0);
    }
    auto ord = db.query(
        "SELECT substr(o.created_at,1,10) AS d, COUNT(*) AS c "
        "FROM orders o "
        "LEFT JOIN packages p ON p.id = o.package_id "
        "LEFT JOIN services s ON s.id = o.service_id "
        "LEFT JOIN stores st ON st.id = o.store_id "
        "WHERE IFNULL(p.merchant_id, IFNULL(s.merchant_id, st.merchant_id)) = CAST(? AS INTEGER) "
        "AND o.deleted_at IS NULL AND o.created_at >= ? GROUP BY d",
        {std::to_string(merchantId), since});
    for (auto& r : ord) {
        std::string d = jsonStr(r, "d");
        if (!day.count(d)) continue;
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
    // 用户活跃度：近 N 日有下单 / 评价 / 消费 / 浏览行为的去重用户数
    auto activeUsers = [&](int days) {
        std::string since = timeutil::afterDaysStr(-(days - 1)).substr(0, 10);
        auto r = db.queryOne(
            "SELECT COUNT(*) AS c FROM ("
            "  SELECT user_id FROM orders WHERE created_at >= ? "
            "  UNION SELECT user_id FROM reviews WHERE created_at >= ? "
            "  UNION SELECT user_id FROM consumption_records WHERE consume_time >= ? "
            "  UNION SELECT user_id FROM browse_history WHERE viewed_at >= ?"
            ")",
            {since, since, since, since});
        return r.is_null() ? 0LL : r.value("c", 0LL);
    };
    out["users"] = {
        {"total", cnt("SELECT COUNT(*) AS c FROM users")},
        {"consumer", cnt("SELECT COUNT(*) AS c FROM users WHERE role='consumer'")},
        {"merchant", cnt("SELECT COUNT(*) AS c FROM users WHERE role='merchant'")},
        {"today_new", cnt("SELECT COUNT(*) AS c FROM users WHERE substr(created_at,1,10)=?",
                          {today})},
        {"active_7d", activeUsers(7)},
        {"active_30d", activeUsers(30)}};
    out["reviews"] = {
        {"visible", cnt("SELECT COUNT(*) AS c FROM reviews WHERE status='visible'")},
        {"pending_reports",
         cnt("SELECT COUNT(*) AS c FROM review_reports WHERE status='pending'") +
             cnt("SELECT COUNT(*) AS c FROM comment_reports WHERE status='pending'")}};
    // 平台订单口径：总量含退款订单；成交额只累计未退款订单；另单列退款量
    auto os = db.queryOne(
        "SELECT COUNT(*) AS total, "
        "IFNULL(ROUND(SUM(CASE WHEN status='refunded' THEN 0 ELSE amount END),2),0) AS amount, "
        "IFNULL(SUM(status='used'),0) AS used, "
        "IFNULL(SUM(status='refunded'),0) AS refunded FROM orders");
    out["orders"] = os.is_null()
                        ? nlohmann::json{
                              {"total", 0}, {"amount", 0.0}, {"used", 0}, {"refunded", 0}}
                        : nlohmann::json{{"total", os.value("total", 0LL)},
                                         {"amount", os.value("amount", 0.0)},
                                         {"used", os.value("used", 0LL)},
                                         {"refunded", os.value("refunded", 0LL)}};
    // 消费流水口径：排除用户已删除的软删除记录
    auto cs = db.queryOne(
        "SELECT COUNT(*) AS total, IFNULL(ROUND(SUM(amount),2),0) AS amount FROM "
        "consumption_records WHERE deleted_at IS NULL");
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
        const int kDays = 7;
        std::string since = timeutil::afterDaysStr(-(kDays - 1)).substr(0, 10);
        std::map<std::string, nlohmann::json> day;
        for (int i = kDays - 1; i >= 0; --i) {
            std::string x = timeutil::afterDaysStr(-i).substr(0, 10);
            day[x] = {{"date", x}, {"new_users", 0}, {"orders", 0}, {"amount", 0.0},
                      {"consume", 0.0}};
        }
        auto ur = d.query(
            "SELECT substr(created_at,1,10) AS dd, COUNT(*) AS c FROM users "
            "WHERE created_at >= ? GROUP BY dd", {since});
        for (auto& r : ur) {
            std::string x = jsonStr(r, "dd");
            if (day.count(x)) day[x]["new_users"] = r.value("c", 0LL);
        }
        auto ors = d.query(
            "SELECT substr(created_at,1,10) AS dd, COUNT(*) AS c, "
            "IFNULL(SUM(CASE WHEN status='refunded' THEN 0 ELSE amount END),0) AS amt "
            "FROM orders WHERE created_at >= ? GROUP BY dd", {since});
        for (auto& r : ors) {
            std::string x = jsonStr(r, "dd");
            if (!day.count(x)) continue;
            day[x]["orders"] = r.value("c", 0LL);
            day[x]["amount"] = r.value("amt", 0.0);
        }
        auto crs = d.query(
            "SELECT substr(consume_time,1,10) AS dd, IFNULL(SUM(amount),0) AS amt "
            "FROM consumption_records WHERE consume_time >= ? AND deleted_at IS NULL "
            "GROUP BY dd", {since});
        for (auto& r : crs) {
            std::string x = jsonStr(r, "dd");
            if (day.count(x)) day[x]["consume"] = r.value("amt", 0.0);
        }
        nlohmann::json arr = nlohmann::json::array();
        for (auto& kv : day) arr.push_back(kv.second);
        return arr;
    }();
    return out;
}

}  // namespace StatDao
