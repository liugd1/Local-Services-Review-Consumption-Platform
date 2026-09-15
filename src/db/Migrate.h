#pragma once
// 轻量增量迁移：老库补充新列 / 重建表结构（幂等）
#include "db/Database.h"
#include "util/Time.h"

namespace migrate {

inline bool hasColumn(const std::string& table, const std::string& col) {
    for (auto& row : Database::instance().query("PRAGMA table_info(" + table + ")")) {
        if (row.value("name", "") == col) return true;
    }
    return false;
}

inline bool run() {
    auto& db = Database::instance();

    // 1) reviews：评价对象化（店铺/门店/服务/套餐）
    if (!hasColumn("reviews", "target_type")) {
        db.execute("ALTER TABLE reviews ADD COLUMN target_type TEXT NOT NULL DEFAULT 'merchant'");
        db.execute("ALTER TABLE reviews ADD COLUMN target_id INTEGER NOT NULL DEFAULT 0");
        // 老数据回填为“对店铺本身的评价”
        db.execute("UPDATE reviews SET target_id = merchant_id WHERE target_id = 0");
    }
    // 2) review_comments：支持对评论追加评论
    if (!hasColumn("review_comments", "parent_id")) {
        db.execute("ALTER TABLE review_comments ADD COLUMN parent_id INTEGER REFERENCES review_comments(id)");
    }
    db.execute("CREATE INDEX IF NOT EXISTS idx_reviews_target ON reviews (target_type, target_id)");

    // 3) orders：下单主体下沉到「门店」，并支持服务项目/优惠套餐两类商品（老库需重建表）
    if (!hasColumn("orders", "store_id")) {
        db.execute("ALTER TABLE orders RENAME TO orders_legacy");
        db.execute(
            "CREATE TABLE orders ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " order_no TEXT NOT NULL UNIQUE,"
            " user_id INTEGER NOT NULL REFERENCES users (id),"
            " store_id INTEGER REFERENCES stores (id),"
            " item_type TEXT NOT NULL DEFAULT 'package'"
            "   CHECK (item_type IN ('package','service')),"
            " package_id INTEGER REFERENCES packages (id),"
            " service_id INTEGER REFERENCES services (id),"
            " amount REAL NOT NULL,"
            " status TEXT NOT NULL DEFAULT 'purchased'"
            "   CHECK (status IN ('purchased','used','refunded')),"
            " created_at TEXT NOT NULL,"
            " used_time TEXT)");
        // 老订单均为套餐订单：门店取该套餐所属商户的第一个门店
        db.execute(
            "INSERT INTO orders (id, order_no, user_id, store_id, item_type, package_id, "
            "service_id, amount, status, created_at, used_time) "
            "SELECT o.id, o.order_no, o.user_id, "
            "  (SELECT s.id FROM stores s JOIN packages p ON p.merchant_id = s.merchant_id "
            "   WHERE p.id = o.package_id ORDER BY s.id LIMIT 1), "
            "  'package', o.package_id, NULL, o.amount, o.status, o.created_at, o.used_time "
            "FROM orders_legacy o");
        db.execute("DROP TABLE orders_legacy");
    }
    // 订单索引（新库与老库统一在此建立）
    db.execute("CREATE INDEX IF NOT EXISTS idx_orders_store ON orders (store_id)");
    db.execute("CREATE INDEX IF NOT EXISTS idx_orders_user ON orders (user_id)");

    // 4) consumption_records：补充发生消费的门店
    if (!hasColumn("consumption_records", "store_id")) {
        db.execute(
            "ALTER TABLE consumption_records ADD COLUMN store_id INTEGER REFERENCES stores (id)");
    }
    db.execute("CREATE INDEX IF NOT EXISTS idx_consumption_merchant ON consumption_records (merchant_id)");
    db.execute("CREATE INDEX IF NOT EXISTS idx_consumption_store ON consumption_records (store_id)");

    // 5) 门店 / 服务项目 / 优惠套餐：补充图片列（多图以英文逗号分隔）
    if (!hasColumn("stores", "images"))
        db.execute("ALTER TABLE stores ADD COLUMN images TEXT DEFAULT ''");
    if (!hasColumn("services", "images"))
        db.execute("ALTER TABLE services ADD COLUMN images TEXT DEFAULT ''");
    if (!hasColumn("packages", "images"))
        db.execute("ALTER TABLE packages ADD COLUMN images TEXT DEFAULT ''");

    // 6) 门店经营项目上架关系回填：存量门店 × 服务/套餐/活动 默认「上架」，
    //    保证升级后原有店铺仍可在门店页下单
    const std::string now = timeutil::nowStr();
    db.execute(
        "INSERT OR IGNORE INTO store_services (store_id, service_id, status, created_at) "
        "SELECT s.id, sv.id, 'on', ? FROM stores s JOIN services sv ON sv.merchant_id = s.merchant_id",
        {now});
    db.execute(
        "INSERT OR IGNORE INTO store_packages (store_id, package_id, status, created_at) "
        "SELECT s.id, pk.id, 'on', ? FROM stores s JOIN packages pk ON pk.merchant_id = s.merchant_id",
        {now});
    db.execute(
        "INSERT OR IGNORE INTO store_coupons (store_id, coupon_id, status, created_at) "
        "SELECT s.id, c.id, 'on', ? FROM stores s JOIN coupons c ON c.merchant_id = s.merchant_id",
        {now});
    return true;
}

}  // namespace migrate
