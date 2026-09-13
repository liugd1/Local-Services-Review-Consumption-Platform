#pragma once
// 评分评价业务（评价对象化：merchant / store / service / package）
#include <nlohmann/json.hpp>
#include <string>

class ReviewService {
public:
    // 发表评价（可对店铺/门店/服务/套餐点评，含图片）。一对象一评。
    static nlohmann::json postReview(long long userId, const nlohmann::json& body);

    // 商户店内评价列表（filterType 空=全部；用于详情评论 tab）
    static nlohmann::json listMerchantReviews(long long merchantId,
                                              const std::string& filterType, int page, int size);
    // 商户内各对象类型的评论汇总（chips：类型计数与均分）
    static nlohmann::json merchantReviewSummary(long long merchantId);

    // 某目标对象独立评论区 + 汇总
    static nlohmann::json listTargetReviews(const std::string& type, long long targetId, int page,
                                            int size);
    // 对象头部信息（名称 / 归属商户），供评论区页使用
    static nlohmann::json objectInfo(const std::string& type, long long id);

    static nlohmann::json listMyReviews(long long userId, int page, int size);
    // 删除评价：作者本人或平台管理员（admin 可删任意）；软删并级联清除其全部评论
    static long long deleteReview(long long userId, const std::string& role, long long reviewId);

    // 点赞
    static nlohmann::json toggleLike(long long userId, long long reviewId, bool like);

    // 评论：parentId>0 表示对某条评论追加评论
    static void commentReview(long long userId, long long reviewId, const std::string& content,
                              long long parentId);
    static nlohmann::json listComments(long long reviewId, long long viewerId = 0);
    // 删除评论：作者本人或平台管理员；主评论删除时级联删除全部子评论，返回删除条数
    static long long deleteComment(long long userId, const std::string& role, long long commentId);
    // 评论点赞（每条评论都可“有用”）
    static nlohmann::json commentLikeToggle(long long userId, long long commentId, bool like);
    // 举报某条评论
    static void reportComment(long long userId, long long commentId, const std::string& reason);
    // 平台处理评论举报
    static nlohmann::json listCommentReports(const std::string& status, int page, int size);
    static void handleCommentReport(long long reportId, const std::string& action,
                                    const std::string& reason);

    // 商户回复评价（面向其商户收到的任意评价）
    static void merchantReply(long long merchantUserId, long long reviewId,
                              const std::string& content);

    // 举报与平台审核
    static void reportReview(long long userId, long long reviewId, const std::string& reason);
    static nlohmann::json listReports(const std::string& status, int page, int size);
    static void handleReport(long long reportId, const std::string& action,
                             const std::string& reason);
};
