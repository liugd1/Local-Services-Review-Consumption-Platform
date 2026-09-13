#include "dao/ReviewDao.h"

#include <sstream>

#include "db/Database.h"
#include "model/Models.h"
#include "util/Time.h"

namespace ReviewDao {

namespace {
const char* kSelectCore =
    "SELECT r.*, m.name AS merchant_name, "
    "u.username, u.nickname, u.avatar, "
    "IFNULL((SELECT COUNT(*) FROM review_likes l WHERE l.review_id = r.id), 0) AS like_count, "
    "IFNULL((SELECT COUNT(*) FROM review_comments c WHERE c.review_id = r.id), 0) "
    "AS comment_count, "
    "IFNULL(rr.content, '') AS merchant_reply, "
    "(SELECT GROUP_CONCAT(ri.image_path, '|') FROM review_images ri "
    " WHERE ri.review_id = r.id) AS images "
    "FROM reviews r "
    "JOIN merchants m ON m.id = r.merchant_id "
    "JOIN users u ON u.id = r.user_id "
    "LEFT JOIN review_replies rr ON rr.review_id = r.id AND rr.comment_id IS NULL ";

// images '|' 串转数组
void attachImageArray(nlohmann::json& row) {
    std::string imgs = jsonStr(row, "images");
    nlohmann::json arr = nlohmann::json::array();
    if (!imgs.empty()) {
        std::stringstream ss(imgs);
        std::string part;
        while (std::getline(ss, part, '|')) {
            if (!part.empty()) arr.push_back(part);
        }
    }
    row["images"] = arr;
}
}  // namespace

long long addReview(const nlohmann::json& r) {
    auto row = Database::instance().queryOne(
        "INSERT INTO reviews (user_id, merchant_id, target_type, target_id, env_score, "
        "service_score, price_score, avg_score, content, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'visible', ?) RETURNING id",
        {std::to_string(r.value("user_id", 0LL)), std::to_string(r.value("merchant_id", 0LL)),
         jsonStr(r, "target_type", "merchant"), std::to_string(r.value("target_id", 0LL)),
         std::to_string(r.value("env_score", 0.0)), std::to_string(r.value("service_score", 0.0)),
         std::to_string(r.value("price_score", 0.0)), std::to_string(r.value("avg_score", 0.0)),
         jsonStr(r, "content"), timeutil::nowStr()});
    return row.is_null() ? 0 : row.value("id", 0LL);
}

bool addReviewImages(long long reviewId, const std::vector<std::string>& paths) {
    auto& db = Database::instance();
    for (const auto& p : paths) {
        if (db.execute("INSERT INTO review_images (review_id, image_path) VALUES (?, ?)",
                       {std::to_string(reviewId), p}) < 0) {
            return false;
        }
    }
    return true;
}

nlohmann::json reviewById(long long id) {
    auto row = Database::instance().queryOne(std::string(kSelectCore) + " WHERE r.id = ?",
                                             {std::to_string(id)});
    if (row.is_null()) return nlohmann::json(nullptr);
    attachImageArray(row);
    return row;
}

long long countUserVisible(long long userId, const std::string& type, long long targetId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM reviews WHERE user_id = ? AND target_type = ? AND "
        "target_id = ? AND status = 'visible'",
        {std::to_string(userId), type, std::to_string(targetId)});
    return row.is_null() ? 0 : row.value("c", 0LL);
}

bool updateStatus(long long reviewId, const std::string& status) {
    return Database::instance().execute("UPDATE reviews SET status = ? WHERE id = ?",
                                        {status, std::to_string(reviewId)}) >= 0;
}

nlohmann::json listTargetReviews(const std::string& type, long long targetId, int page, int size,
                                 long long& total) {
    auto& db = Database::instance();
    auto totalRow = db.queryOne(
        "SELECT COUNT(*) AS c FROM reviews WHERE target_type = ? AND target_id = ? "
        "AND status = 'visible'",
        {type, std::to_string(targetId)});
    total = totalRow.is_null() ? 0 : totalRow.value("c", 0LL);
    int offset = (page - 1) * size;
    auto rows = db.query(std::string(kSelectCore) +
                             " WHERE r.target_type = ? AND r.target_id = ? AND r.status = "
                             "'visible' ORDER BY r.id DESC LIMIT ? OFFSET ?",
                         {type, std::to_string(targetId), std::to_string(size),
                          std::to_string(offset)});
    for (auto& row : rows) attachImageArray(row);
    return rows;
}

nlohmann::json targetSummary(const std::string& type, long long targetId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS cnt, IFNULL(ROUND(AVG(avg_score), 1), 0) AS avg_score "
        "FROM reviews WHERE target_type = ? AND target_id = ? AND status = 'visible'",
        {type, std::to_string(targetId)});
    if (row.is_null()) return {{"cnt", 0}, {"avg_score", 0.0}};
    return {{"cnt", row.value("cnt", 0LL)}, {"avg_score", row.value("avg_score", 0.0)}};
}

nlohmann::json listMerchantReviews(long long merchantId, const std::string& filterType, int page,
                                   int size, long long& total) {
    auto& db = Database::instance();
    nlohmann::json rows;
    long long c = 0;
    if (filterType.empty()) {
        auto tr = db.queryOne(
            "SELECT COUNT(*) AS c FROM reviews WHERE merchant_id = ? AND status = 'visible'",
            {std::to_string(merchantId)});
        c = tr.is_null() ? 0 : tr.value("c", 0LL);
        rows = db.query(std::string(kSelectCore) +
                            " WHERE r.merchant_id = ? AND r.status = 'visible' "
                            "ORDER BY r.id DESC LIMIT ? OFFSET ?",
                        {std::to_string(merchantId), std::to_string(size),
                         std::to_string((page - 1) * size)});
    } else {
        auto tr = db.queryOne(
            "SELECT COUNT(*) AS c FROM reviews WHERE merchant_id = ? AND target_type = ? AND "
            "status = 'visible'",
            {std::to_string(merchantId), filterType});
        c = tr.is_null() ? 0 : tr.value("c", 0LL);
        rows = db.query(std::string(kSelectCore) +
                            " WHERE r.merchant_id = ? AND r.target_type = ? AND r.status = "
                            "'visible' ORDER BY r.id DESC LIMIT ? OFFSET ?",
                        {std::to_string(merchantId), filterType, std::to_string(size),
                         std::to_string((page - 1) * size)});
    }
    total = c;
    for (auto& row : rows) attachImageArray(row);
    return rows;
}

nlohmann::json listByUser(long long userId, int page, int size, long long& total) {
    auto& db = Database::instance();
    auto totalRow = db.queryOne(
        "SELECT COUNT(*) AS c FROM reviews WHERE user_id = ? AND status != 'deleted'",
        {std::to_string(userId)});
    total = totalRow.is_null() ? 0 : totalRow.value("c", 0LL);
    int offset = (page - 1) * size;
    auto rows = db.query(std::string(kSelectCore) +
                             " WHERE r.user_id = ? AND r.status != 'deleted' "
                             "ORDER BY r.id DESC LIMIT ? OFFSET ?",
                         {std::to_string(userId), std::to_string(size), std::to_string(offset)});
    for (auto& row : rows) attachImageArray(row);
    return rows;
}

// 商户店内各评价对象汇总（计数 / 均分）
nlohmann::json summaryByMerchant(long long merchantId) {
    return Database::instance().query(
        "SELECT target_type, target_id, COUNT(*) AS cnt, "
        "ROUND(AVG(avg_score), 1) AS avg_score FROM reviews "
        "WHERE merchant_id = ? AND status = 'visible' GROUP BY target_type, target_id "
        "ORDER BY target_type, target_id",
        {std::to_string(merchantId)});
}

// ---------------- 评论（含楼中楼） ----------------
long long addComment(long long reviewId, long long userId, const std::string& content,
                     long long parentId) {
    auto& db = Database::instance();
    nlohmann::json row;
    if (parentId > 0) {
        row = db.queryOne(
            "INSERT INTO review_comments (review_id, user_id, parent_id, content, created_at) "
            "VALUES (?, ?, ?, ?, ?) RETURNING id",
            {std::to_string(reviewId), std::to_string(userId), std::to_string(parentId), content,
             timeutil::nowStr()});
    } else {
        row = db.queryOne(
            "INSERT INTO review_comments (review_id, user_id, parent_id, content, created_at) "
            "VALUES (?, ?, NULL, ?, ?) RETURNING id",
            {std::to_string(reviewId), std::to_string(userId), content, timeutil::nowStr()});
    }
    return row.is_null() ? 0 : row.value("id", 0LL);
}

nlohmann::json listComments(long long reviewId, long long viewerId) {
    return Database::instance().query(
        "SELECT c.id, c.review_id, c.parent_id, c.content, c.created_at, "
        "u.id AS user_id, u.username, u.nickname, u.avatar, "
        "IFNULL(pu.nickname, '') AS reply_nickname, IFNULL(pu.username, '') AS reply_username, "
        "IFNULL((SELECT COUNT(*) FROM comment_likes cl WHERE cl.comment_id = c.id), 0) "
        "AS like_count, "
        "IFNULL((SELECT 1 FROM comment_likes cl2 WHERE cl2.comment_id = c.id AND cl2.user_id = ?), "
        "0) AS liked "
        "FROM review_comments c "
        "JOIN users u ON u.id = c.user_id "
        "LEFT JOIN review_comments pc ON pc.id = c.parent_id "
        "LEFT JOIN users pu ON pu.id = pc.user_id "
        "WHERE c.review_id = ? ORDER BY c.id ASC",
        {std::to_string(viewerId), std::to_string(reviewId)});
}

bool commentBelongsTo(long long reviewId, long long commentId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM review_comments WHERE id = ? AND review_id = ?",
        {std::to_string(commentId), std::to_string(reviewId)});
    return row.is_null() ? false : (row.value("c", 0LL) > 0);
}

long long commentAuthorId(long long commentId) {
    auto row = Database::instance().queryOne("SELECT user_id FROM review_comments WHERE id = ?",
                                             {std::to_string(commentId)});
    return row.is_null() ? 0 : row.value("user_id", 0LL);
}

// 递归删除该评论及其全部后代（连同点赞/举报记录），返回删除条数
long long deleteCommentTree(long long commentId) {
    auto& db = Database::instance();
    std::string cte =
        "WITH RECURSIVE cte(id) AS ( "
        "  SELECT id FROM review_comments WHERE id = " + std::to_string(commentId) +
        "  UNION ALL "
        "  SELECT c.id FROM review_comments c JOIN cte ON c.parent_id = cte.id "
        ") ";
    // 先清附属，再删评论本体
    db.execute(cte + "DELETE FROM comment_likes WHERE comment_id IN (SELECT id FROM cte)");
    db.execute(cte + "DELETE FROM comment_reports WHERE comment_id IN (SELECT id FROM cte)");
    return db.execute(cte + "DELETE FROM review_comments WHERE id IN (SELECT id FROM cte)");
}

// 删除某评价的全部评论（连带点赞/举报附属），返回删除评论条数
long long deleteCommentsOfReview(long long reviewId) {
    auto& db = Database::instance();
    db.execute("DELETE FROM comment_likes WHERE comment_id IN "
               "(SELECT id FROM review_comments WHERE review_id = ?)",
               {std::to_string(reviewId)});
    db.execute("DELETE FROM comment_reports WHERE comment_id IN "
               "(SELECT id FROM review_comments WHERE review_id = ?)",
               {std::to_string(reviewId)});
    return db.execute("DELETE FROM review_comments WHERE review_id = ?",
                      {std::to_string(reviewId)});
}

// ---------------- 点赞 ----------------
bool likeAdd(long long reviewId, long long userId) {
    Database::instance().execute(
        "INSERT OR IGNORE INTO review_likes (review_id, user_id, created_at) VALUES (?, ?, ?)",
        {std::to_string(reviewId), std::to_string(userId), timeutil::nowStr()});
    return true;
}

bool likeRemove(long long reviewId, long long userId) {
    return Database::instance().execute(
               "DELETE FROM review_likes WHERE review_id = ? AND user_id = ?",
               {std::to_string(reviewId), std::to_string(userId)}) >= 0;
}

// ---------------- 商户回复 ----------------
nlohmann::json getMerchantReply(long long reviewId) {
    auto row = Database::instance().queryOne(
        "SELECT * FROM review_replies WHERE review_id = ? AND comment_id IS NULL",
        {std::to_string(reviewId)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

long long addMerchantReply(long long reviewId, long long merchantId, const std::string& content) {
    auto row = Database::instance().queryOne(
        "INSERT INTO review_replies (review_id, comment_id, merchant_id, content, created_at) "
        "VALUES (?, NULL, ?, ?, ?) RETURNING id",
        {std::to_string(reviewId), std::to_string(merchantId), content, timeutil::nowStr()});
    return row.is_null() ? 0 : row.value("id", 0LL);
}

bool updateMerchantReply(long long replyId, const std::string& content) {
    return Database::instance().execute("UPDATE review_replies SET content = ? WHERE id = ?",
                                        {content, std::to_string(replyId)}) >= 0;
}

// ---------------- 举报 ----------------
long long addReport(long long reviewId, long long userId, const std::string& reason) {
    auto row = Database::instance().queryOne(
        "INSERT INTO review_reports (review_id, user_id, reason, status, result, created_at) "
        "VALUES (?, ?, ?, 'pending', '', ?) RETURNING id",
        {std::to_string(reviewId), std::to_string(userId), reason, timeutil::nowStr()});
    return row.is_null() ? 0 : row.value("id", 0LL);
}

nlohmann::json reportById(long long id) {
    auto row = Database::instance().queryOne(
        "SELECT rp.*, r.content AS review_content, r.merchant_id, m.name AS merchant_name, "
        "ru.username AS reporter_username FROM review_reports rp "
        "JOIN reviews r ON r.id = rp.review_id "
        "JOIN merchants m ON m.id = r.merchant_id "
        "JOIN users ru ON ru.id = rp.user_id "
        "WHERE rp.id = ?",
        {std::to_string(id)});
    return row.is_null() ? nlohmann::json(nullptr) : row;
}

nlohmann::json listReports(const std::string& status, int page, int size, long long& total) {
    auto& db = Database::instance();
    std::string where = status.empty() ? " WHERE 1=1 " : " WHERE rp.status = ? ";
    std::vector<std::string> params;
    if (!status.empty()) params.push_back(status);

    auto totalRow = db.queryOne("SELECT COUNT(*) AS c FROM review_reports rp" + where, params);
    total = totalRow.is_null() ? 0 : totalRow.value("c", 0LL);

    auto pageParams = params;
    pageParams.push_back(std::to_string(size));
    pageParams.push_back(std::to_string((page - 1) * size));
    return db.query(
        "SELECT rp.id, rp.review_id, rp.reason, rp.status, rp.result, rp.created_at, "
        "rp.user_id, ru.username AS reporter_username, r.content AS review_content, "
        "r.avg_score, r.merchant_id, m.name AS merchant_name, "
        "r.user_id AS review_user_id, u2.nickname AS review_author "
        "FROM review_reports rp "
        "JOIN reviews r ON r.id = rp.review_id "
        "JOIN merchants m ON m.id = r.merchant_id "
        "JOIN users ru ON ru.id = rp.user_id "
        "JOIN users u2 ON u2.id = r.user_id" +
            where + " ORDER BY rp.id DESC LIMIT ? OFFSET ?",
        pageParams);
}

bool setReportStatus(long long id, const std::string& status, const std::string& result) {
    return Database::instance().execute(
               "UPDATE review_reports SET status = ?, result = ? WHERE id = ?",
               {status, result, std::to_string(id)}) >= 0;
}

// ---------------- 评论点赞（每条评论“有用”） ----------------
bool commentLikeAdd(long long commentId, long long userId) {
    Database::instance().execute(
        "INSERT OR IGNORE INTO comment_likes (comment_id, user_id, created_at) VALUES (?, ?, ?)",
        {std::to_string(commentId), std::to_string(userId), timeutil::nowStr()});
    return true;
}

bool commentLikeRemove(long long commentId, long long userId) {
    return Database::instance().execute(
               "DELETE FROM comment_likes WHERE comment_id = ? AND user_id = ?",
               {std::to_string(commentId), std::to_string(userId)}) >= 0;
}

bool isCommentLiked(long long commentId, long long userId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM comment_likes WHERE comment_id = ? AND user_id = ?",
        {std::to_string(commentId), std::to_string(userId)});
    return row.is_null() ? false : (row.value("c", 0LL) > 0);
}

long long countCommentLikes(long long commentId) {
    auto row = Database::instance().queryOne(
        "SELECT COUNT(*) AS c FROM comment_likes WHERE comment_id = ?",
        {std::to_string(commentId)});
    return row.is_null() ? 0 : row.value("c", 0LL);
}

// ---------------- 评论举报 ----------------
long long addCommentReport(long long commentId, long long userId, const std::string& reason) {
    auto row = Database::instance().queryOne(
        "INSERT INTO comment_reports (comment_id, user_id, reason, status, result, created_at) "
        "VALUES (?, ?, ?, 'pending', '', ?) RETURNING id",
        {std::to_string(commentId), std::to_string(userId), reason, timeutil::nowStr()});
    return row.is_null() ? 0 : row.value("id", 0LL);
}

nlohmann::json commentReportById(long long id) {
    return Database::instance().queryOne(
        "SELECT rp.*, c.content AS comment_content, cu.nickname AS comment_author, "
        "ru.username AS reporter_username, m.name AS merchant_name FROM comment_reports rp "
        "JOIN review_comments c ON c.id = rp.comment_id "
        "JOIN reviews r ON r.id = c.review_id "
        "JOIN merchants m ON m.id = r.merchant_id "
        "JOIN users ru ON ru.id = rp.user_id "
        "JOIN users cu ON cu.id = c.user_id WHERE rp.id = ?",
        {std::to_string(id)});
}

nlohmann::json listCommentReports(const std::string& status, int page, int size,
                                  long long& total) {
    auto& db = Database::instance();
    std::string where = status.empty() ? " WHERE 1=1 " : " WHERE rp.status = ? ";
    std::vector<std::string> params;
    if (!status.empty()) params.push_back(status);

    auto totalRow = db.queryOne("SELECT COUNT(*) AS c FROM comment_reports rp" + where, params);
    total = totalRow.is_null() ? 0 : totalRow.value("c", 0LL);

    auto pageParams = params;
    pageParams.push_back(std::to_string(size));
    pageParams.push_back(std::to_string((page - 1) * size));
    return db.query(
        "SELECT rp.id, rp.comment_id, rp.reason, rp.status, rp.result, rp.created_at, "
        "rp.user_id, ru.username AS reporter_username, c.content AS comment_content, "
        "cu.nickname AS comment_author, m.name AS merchant_name "
        "FROM comment_reports rp "
        "JOIN review_comments c ON c.id = rp.comment_id "
        "JOIN reviews r ON r.id = c.review_id "
        "JOIN merchants m ON m.id = r.merchant_id "
        "JOIN users ru ON ru.id = rp.user_id "
        "JOIN users cu ON cu.id = c.user_id" +
            where + " ORDER BY rp.id DESC LIMIT ? OFFSET ?",
        pageParams);
}

bool setCommentReportStatus(long long id, const std::string& status, const std::string& result) {
    return Database::instance().execute(
               "UPDATE comment_reports SET status = ?, result = ? WHERE id = ?",
               {status, result, std::to_string(id)}) >= 0;
}

}  // namespace ReviewDao


