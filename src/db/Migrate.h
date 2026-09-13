#pragma once
// 轻量增量迁移：老库补充新列（幂等）
#include "db/Database.h"

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
    return true;
}

}  // namespace migrate
