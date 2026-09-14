#include "dao/OrderDao.h"

#include "db/Database.h"
#include "model/Models.h"
#include "util/Time.h"

namespace OrderDao {

namespace {

// 订单公共查询：门店名 + 项目名（服务/套餐二选一）+ 商户归属
const char* kOrderSelect =
    "SELECT o.*, st.name AS store_name, st.area AS store_area, "
    "IFNULL(p.merchant_id, s.merchant_id) AS merchant_id, "
    "CASE o.item_type WHEN 'package' THEN p.name ELSE s.name END AS item_name, "
    "p.content AS package_content, p.valid_days AS package_valid_days, "
    "s.applicable_time AS service_applicable_time, s.price_unit AS service_price_unit "
    "FROM orders o "
    "LEFT JOIN stores st ON st.id = o.store_id "
    "LEFT JOIN packages p ON p.id = o.package_id "
    "LEFT JOIN services s ON s.id = o.service_id ";

}  // namespace

long long create(long long userId, long long storeId, const std::string& itemType, long long itemId,
                 double amount, const std::string& orderNo) {
    auto& db = Database::instance();
    nlohmann::json row;
    if (itemType == "service") {
        row = db.queryOne(
            "INSERT INTO orders (order_no, user_id, store_id, item_type, package_id, service_id, "
            "amount, status, created_at) "
            "VALUES (?, ?, ?, 'service', NULL, ?, ?, 'purchased', ?) RETURNING id",
            {orderNo, std::to_string(userId), std::to_string(storeId), std::to_string(itemId),
             std::to_string(amount), timeutil::nowStr()});
    } else {
        row = db.queryOne(
            "INSERT INTO orders (order_no, user_id, store_id, item_type, package_id, service_id, "
            "amount, status, created_at) "
            "VALUES (?, ?, ?, 'package', ?, NULL, ?, 'purchased', ?) RETURNING id",
            {orderNo, std::to_string(userId), std::to_string(storeId), std::to_string(itemId),
             std::to_string(amount), timeutil::nowStr()});
    }
    return row.is_null() ? 0 : row.value("id", 0LL);
}

long long countPurchased(long long userId, const std::string& itemType, long long itemId) {
    auto& db = Database::instance();
    auto row = (itemType == "service")
                   ? db.queryOne("SELECT COUNT(*) AS c FROM orders WHERE user_id = ? "
                                 "AND item_type = 'service' AND service_id = ? "
                                 "AND status != 'refunded'",
                                 {std::to_string(userId), std::to_string(itemId)})
                   : db.queryOne("SELECT COUNT(*) AS c FROM orders WHERE user_id = ? "
                                 "AND item_type = 'package' AND package_id = ? "
                                 "AND status != 'refunded'",
                                 {std::to_string(userId), std::to_string(itemId)});
    return row.is_null() ? 0 : row.value("c", 0LL);
}

nlohmann::json orderById(long long id) {
    auto row = Database::instance().queryOne(std::string(kOrderSelect) + "WHERE o.id = ?",
                                             {std::to_string(id)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

nlohmann::json listByUser(long long userId, const std::string& status, int page, int size,
                          long long& total) {
    auto& db = Database::instance();
    nlohmann::json rows;
    long long c = 0;
    const std::string lim = std::to_string(size);
    const std::string off = std::to_string((page - 1) * size);
    if (status.empty() || status == "all") {
        auto tr = db.queryOne("SELECT COUNT(*) AS c FROM orders WHERE user_id = ?",
                              {std::to_string(userId)});
        c = tr.is_null() ? 0 : tr.value("c", 0LL);
        rows = db.query(std::string(kOrderSelect) +
                            "WHERE o.user_id = ? ORDER BY o.id DESC LIMIT ? OFFSET ?",
                        {std::to_string(userId), lim, off});
    } else {
        auto tr = db.queryOne("SELECT COUNT(*) AS c FROM orders WHERE user_id = ? AND status = ?",
                              {std::to_string(userId), status});
        c = tr.is_null() ? 0 : tr.value("c", 0LL);
        rows = db.query(
            std::string(kOrderSelect) +
                "WHERE o.user_id = ? AND o.status = ? ORDER BY o.id DESC LIMIT ? OFFSET ?",
            {std::to_string(userId), status, lim, off});
    }
    total = c;
    return rows;
}

bool markUsed(long long orderId, long long userId) {
    return Database::instance().execute(
               "UPDATE orders SET status = 'used', used_time = ? WHERE id = ? AND user_id = ? "
               "AND status = 'purchased'",
               {timeutil::nowStr(), std::to_string(orderId), std::to_string(userId)}) == 1;
}

bool markRefunded(long long orderId, long long userId) {
    return Database::instance().execute(
               "UPDATE orders SET status = 'refunded' WHERE id = ? AND user_id = ? "
               "AND status = 'purchased'",
               {std::to_string(orderId), std::to_string(userId)}) == 1;
}

bool writeConsumption(long long userId, long long merchantId, long long storeId,
                      long long serviceId, long long packageId, double amount) {
    auto& db = Database::instance();
    std::string now = timeutil::nowStr();
    if (serviceId > 0) {
        return db.execute(
                   "INSERT INTO consumption_records (user_id, merchant_id, store_id, service_id, "
                   "package_id, amount, consume_time, created_at) VALUES (?, ?, ?, ?, NULL, ?, ?, ?)",
                   {std::to_string(userId), std::to_string(merchantId), std::to_string(storeId),
                    std::to_string(serviceId), std::to_string(amount), now, now}) >= 0;
    }
    return db.execute(
               "INSERT INTO consumption_records (user_id, merchant_id, store_id, service_id, "
               "package_id, amount, consume_time, created_at) VALUES (?, ?, ?, NULL, ?, ?, ?, ?)",
               {std::to_string(userId), std::to_string(merchantId), std::to_string(storeId),
                std::to_string(packageId), std::to_string(amount), now, now}) >= 0;
}

}  // namespace OrderDao
