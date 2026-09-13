#include "dao/OrderDao.h"

#include "db/Database.h"
#include "model/Models.h"
#include "util/Time.h"

namespace OrderDao {

long long create(long long userId, long long packageId, double amount,
                 const std::string& orderNo) {
    auto row = Database::instance().queryOne(
        "INSERT INTO orders (order_no, user_id, package_id, amount, status, created_at) "
        "VALUES (?, ?, ?, ?, 'purchased', ?) RETURNING id",
        {orderNo, std::to_string(userId), std::to_string(packageId), std::to_string(amount),
         timeutil::nowStr()});
    return row.is_null() ? 0 : row.value("id", 0LL);
}

long long countPurchased(long long userId, long long packageId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM orders WHERE user_id = ? AND package_id = ? "
        "AND status != 'refunded'",
        {std::to_string(userId), std::to_string(packageId)});
    return row.is_null() ? 0 : row.value("c", 0LL);
}

nlohmann::json orderById(long long id) {
    auto row = Database::instance().queryOne(
        "SELECT o.*, p.name AS package_name, p.content, m.name AS merchant_name, "
        "m.id AS merchant_id FROM orders o "
        "JOIN packages p ON p.id = o.package_id "
        "JOIN merchants m ON m.id = p.merchant_id WHERE o.id = ?",
        {std::to_string(id)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

nlohmann::json listByUser(long long userId, const std::string& status, int page, int size,
                          long long& total) {
    auto& db = Database::instance();
    nlohmann::json rows;
    long long c = 0;
    if (status.empty() || status == "all") {
        auto tr = db.queryOne("SELECT COUNT(*) AS c FROM orders WHERE user_id = ?",
                              {std::to_string(userId)});
        c = tr.is_null() ? 0 : tr.value("c", 0LL);
        rows = db.query(
            "SELECT o.*, p.name AS package_name, p.content, m.name AS merchant_name, "
            "m.id AS merchant_id FROM orders o "
            "JOIN packages p ON p.id = o.package_id "
            "JOIN merchants m ON m.id = p.merchant_id "
            "WHERE o.user_id = ? ORDER BY o.id DESC LIMIT ? OFFSET ?",
            {std::to_string(userId), std::to_string(size), std::to_string((page - 1) * size)});
    } else {
        auto tr = db.queryOne("SELECT COUNT(*) AS c FROM orders WHERE user_id = ? AND status = ?",
                              {std::to_string(userId), status});
        c = tr.is_null() ? 0 : tr.value("c", 0LL);
        rows = db.query(
            "SELECT o.*, p.name AS package_name, p.content, m.name AS merchant_name, "
            "m.id AS merchant_id FROM orders o "
            "JOIN packages p ON p.id = o.package_id "
            "JOIN merchants m ON m.id = p.merchant_id "
            "WHERE o.user_id = ? AND o.status = ? ORDER BY o.id DESC LIMIT ? OFFSET ?",
            {std::to_string(userId), status, std::to_string(size),
             std::to_string((page - 1) * size)});
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

bool writeConsumption(long long userId, long long merchantId, long long packageId,
                      double amount) {
    std::string now = timeutil::nowStr();
    return Database::instance().execute(
               "INSERT INTO consumption_records (user_id, merchant_id, service_id, package_id, "
               "amount, consume_time, created_at) VALUES (?, ?, NULL, ?, ?, ?, ?)",
               {std::to_string(userId), std::to_string(merchantId), std::to_string(packageId),
                std::to_string(amount), now, now}) >= 0;
}

}  // namespace OrderDao
