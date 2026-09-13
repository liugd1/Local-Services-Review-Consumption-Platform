#include "dao/MerchantDao.h"

#include "db/Database.h"
#include "model/Models.h"
#include "util/Time.h"

namespace MerchantDao {

// ---------------- 商户 ----------------
long long create(long long userId, const nlohmann::json& m) {
    auto& db = Database::instance();
    std::string cat = jsonStr(m, "category_id");
    std::string now = timeutil::nowStr();
    nlohmann::json row;
    if (cat.empty()) {
        row = db.queryOne(
            "INSERT INTO merchants (user_id, name, category_id, area, business_hours, phone, "
            "intro, price_min, price_max, logo, images, status, reject_reason, view_count, "
            "created_at) VALUES (?, ?, NULL, ?, ?, ?, ?, ?, ?, ?, ?, 'pending', '', 0, ?) "
            "RETURNING id",
            {std::to_string(userId), jsonStr(m, "name"), jsonStr(m, "area"),
             jsonStr(m, "business_hours"), jsonStr(m, "phone"), jsonStr(m, "intro"),
             std::to_string(m.value("price_min", 0.0)), std::to_string(m.value("price_max", 0.0)),
             jsonStr(m, "logo"), jsonStr(m, "images"), now});
    } else {
        row = db.queryOne(
            "INSERT INTO merchants (user_id, name, category_id, area, business_hours, phone, "
            "intro, price_min, price_max, logo, images, status, reject_reason, view_count, "
            "created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'pending', '', 0, ?) "
            "RETURNING id",
            {std::to_string(userId), jsonStr(m, "name"), cat, jsonStr(m, "area"),
             jsonStr(m, "business_hours"), jsonStr(m, "phone"), jsonStr(m, "intro"),
             std::to_string(m.value("price_min", 0.0)), std::to_string(m.value("price_max", 0.0)),
             jsonStr(m, "logo"), jsonStr(m, "images"), now});
    }
    return row.is_null() ? 0 : row.value("id", 0LL);
}

nlohmann::json byId(long long id) {
    auto row = Database::instance().queryOne(
        "SELECT m.*, c.name AS category_name FROM merchants m "
        "LEFT JOIN categories c ON c.id = m.category_id WHERE m.id = ?",
        {std::to_string(id)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

nlohmann::json byOwner(long long userId) {
    auto row = Database::instance().queryOne(
        "SELECT m.*, c.name AS category_name FROM merchants m "
        "LEFT JOIN categories c ON c.id = m.category_id "
        "WHERE m.user_id = ? ORDER BY m.id DESC LIMIT 1",
        {std::to_string(userId)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

void updateProfile(long long id, const nlohmann::json& full) {
    auto& db = Database::instance();
    std::string cat = jsonStr(full, "category_id");
    if (cat.empty()) {
        db.execute(
            "UPDATE merchants SET name = ?, category_id = NULL, area = ?, business_hours = ?, "
            "phone = ?, intro = ?, price_min = ?, price_max = ?, logo = ?, images = ? "
            "WHERE id = ?",
            {jsonStr(full, "name"), jsonStr(full, "area"), jsonStr(full, "business_hours"),
             jsonStr(full, "phone"), jsonStr(full, "intro"),
             std::to_string(full.value("price_min", 0.0)),
             std::to_string(full.value("price_max", 0.0)), jsonStr(full, "logo"),
             jsonStr(full, "images"), std::to_string(id)});
    } else {
        db.execute(
            "UPDATE merchants SET name = ?, category_id = ?, area = ?, business_hours = ?, "
            "phone = ?, intro = ?, price_min = ?, price_max = ?, logo = ?, images = ? "
            "WHERE id = ?",
            {jsonStr(full, "name"), cat, jsonStr(full, "area"), jsonStr(full, "business_hours"),
             jsonStr(full, "phone"), jsonStr(full, "intro"),
             std::to_string(full.value("price_min", 0.0)),
             std::to_string(full.value("price_max", 0.0)), jsonStr(full, "logo"),
             jsonStr(full, "images"), std::to_string(id)});
    }
}

bool setStatus(long long id, const std::string& status, const std::string& reason) {
    return Database::instance().execute(
               "UPDATE merchants SET status = ?, reject_reason = ? WHERE id = ?",
               {status, reason, std::to_string(id)}) >= 0;
}

void incViewCount(long long id) {
    Database::instance().execute("UPDATE merchants SET view_count = view_count + 1 WHERE id = ?",
                                 {std::to_string(id)});
}

nlohmann::json adminList(const std::string& status, const std::string& keyword, int page,
                         int size, long long& total) {
    auto& db = Database::instance();
    std::string where = " WHERE 1=1 ";
    std::vector<std::string> params;
    if (!status.empty()) {
        where += " AND m.status = ? ";
        params.push_back(status);
    }
    if (!keyword.empty()) {
        where += " AND (m.name LIKE ? OR m.phone LIKE ? OR m.intro LIKE ?) ";
        auto kw = "%" + keyword + "%";
        params.push_back(kw);
        params.push_back(kw);
        params.push_back(kw);
    }
    auto totalRow = db.queryOne("SELECT COUNT(*) AS c FROM merchants m" + where, params);
    total = totalRow.is_null() ? 0 : totalRow.value("c", 0LL);

    std::vector<std::string> pageParams = params;
    pageParams.push_back(std::to_string(size));
    pageParams.push_back(std::to_string((page - 1) * size));
    return db.query(
        "SELECT m.*, c.name AS category_name, u.username AS owner_username FROM merchants m "
        "LEFT JOIN categories c ON c.id = m.category_id "
        "LEFT JOIN users u ON u.id = m.user_id" +
            where + " ORDER BY m.id DESC LIMIT ? OFFSET ?",
        pageParams);
}

nlohmann::json countByStatus() {
    auto rows = Database::instance().query(
        "SELECT status, COUNT(*) AS cnt FROM merchants GROUP BY status");
    return rows;
}

// ---------------- 门店 ----------------
nlohmann::json storeById(long long id) {
    auto row = Database::instance().queryOne("SELECT * FROM stores WHERE id = ?",
                                             {std::to_string(id)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

nlohmann::json listStores(long long merchantId) {
    return Database::instance().query(
        "SELECT * FROM stores WHERE merchant_id = ? ORDER BY id", {std::to_string(merchantId)});
}

long long addStore(long long merchantId, const nlohmann::json& s) {
    auto row = Database::instance().queryOne(
        "INSERT INTO stores (merchant_id, name, address, area, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?) RETURNING id",
        {std::to_string(merchantId), jsonStr(s, "name"), jsonStr(s, "address"),
         jsonStr(s, "area"), jsonStr(s, "status", "open"), timeutil::nowStr()});
    return row.is_null() ? 0 : row.value("id", 0LL);
}

void updateStore(long long id, const nlohmann::json& s) {
    Database::instance().execute(
        "UPDATE stores SET name = ?, address = ?, area = ?, status = ? WHERE id = ?",
        {jsonStr(s, "name"), jsonStr(s, "address"), jsonStr(s, "area"), jsonStr(s, "status"),
         std::to_string(id)});
}

bool removeStore(long long id) {
    auto& db = Database::instance();
    // 先解除服务对门店的引用，避免外键冲突
    db.execute("UPDATE services SET store_id = NULL WHERE store_id = ?", {std::to_string(id)});
    return db.execute("DELETE FROM stores WHERE id = ?", {std::to_string(id)}) >= 0;
}

// ---------------- 服务项目 ----------------
nlohmann::json listServices(long long merchantId, const std::string& status) {
    auto& db = Database::instance();
    if (status.empty()) {
        return db.query(
            "SELECT s.*, st.name AS store_name FROM services s "
            "LEFT JOIN stores st ON st.id = s.store_id WHERE s.merchant_id = ? ORDER BY s.id DESC",
            {std::to_string(merchantId)});
    }
    return db.query(
        "SELECT s.*, st.name AS store_name FROM services s "
        "LEFT JOIN stores st ON st.id = s.store_id "
        "WHERE s.merchant_id = ? AND s.status = ? ORDER BY s.id DESC",
        {std::to_string(merchantId), status});
}

long long addService(long long merchantId, const nlohmann::json& s) {
    auto& db = Database::instance();
    std::string storeId = jsonStr(s, "store_id");
    nlohmann::json row;
    if (storeId.empty()) {
        row = db.queryOne(
            "INSERT INTO services (merchant_id, store_id, name, price, price_unit, "
            "applicable_time, stock, limit_count, status, created_at) "
            "VALUES (?, NULL, ?, ?, ?, ?, ?, ?, ?, ?) RETURNING id",
            {std::to_string(merchantId), jsonStr(s, "name"),
             std::to_string(s.value("price", 0.0)), jsonStr(s, "price_unit"),
             jsonStr(s, "applicable_time"), std::to_string(s.value("stock", -1)),
             std::to_string(s.value("limit_count", 0)), jsonStr(s, "status", "on"),
             timeutil::nowStr()});
    } else {
        row = db.queryOne(
            "INSERT INTO services (merchant_id, store_id, name, price, price_unit, "
            "applicable_time, stock, limit_count, status, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?) RETURNING id",
            {std::to_string(merchantId), storeId, jsonStr(s, "name"),
             std::to_string(s.value("price", 0.0)), jsonStr(s, "price_unit"),
             jsonStr(s, "applicable_time"), std::to_string(s.value("stock", -1)),
             std::to_string(s.value("limit_count", 0)), jsonStr(s, "status", "on"),
             timeutil::nowStr()});
    }
    return row.is_null() ? 0 : row.value("id", 0LL);
}

void updateService(long long id, const nlohmann::json& s) {
    auto& db = Database::instance();
    std::string storeId = jsonStr(s, "store_id");
    if (storeId.empty()) {
        db.execute(
            "UPDATE services SET store_id = NULL, name = ?, price = ?, price_unit = ?, "
            "applicable_time = ?, stock = ?, limit_count = ?, status = ? WHERE id = ?",
            {jsonStr(s, "name"), std::to_string(s.value("price", 0.0)),
             jsonStr(s, "price_unit"), jsonStr(s, "applicable_time"),
             std::to_string(s.value("stock", -1)), std::to_string(s.value("limit_count", 0)),
             jsonStr(s, "status", "on"), std::to_string(id)});
    } else {
        db.execute(
            "UPDATE services SET store_id = ?, name = ?, price = ?, price_unit = ?, "
            "applicable_time = ?, stock = ?, limit_count = ?, status = ? WHERE id = ?",
            {storeId, jsonStr(s, "name"), std::to_string(s.value("price", 0.0)),
             jsonStr(s, "price_unit"), jsonStr(s, "applicable_time"),
             std::to_string(s.value("stock", -1)), std::to_string(s.value("limit_count", 0)),
             jsonStr(s, "status", "on"), std::to_string(id)});
    }
}

bool updateServiceStatus(long long id, const std::string& status) {
    return Database::instance().execute("UPDATE services SET status = ? WHERE id = ?",
                                        {status, std::to_string(id)}) >= 0;
}

nlohmann::json serviceById(long long id) {
    auto row = Database::instance().queryOne(
        "SELECT s.*, st.name AS store_name FROM services s "
        "LEFT JOIN stores st ON st.id = s.store_id WHERE s.id = ?",
        {std::to_string(id)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

// ---------------- 消费套餐 ----------------
nlohmann::json listPackages(long long merchantId, const std::string& status) {
    auto& db = Database::instance();
    if (status.empty()) {
        return db.query("SELECT * FROM packages WHERE merchant_id = ? ORDER BY id DESC",
                        {std::to_string(merchantId)});
    }
    return db.query("SELECT * FROM packages WHERE merchant_id = ? AND status = ? ORDER BY id DESC",
                    {std::to_string(merchantId), status});
}

long long addPackage(long long merchantId, const nlohmann::json& p) {
    auto row = Database::instance().queryOne(
        "INSERT INTO packages (merchant_id, name, content, price, valid_days, limit_count, "
        "status, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?) RETURNING id",
        {std::to_string(merchantId), jsonStr(p, "name"), jsonStr(p, "content"),
         std::to_string(p.value("price", 0.0)), std::to_string(p.value("valid_days", 30)),
         std::to_string(p.value("limit_count", 0)), jsonStr(p, "status", "on"),
         timeutil::nowStr()});
    return row.is_null() ? 0 : row.value("id", 0LL);
}

void updatePackage(long long id, const nlohmann::json& p) {
    Database::instance().execute(
        "UPDATE packages SET name = ?, content = ?, price = ?, valid_days = ?, limit_count = ?, "
        "status = ? WHERE id = ?",
        {jsonStr(p, "name"), jsonStr(p, "content"), std::to_string(p.value("price", 0.0)),
         std::to_string(p.value("valid_days", 30)), std::to_string(p.value("limit_count", 0)),
         jsonStr(p, "status", "on"), std::to_string(id)});
}

bool updatePackageStatus(long long id, const std::string& status) {
    return Database::instance().execute("UPDATE packages SET status = ? WHERE id = ?",
                                        {status, std::to_string(id)}) >= 0;
}

nlohmann::json packageById(long long id) {
    auto row = Database::instance().queryOne("SELECT * FROM packages WHERE id = ?",
                                             {std::to_string(id)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

}  // namespace MerchantDao

