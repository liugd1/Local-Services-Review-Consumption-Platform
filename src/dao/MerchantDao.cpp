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
        "INSERT INTO stores (merchant_id, name, address, area, images, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?) RETURNING id",
        {std::to_string(merchantId), jsonStr(s, "name"), jsonStr(s, "address"),
         jsonStr(s, "area"), jsonArrayToCsv(s, "images"), jsonStr(s, "status", "open"),
         timeutil::nowStr()});
    long long id = row.is_null() ? 0 : row.value("id", 0LL);
    // 新门店默认上架该商户现有的服务 / 套餐 / 活动（可随后按门店下架）
    if (id > 0) fanoutStoreItems(merchantId, id);
    return id;
}

void updateStore(long long id, const nlohmann::json& s) {
    Database::instance().execute(
        "UPDATE stores SET name = ?, address = ?, area = ?, images = ?, status = ? WHERE id = ?",
        {jsonStr(s, "name"), jsonStr(s, "address"), jsonStr(s, "area"),
         jsonArrayToCsv(s, "images"), jsonStr(s, "status"), std::to_string(id)});
}

bool removeStore(long long id) {
    auto& db = Database::instance();
    // 先解除服务对门店的引用，避免外键冲突
    db.execute("UPDATE services SET store_id = NULL WHERE store_id = ?", {std::to_string(id)});
    // 清理门店级上架关系
    db.execute("DELETE FROM store_services WHERE store_id = ?", {std::to_string(id)});
    db.execute("DELETE FROM store_packages WHERE store_id = ?", {std::to_string(id)});
    db.execute("DELETE FROM store_coupons WHERE store_id = ?", {std::to_string(id)});
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
            "applicable_time, stock, limit_count, images, status, created_at) "
            "VALUES (?, NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?) RETURNING id",
            {std::to_string(merchantId), jsonStr(s, "name"),
             std::to_string(s.value("price", 0.0)), jsonStr(s, "price_unit"),
             jsonStr(s, "applicable_time"), std::to_string(s.value("stock", -1)),
             std::to_string(s.value("limit_count", 0)), jsonArrayToCsv(s, "images"),
             jsonStr(s, "status", "on"), timeutil::nowStr()});
    } else {
        row = db.queryOne(
            "INSERT INTO services (merchant_id, store_id, name, price, price_unit, "
            "applicable_time, stock, limit_count, images, status, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) RETURNING id",
            {std::to_string(merchantId), storeId, jsonStr(s, "name"),
             std::to_string(s.value("price", 0.0)), jsonStr(s, "price_unit"),
             jsonStr(s, "applicable_time"), std::to_string(s.value("stock", -1)),
             std::to_string(s.value("limit_count", 0)), jsonArrayToCsv(s, "images"),
             jsonStr(s, "status", "on"), timeutil::nowStr()});
    }
    long long id = row.is_null() ? 0 : row.value("id", 0LL);
    // 新服务默认上架到该商户所有门店（可在「门店管理」按门店下架）
    if (id > 0) fanoutItemToStores(merchantId, "service", id);
    return id;
}

void updateService(long long id, const nlohmann::json& s) {
    auto& db = Database::instance();
    std::string storeId = jsonStr(s, "store_id");
    if (storeId.empty()) {
        db.execute(
            "UPDATE services SET store_id = NULL, name = ?, price = ?, price_unit = ?, "
            "applicable_time = ?, stock = ?, limit_count = ?, images = ?, status = ? WHERE id = ?",
            {jsonStr(s, "name"), std::to_string(s.value("price", 0.0)),
             jsonStr(s, "price_unit"), jsonStr(s, "applicable_time"),
             std::to_string(s.value("stock", -1)), std::to_string(s.value("limit_count", 0)),
             jsonArrayToCsv(s, "images"), jsonStr(s, "status", "on"), std::to_string(id)});
    } else {
        db.execute(
            "UPDATE services SET store_id = ?, name = ?, price = ?, price_unit = ?, "
            "applicable_time = ?, stock = ?, limit_count = ?, images = ?, status = ? WHERE id = ?",
            {storeId, jsonStr(s, "name"), std::to_string(s.value("price", 0.0)),
             jsonStr(s, "price_unit"), jsonStr(s, "applicable_time"),
             std::to_string(s.value("stock", -1)), std::to_string(s.value("limit_count", 0)),
             jsonArrayToCsv(s, "images"), jsonStr(s, "status", "on"), std::to_string(id)});
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
        "images, status, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) RETURNING id",
        {std::to_string(merchantId), jsonStr(p, "name"), jsonStr(p, "content"),
         std::to_string(p.value("price", 0.0)), std::to_string(p.value("valid_days", 30)),
         std::to_string(p.value("limit_count", 0)), jsonArrayToCsv(p, "images"),
         jsonStr(p, "status", "on"), timeutil::nowStr()});
    long long id = row.is_null() ? 0 : row.value("id", 0LL);
    // 新套餐默认上架到该商户所有门店（可在「门店管理」按门店下架）
    if (id > 0) fanoutItemToStores(merchantId, "package", id);
    return id;
}

void updatePackage(long long id, const nlohmann::json& p) {
    Database::instance().execute(
        "UPDATE packages SET name = ?, content = ?, price = ?, valid_days = ?, limit_count = ?, "
        "images = ?, status = ? WHERE id = ?",
        {jsonStr(p, "name"), jsonStr(p, "content"), std::to_string(p.value("price", 0.0)),
         std::to_string(p.value("valid_days", 30)), std::to_string(p.value("limit_count", 0)),
         jsonArrayToCsv(p, "images"), jsonStr(p, "status", "on"), std::to_string(id)});
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

// ---------------- 门店经营项目上架关系 ----------------
namespace {

// 项目类型 → 关系表 / 外键列 / 源表（白名单，杜绝 SQL 拼接注入）
struct KindMap {
    const char* relTable;
    const char* relCol;
    const char* srcTable;
    bool ok;
};
KindMap kindOf(const std::string& kind) {
    if (kind == "service") return {"store_services", "service_id", "services", true};
    if (kind == "package") return {"store_packages", "package_id", "packages", true};
    if (kind == "coupon") return {"store_coupons", "coupon_id", "coupons", true};
    return {"", "", "", false};
}

}  // namespace

void fanoutItemToStores(long long merchantId, const std::string& itemKind, long long itemId) {
    auto k = kindOf(itemKind);
    if (!k.ok || itemId <= 0) return;
    Database::instance().execute(
        "INSERT OR IGNORE INTO " + std::string(k.relTable) + " (store_id, " + k.relCol +
            ", status, created_at) SELECT s.id, ?, 'on', ? FROM stores s WHERE s.merchant_id = ?",
        {std::to_string(itemId), timeutil::nowStr(), std::to_string(merchantId)});
}

void fanoutStoreItems(long long merchantId, long long storeId) {
    auto& db = Database::instance();
    const std::string now = timeutil::nowStr();
    db.execute(
        "INSERT OR IGNORE INTO store_services (store_id, service_id, status, created_at) "
        "SELECT ?, sv.id, 'on', ? FROM services sv WHERE sv.merchant_id = ?",
        {std::to_string(storeId), now, std::to_string(merchantId)});
    db.execute(
        "INSERT OR IGNORE INTO store_packages (store_id, package_id, status, created_at) "
        "SELECT ?, pk.id, 'on', ? FROM packages pk WHERE pk.merchant_id = ?",
        {std::to_string(storeId), now, std::to_string(merchantId)});
    db.execute(
        "INSERT OR IGNORE INTO store_coupons (store_id, coupon_id, status, created_at) "
        "SELECT ?, c.id, 'on', ? FROM coupons c WHERE c.merchant_id = ?",
        {std::to_string(storeId), now, std::to_string(merchantId)});
}

bool setStoreOffering(long long storeId, const std::string& itemKind, long long itemId,
                      const std::string& status) {
    auto k = kindOf(itemKind);
    if (!k.ok || itemId <= 0 || (status != "on" && status != "off")) return false;
    auto& db = Database::instance();
    long changed = db.execute("UPDATE " + std::string(k.relTable) + " SET status = ? "
                              "WHERE store_id = ? AND " + k.relCol + " = ?",
                              {status, std::to_string(storeId), std::to_string(itemId)});
    if (changed > 0) return true;
    // 关系不存在则新建（历史数据兜底）
    db.execute("INSERT OR IGNORE INTO " + std::string(k.relTable) + " (store_id, " + k.relCol +
                   ", status, created_at) VALUES (?, ?, ?, ?)",
               {std::to_string(storeId), std::to_string(itemId), status, timeutil::nowStr()});
    return true;
}

void bulkStoreOffering(long long storeId, long long merchantId, const std::string& itemKind,
                       const std::string& status) {
    auto k = kindOf(itemKind);
    if (!k.ok || (status != "on" && status != "off")) return;
    auto& db = Database::instance();
    // 先补齐关系行，再统一更新（保证「全部上架/下架」覆盖所有项目）
    db.execute("INSERT OR IGNORE INTO " + std::string(k.relTable) + " (store_id, " + k.relCol +
                   ", status, created_at) SELECT ?, t.id, 'off', ? FROM " + k.srcTable +
                   " t WHERE t.merchant_id = ?",
               {std::to_string(storeId), timeutil::nowStr(), std::to_string(merchantId)});
    db.execute("UPDATE " + std::string(k.relTable) + " SET status = ? WHERE store_id = ? AND " +
                   k.relCol + " IN (SELECT id FROM " + k.srcTable + " WHERE merchant_id = ?)",
               {status, std::to_string(storeId), std::to_string(merchantId)});
}

nlohmann::json storeOnSaleCounts(long long storeId) {
    auto one = [&](const char* sql) {
        auto r = Database::instance().queryOne(sql, {std::to_string(storeId)});
        return r.is_null() ? 0LL : r.value("c", 0LL);
    };
    return {{"services", one("SELECT COUNT(*) AS c FROM store_services ss "
                             "JOIN services s ON s.id = ss.service_id "
                             "WHERE ss.store_id = ? AND ss.status = 'on' AND s.status = 'on'")},
            {"packages", one("SELECT COUNT(*) AS c FROM store_packages sp "
                             "JOIN packages p ON p.id = sp.package_id "
                             "WHERE sp.store_id = ? AND sp.status = 'on' AND p.status = 'on'")},
            {"coupons", one("SELECT COUNT(*) AS c FROM store_coupons sc "
                            "JOIN coupons c ON c.id = sc.coupon_id "
                            "WHERE sc.store_id = ? AND sc.status = 'on' AND c.status = 'published'")}};
}

nlohmann::json storeOfferings(long long storeId, long long merchantId) {
    auto& db = Database::instance();
    const std::string sid = std::to_string(storeId);
    return {
        {"services",
         db.query("SELECT s.id, s.name, s.price, s.images, s.status AS item_status, "
                  "IFNULL(ss.status, 'off') AS store_status "
                  "FROM services s LEFT JOIN store_services ss "
                  "ON ss.service_id = s.id AND ss.store_id = ? "
                  "WHERE s.merchant_id = ? ORDER BY s.id DESC",
                  {sid, std::to_string(merchantId)})},
        {"packages",
         db.query("SELECT p.id, p.name, p.price, p.images, p.status AS item_status, "
                  "IFNULL(sp.status, 'off') AS store_status "
                  "FROM packages p LEFT JOIN store_packages sp "
                  "ON sp.package_id = p.id AND sp.store_id = ? "
                  "WHERE p.merchant_id = ? ORDER BY p.id DESC",
                  {sid, std::to_string(merchantId)})},
        {"coupons",
         db.query("SELECT c.id, c.name, c.type, c.face_value, c.discount_rate, "
                  "c.status AS item_status, IFNULL(sc.status, 'off') AS store_status "
                  "FROM coupons c LEFT JOIN store_coupons sc "
                  "ON sc.coupon_id = c.id AND sc.store_id = ? "
                  "WHERE c.merchant_id = ? ORDER BY c.id DESC",
                  {sid, std::to_string(merchantId)})}};
}

nlohmann::json listStoreServices(long long storeId) {
    return Database::instance().query(
        "SELECT s.id, s.name, s.price, s.price_unit, s.applicable_time, s.stock, s.limit_count, "
        "s.images, s.merchant_id FROM store_services ss JOIN services s ON s.id = ss.service_id "
        "WHERE ss.store_id = ? AND ss.status = 'on' AND s.status = 'on' ORDER BY s.id DESC",
        {std::to_string(storeId)});
}

nlohmann::json listStorePackages(long long storeId) {
    return Database::instance().query(
        "SELECT p.id, p.name, p.content, p.price, p.valid_days, p.limit_count, p.images, "
        "p.merchant_id FROM store_packages sp JOIN packages p ON p.id = sp.package_id "
        "WHERE sp.store_id = ? AND sp.status = 'on' AND p.status = 'on' ORDER BY p.id DESC",
        {std::to_string(storeId)});
}

nlohmann::json listStoreCoupons(long long storeId) {
    std::string now = timeutil::nowStr();
    return Database::instance().query(
        "SELECT c.*, m.name AS merchant_name FROM store_coupons sc "
        "JOIN coupons c ON c.id = sc.coupon_id JOIN merchants m ON m.id = c.merchant_id "
        "WHERE sc.store_id = ? AND sc.status = 'on' AND c.status = 'published' "
        "AND c.start_time <= ? AND c.end_time >= ? ORDER BY c.id DESC",
        {std::to_string(storeId), now, now});
}

bool isOnSaleAtStore(long long storeId, const std::string& itemKind, long long itemId) {
    auto k = kindOf(itemKind);
    if (!k.ok || itemId <= 0) return false;
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM " + std::string(k.relTable) + " WHERE store_id = ? AND " +
            k.relCol + " = ? AND status = 'on'",
        {std::to_string(storeId), std::to_string(itemId)});
    return row.is_null() ? false : (row.value("c", 0LL) > 0);
}

}  // namespace MerchantDao

