#include "dao/SearchDao.h"

#include "db/Database.h"
#include "model/Models.h"
#include "util/Time.h"

namespace SearchDao {

namespace {
// 商户 + 评分/评价数聚合的公共查询片段
const char* kSelectPrefix =
    "SELECT m.*, c.name AS category_name, u.username AS owner_username, "
    "       IFNULL(rv.review_count, 0) AS review_count, "
    "       IFNULL(rv.avg_score, 0) AS avg_score ";
const char* kFromBase =
    "FROM merchants m "
    "LEFT JOIN categories c ON c.id = m.category_id "
    "LEFT JOIN users u ON u.id = m.user_id "
    "LEFT JOIN (SELECT merchant_id, COUNT(*) AS review_count, AVG(avg_score) AS avg_score "
    "           FROM reviews WHERE status = 'visible' AND target_type = 'merchant' "
    "           GROUP BY merchant_id) rv "
    "ON rv.merchant_id = m.id ";
const char* kWhereApproved = " WHERE m.status = 'approved' ";

std::string orderClause(const std::string& sort) {
    if (sort == "score") return " ORDER BY avg_score DESC, m.view_count DESC, m.id DESC ";
    if (sort == "popularity") return " ORDER BY m.view_count DESC, m.id DESC ";
    if (sort == "newest") return " ORDER BY m.id DESC ";
    return " ORDER BY m.id DESC ";
}
}  // namespace

nlohmann::json search(const SearchParams& p, long long& total) {
    auto& db = Database::instance();
    std::string where = kWhereApproved;
    std::vector<std::string> params;

    if (!p.keyword.empty()) {
        where += " AND (m.name LIKE ? OR m.intro LIKE ? OR m.area LIKE ? OR c.name LIKE ?) ";
        auto kw = "%" + p.keyword + "%";
        params.push_back(kw);
        params.push_back(kw);
        params.push_back(kw);
        params.push_back(kw);
    }
    if (p.categoryId > 0) {
        where += " AND m.category_id = ? ";
        params.push_back(std::to_string(p.categoryId));
    }
    if (!p.area.empty()) {
        where += " AND m.area LIKE ? ";
        params.push_back("%" + p.area + "%");
    }
    if (p.priceMin > 0) {
        where += " AND m.price_max >= ? ";
        params.push_back(std::to_string(p.priceMin));
    }
    if (p.priceMax > 0) {
        where += " AND m.price_min <= ? ";
        params.push_back(std::to_string(p.priceMax));
    }
    if (p.minScore > 0) {
        where += " AND rv.avg_score >= ? ";
        params.push_back(std::to_string(p.minScore));
    }

    // 统计总数（内层按商户 id 去重）
    auto countRow = db.queryOne(
        "SELECT COUNT(*) AS c FROM (SELECT m.id " + std::string(kFromBase) + where + ") t",
        params);
    total = countRow.is_null() ? 0 : countRow.value("c", 0LL);

    // 分页列表
    auto pageParams = params;
    pageParams.push_back(std::to_string(p.size));
    pageParams.push_back(std::to_string((p.page - 1) * p.size));
    return db.query(std::string(kSelectPrefix) + kFromBase + where + orderClause(p.sort) +
                        " LIMIT ? OFFSET ?",
                    pageParams);
}

nlohmann::json detail(long long merchantId) {
    auto row = Database::instance().queryOne(
        std::string(kSelectPrefix) + kFromBase + " WHERE m.id = ?",
        {std::to_string(merchantId)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

nlohmann::json hotRank(int limit) {
    return Database::instance().query(std::string(kSelectPrefix) + kFromBase + kWhereApproved +
                                          " ORDER BY m.view_count DESC, m.id DESC LIMIT ?",
                                      {std::to_string(limit)});
}

nlohmann::json newRank(int limit) {
    return Database::instance().query(std::string(kSelectPrefix) + kFromBase + kWhereApproved +
                                          " ORDER BY m.id DESC LIMIT ?",
                                      {std::to_string(limit)});
}

nlohmann::json categoryRank(long long categoryId, int limit) {
    return Database::instance().query(std::string(kSelectPrefix) + kFromBase + kWhereApproved +
                                          " AND m.category_id = ? ORDER BY avg_score DESC, "
                                          "m.view_count DESC LIMIT ?",
                                      {std::to_string(categoryId), std::to_string(limit)});
}

}  // namespace SearchDao
