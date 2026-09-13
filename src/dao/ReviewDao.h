#pragma once
// 评价 / 点赞 / 评论(含楼中楼) / 回复 / 举报 数据访问
// 评价对象化：target_type ∈ {merchant, store, service, package}
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace ReviewDao {

// 新增评价（r 需含 user_id/merchant_id/target_type/target_id 与三维分）返回 id
long long addReview(const nlohmann::json& r);
bool addReviewImages(long long reviewId, const std::vector<std::string>& paths);
nlohmann::json reviewById(long long id);  // 行对象（含作者/互动统计/images），无则 nullptr
// 用户对某对象已点评数（一对象一评）
long long countUserVisible(long long userId, const std::string& type, long long targetId);
bool updateStatus(long long reviewId, const std::string& status);

// 某目标对象独立评论区（talk 页 / 对象详情）
nlohmann::json listTargetReviews(const std::string& type, long long targetId, int page, int size,
                                 long long& total);
// 某目标对象的汇总（评论数与均分）
nlohmann::json targetSummary(const std::string& type, long long targetId);
// 某商户店内评价列表（可按对象类型过滤，用于店铺评论 tab）
nlohmann::json listMerchantReviews(long long merchantId, const std::string& filterType, int page,
                                   int size, long long& total);
// 我的评价
nlohmann::json listByUser(long long userId, int page, int size, long long& total);
// 商户内按对象类型汇总（计数与均分）
nlohmann::json summaryByMerchant(long long merchantId);

// 评论：parentId>0 表示对某条评论追加回复
long long addComment(long long reviewId, long long userId, const std::string& content,
                     long long parentId);
nlohmann::json listComments(long long reviewId, long long viewerId = 0);  // 含 like_count / viewer 是否已赞
// 校验某评论属于某评价（供楼中楼回复）
bool commentBelongsTo(long long reviewId, long long commentId);
// 评论作者 id（不存在返回 0）
long long commentAuthorId(long long commentId);
// 删除该评论及其全部后代（递归），返回删除条数
long long deleteCommentTree(long long commentId);
// 删除某评价的全部评论（连带附属），返回删除评论条数
long long deleteCommentsOfReview(long long reviewId);

// 评论点赞（每条评论“有用”）
bool commentLikeAdd(long long commentId, long long userId);
bool commentLikeRemove(long long commentId, long long userId);
bool isCommentLiked(long long commentId, long long userId);
long long countCommentLikes(long long commentId);

// 评论举报
long long addCommentReport(long long commentId, long long userId, const std::string& reason);
nlohmann::json listCommentReports(const std::string& status, int page, int size, long long& total);
nlohmann::json commentReportById(long long id);
bool setCommentReportStatus(long long id, const std::string& status, const std::string& result);

// 点赞
bool likeAdd(long long reviewId, long long userId);
bool likeRemove(long long reviewId, long long userId);

// 商户对评价的回复（comment_id IS NULL 表示对评价整体回复）
nlohmann::json getMerchantReply(long long reviewId);
long long addMerchantReply(long long reviewId, long long merchantId, const std::string& content);
bool updateMerchantReply(long long replyId, const std::string& content);

// 举报
long long addReport(long long reviewId, long long userId, const std::string& reason);
nlohmann::json reportById(long long id);
nlohmann::json listReports(const std::string& status, int page, int size, long long& total);
bool setReportStatus(long long id, const std::string& status, const std::string& result);

}  // namespace ReviewDao
