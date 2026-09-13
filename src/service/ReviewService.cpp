#include "service/ReviewService.h"

#include <cmath>

#include "dao/MerchantDao.h"
#include "dao/ReviewDao.h"
#include "dao/SearchDao.h"
#include "model/Models.h"
#include "util/BizError.h"

namespace {

// 评价对象名（用于我的评价 / 列表装饰 / talk 页头）
std::string objectName(const std::string& type, long long id) {
    if (type == "merchant") {
        auto m = SearchDao::detail(id);
        return m.is_null() ? "" : jsonStr(m, "name");
    }
    if (type == "store") {
        auto s = MerchantDao::storeById(id);
        return s.is_null() ? "" : jsonStr(s, "name");
    }
    if (type == "service") {
        auto s = MerchantDao::serviceById(id);
        return s.is_null() ? "" : jsonStr(s, "name");
    }
    if (type == "package") {
        auto p = MerchantDao::packageById(id);
        return p.is_null() ? "" : jsonStr(p, "name");
    }
    return "";
}

// 解析对象：校验 type/id 合法，返回归属商户 id 与对象名
nlohmann::json resolveObject(const std::string& type, long long id) {
    nlohmann::json out = nullptr;
    long long merchantId = 0;
    std::string name;
    if (type == "merchant") {
        auto m = SearchDao::detail(id);
        if (m.is_null()) return nullptr;
        merchantId = id;
        name = jsonStr(m, "name");
    } else if (type == "store") {
        auto s = MerchantDao::storeById(id);
        if (s.is_null()) return nullptr;
        merchantId = s.value("merchant_id", 0LL);
        name = jsonStr(s, "name");
    } else if (type == "service") {
        auto s = MerchantDao::serviceById(id);
        if (s.is_null()) return nullptr;
        merchantId = s.value("merchant_id", 0LL);
        name = jsonStr(s, "name");
    } else if (type == "package") {
        auto p = MerchantDao::packageById(id);
        if (p.is_null()) return nullptr;
        merchantId = p.value("merchant_id", 0LL);
        name = jsonStr(p, "name");
    } else {
        return nullptr;
    }
    auto m = MerchantDao::byId(merchantId);
    out = {{"target_type", type}, {"target_id", id}, {"merchant_id", merchantId},
           {"name", name}};
    out["merchant_name"] = m.is_null() ? "" : jsonStr(m, "name");
    return out;
}

double roundScore(double v) { return std::round(v * 10.0) / 10.0; }

void checkScore(double v) {
    if (v < 1 || v > 5) throw BizError(resp::PARAM_ERROR, "评分数值需在 1-5 之间");
}

// 给评价行附加对象名（talk 列表 / 我的评价）
void decorateTargetName(nlohmann::json& row) {
    row["target_name"] = objectName(jsonStr(row, "target_type"), row.value("target_id", 0LL));
}

}  // namespace

// ---------------- 发表评价（对象化） ----------------
nlohmann::json ReviewService::postReview(long long userId, const nlohmann::json& body) {
    std::string type = jsonStr(body, "target_type", "merchant");
    long long targetId = jsonInt(body, "target_id");
    if (type == "merchant" && targetId <= 0) targetId = jsonInt(body, "merchant_id");
    if (targetId <= 0) throw BizError(resp::PARAM_ERROR, "参数错误：缺少评价对象");

    auto obj = resolveObject(type, targetId);
    if (obj.is_null()) throw BizError(resp::NOT_FOUND, "评价对象不存在");
    // 对象所属商户须已上架
    auto m = SearchDao::detail(obj.value("merchant_id", 0LL));
    if (m.is_null() || jsonStr(m, "status") != "approved")
        throw BizError(resp::FORBIDDEN, "该商户尚未上架，暂时不能评价");
    long long merchantId = obj.value("merchant_id", 0LL);

    double env = jsonNum(body, "env_score");
    double svc = jsonNum(body, "service_score");
    double price = jsonNum(body, "price_score");
    checkScore(env);
    checkScore(svc);
    checkScore(price);

    std::string content = jsonStr(body, "content");
    if (content.size() > 2000) throw BizError(resp::PARAM_ERROR, "评价内容过长（最多 2000 字）");

    nlohmann::json images = nlohmann::json::array();
    auto it = body.find("images");
    if (it != body.end() && it->is_array()) {
        for (const auto& img : *it) {
            if (!img.is_string()) continue;
            std::string p = img.get<std::string>();
            if (!p.empty() && p.size() <= 200) images.push_back(p);
        }
        if (images.size() > 6) throw BizError(resp::PARAM_ERROR, "最多上传 6 张图片");
    }

    // 同一对象一评
    if (ReviewDao::countUserVisible(userId, type, targetId) > 0)
        throw BizError(resp::CONFLICT, "您已评价过该对象，可删除后重新评价");

    nlohmann::json r = {{"user_id", userId},
                        {"merchant_id", merchantId},
                        {"target_type", type},
                        {"target_id", targetId},
                        {"env_score", env},
                        {"service_score", svc},
                        {"price_score", price},
                        {"avg_score", roundScore((env + svc + price) / 3.0)},
                        {"content", content}};
    auto id = ReviewDao::addReview(r);
    if (id == 0) throw BizError(resp::SERVER_ERROR, "发表评价失败");
    if (!images.empty()) {
        std::vector<std::string> paths;
        for (const auto& img : images) paths.push_back(img.get<std::string>());
        ReviewDao::addReviewImages(id, paths);
    }
    auto rev = ReviewDao::reviewById(id);
    decorateTargetName(rev);
    return rev;
}

// ---------------- 列表与汇总 ----------------
nlohmann::json ReviewService::listMerchantReviews(long long merchantId,
                                                  const std::string& filterType, int page,
                                                  int size) {
    if (page < 1) page = 1;
    if (size < 1 || size > 50) size = 10;
    if (SearchDao::detail(merchantId).is_null())
        throw BizError(resp::NOT_FOUND, "商户不存在");
    std::string ft = filterType;
    if (!ft.empty() && ft != "merchant" && ft != "store" && ft != "service" &&
        ft != "package")
        throw BizError(resp::PARAM_ERROR, "评价对象类型仅支持 merchant/store/service/package");
    long long total = 0;
    auto rows = ReviewDao::listMerchantReviews(merchantId, ft, page, size, total);
    for (auto& row : rows) decorateTargetName(row);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)}};
}

nlohmann::json ReviewService::merchantReviewSummary(long long merchantId) {
    nlohmann::json out = {{"merchant", {{"cnt", 0}, {"avg_score", 0.0}}},
                          {"store", {{"cnt", 0}, {"avg_score", 0.0}}},
                          {"service", {{"cnt", 0}, {"avg_score", 0.0}}},
                          {"package", {{"cnt", 0}, {"avg_score", 0.0}}},
                          {"total", 0}};
    auto rows = ReviewDao::summaryByMerchant(merchantId);
    for (const auto& g : rows) {
        std::string t = jsonStr(g, "target_type");
        if (!out.contains(t)) continue;
        auto& agg = out[t];
        agg["cnt"] = agg.value("cnt", 0LL) + g.value("cnt", 0LL);
        double a = g.value("avg_score", 0.0);
        agg["avg_score"] = a;  // 同一类型多对象取最新组平均即可，供展示
        out["total"] = out.value("total", 0LL) + g.value("cnt", 0LL);
    }
    return out;
}

nlohmann::json ReviewService::listTargetReviews(const std::string& type, long long targetId,
                                                int page, int size) {
    if (page < 1) page = 1;
    if (size < 1 || size > 50) size = 10;
    auto obj = resolveObject(type, targetId);
    if (obj.is_null()) throw BizError(resp::NOT_FOUND, "评价对象不存在");
    long long total = 0;
    auto rows = ReviewDao::listTargetReviews(type, targetId, page, size, total);
    for (auto& row : rows) decorateTargetName(row);
    auto sum = ReviewDao::targetSummary(type, targetId);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)},
            {"target", obj}, {"summary", sum}};
}

nlohmann::json ReviewService::objectInfo(const std::string& type, long long id) {
    auto obj = resolveObject(type, id);
    if (obj.is_null()) throw BizError(resp::NOT_FOUND, "评价对象不存在");
    auto sum = ReviewDao::targetSummary(type, id);
    obj["summary"] = sum;
    return obj;
}

nlohmann::json ReviewService::listMyReviews(long long userId, int page, int size) {
    if (page < 1) page = 1;
    if (size < 1 || size > 50) size = 10;
    long long total = 0;
    auto rows = ReviewDao::listByUser(userId, page, size, total);
    for (auto& row : rows) decorateTargetName(row);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)}};
}

long long ReviewService::deleteReview(long long userId, const std::string& role,
                                      long long reviewId) {
    auto r = ReviewDao::reviewById(reviewId);
    if (r.is_null() || jsonStr(r, "status") == "deleted")
        throw BizError(resp::NOT_FOUND, "评价不存在");
    if (r.value("user_id", 0LL) != userId && role != "admin")
        throw BizError(resp::FORBIDDEN, "只能删除自己发表的评价");
    // 软删主评论（评价），并级联清除其评论区的全部评论
    ReviewDao::updateStatus(reviewId, "deleted");
    return ReviewDao::deleteCommentsOfReview(reviewId);
}

nlohmann::json ReviewService::toggleLike(long long userId, long long reviewId, bool like) {
    auto r = ReviewDao::reviewById(reviewId);
    if (r.is_null() || jsonStr(r, "status") == "deleted")
        throw BizError(resp::NOT_FOUND, "评价不存在");
    if (like) {
        ReviewDao::likeAdd(reviewId, userId);
        return {{"liked", true}};
    }
    ReviewDao::likeRemove(reviewId, userId);
    return {{"liked", false}};
}

void ReviewService::commentReview(long long userId, long long reviewId,
                                  const std::string& content, long long parentId) {
    if (content.empty()) throw BizError(resp::PARAM_ERROR, "评论内容不能为空");
    if (content.size() > 500) throw BizError(resp::PARAM_ERROR, "评论过长（最多 500 字）");
    auto r = ReviewDao::reviewById(reviewId);
    if (r.is_null() || jsonStr(r, "status") == "deleted")
        throw BizError(resp::NOT_FOUND, "评价不存在");
    if (parentId > 0 && !ReviewDao::commentBelongsTo(reviewId, parentId))
        throw BizError(resp::NOT_FOUND, "被回复的评论不存在");
    if (ReviewDao::addComment(reviewId, userId, content, parentId) == 0)
        throw BizError(resp::SERVER_ERROR, "评论失败");
}

nlohmann::json ReviewService::listComments(long long reviewId, long long viewerId) {
    auto r = ReviewDao::reviewById(reviewId);
    if (r.is_null()) throw BizError(resp::NOT_FOUND, "评价不存在");
    return ReviewDao::listComments(reviewId, viewerId);
}

nlohmann::json ReviewService::commentLikeToggle(long long userId, long long commentId, bool like) {
    if (ReviewDao::commentAuthorId(commentId) == 0)
        throw BizError(resp::NOT_FOUND, "评论不存在");
    if (like) {
        ReviewDao::commentLikeAdd(commentId, userId);
        return {{"liked", true}, {"like_count", ReviewDao::countCommentLikes(commentId)}};
    }
    ReviewDao::commentLikeRemove(commentId, userId);
    return {{"liked", false}, {"like_count", ReviewDao::countCommentLikes(commentId)}};
}

void ReviewService::reportComment(long long userId, long long commentId,
                                  const std::string& reason) {
    if (reason.empty()) throw BizError(resp::PARAM_ERROR, "请填写举报原因");
    if (reason.size() > 200) throw BizError(resp::PARAM_ERROR, "举报原因过长");
    if (ReviewDao::commentAuthorId(commentId) == 0)
        throw BizError(resp::NOT_FOUND, "评论不存在");
    if (ReviewDao::addCommentReport(commentId, userId, reason) == 0)
        throw BizError(resp::SERVER_ERROR, "举报提交失败");
}

nlohmann::json ReviewService::listCommentReports(const std::string& status, int page, int size) {
    if (page < 1) page = 1;
    if (size < 1 || size > 100) size = 20;
    long long total = 0;
    auto rows = ReviewDao::listCommentReports(status, page, size, total);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)}};
}

void ReviewService::handleCommentReport(long long reportId, const std::string& action,
                                        const std::string& reason) {
    auto rep = ReviewDao::commentReportById(reportId);
    if (rep.is_null()) throw BizError(resp::NOT_FOUND, "举报记录不存在");
    if (action == "delete") {
        // 平台判定违规：删除该评论（递归清除其后代评论）
        ReviewDao::deleteCommentTree(rep.value("comment_id", 0LL));
        ReviewDao::setCommentReportStatus(reportId, "resolved", "已删除该评论");
    } else if (action == "reject") {
        if (reason.empty()) throw BizError(resp::PARAM_ERROR, "请填写驳回原因");
        ReviewDao::setCommentReportStatus(reportId, "rejected", reason);
    } else {
        throw BizError(resp::PARAM_ERROR, "处理动作仅支持 delete / reject");
    }
}

long long ReviewService::deleteComment(long long userId, const std::string& role,
                                       long long commentId) {
    long long authorId = ReviewDao::commentAuthorId(commentId);
    if (authorId == 0) throw BizError(resp::NOT_FOUND, "评论不存在");
    if (userId != authorId && role != "admin")
        throw BizError(resp::FORBIDDEN, "只能删除自己发表的评论");
    long long deleted = ReviewDao::deleteCommentTree(commentId);
    if (deleted < 0) throw BizError(resp::SERVER_ERROR, "删除评论失败");
    return deleted;
}

void ReviewService::merchantReply(long long merchantUserId, long long reviewId,
                                  const std::string& content) {
    if (content.empty()) throw BizError(resp::PARAM_ERROR, "回复内容不能为空");
    if (content.size() > 500) throw BizError(resp::PARAM_ERROR, "回复过长（最多 500 字）");
    auto m = MerchantDao::byOwner(merchantUserId);
    if (m.is_null()) throw BizError(resp::CONFLICT, "尚未入驻商户");
    if (jsonStr(m, "status") != "approved")
        throw BizError(resp::FORBIDDEN, "商户尚未通过审核");
    long long myMerchantId = m.value("id", 0LL);

    auto r = ReviewDao::reviewById(reviewId);
    if (r.is_null()) throw BizError(resp::NOT_FOUND, "评价不存在");
    if (r.value("merchant_id", 0LL) != myMerchantId)
        throw BizError(resp::FORBIDDEN, "只能回复本商户收到的评价");

    auto reply = ReviewDao::getMerchantReply(reviewId);
    if (reply.is_null()) {
        if (ReviewDao::addMerchantReply(reviewId, myMerchantId, content) == 0)
            throw BizError(resp::SERVER_ERROR, "回复失败");
    } else {
        ReviewDao::updateMerchantReply(reply.value("id", 0LL), content);
    }
}

void ReviewService::reportReview(long long userId, long long reviewId,
                                 const std::string& reason) {
    if (reason.empty()) throw BizError(resp::PARAM_ERROR, "请填写举报原因");
    if (reason.size() > 200) throw BizError(resp::PARAM_ERROR, "举报原因过长");
    auto r = ReviewDao::reviewById(reviewId);
    if (r.is_null()) throw BizError(resp::NOT_FOUND, "评价不存在");
    if (ReviewDao::addReport(reviewId, userId, reason) == 0)
        throw BizError(resp::SERVER_ERROR, "举报提交失败");
}

nlohmann::json ReviewService::listReports(const std::string& status, int page, int size) {
    if (page < 1) page = 1;
    if (size < 1 || size > 100) size = 20;
    long long total = 0;
    auto rows = ReviewDao::listReports(status, page, size, total);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)}};
}

void ReviewService::handleReport(long long reportId, const std::string& action,
                                 const std::string& reason) {
    auto rep = ReviewDao::reportById(reportId);
    if (rep.is_null()) throw BizError(resp::NOT_FOUND, "举报记录不存在");

    if (action == "hide") {
        ReviewDao::updateStatus(rep.value("review_id", 0LL), "hidden");
        ReviewDao::setReportStatus(reportId, "resolved", "已下架该评价");
    } else if (action == "reject") {
        if (reason.empty()) throw BizError(resp::PARAM_ERROR, "请填写驳回原因");
        ReviewDao::setReportStatus(reportId, "rejected", reason);
    } else {
        throw BizError(resp::PARAM_ERROR, "处理动作仅支持 hide / reject");
    }
}

