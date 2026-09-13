#include "controller/ReviewController.h"

#include <string>

#include "model/Models.h"
#include "server/Api.h"
#include "service/ReviewService.h"
#include "util/Json.h"

namespace {

bool needLogin(const httplib::Request& req, httplib::Response& res, const std::string& role,
               const std::function<void(const api::AuthCtx&)>& fn) {
    auto ctx = api::authenticate(req);
    if (!ctx) {
        sendUnauthorized(res);
        return false;
    }
    if (!role.empty() && ctx->role != role) {
        sendForbidden(res);
        return false;
    }
    fn(*ctx);
    return true;
}

int intParam(const httplib::Request& req, const char* key, int def, int lo, int hi) {
    auto v = req.get_param_value(key);
    try {
        int r = std::stoi(v);
        return r < lo ? lo : (r > hi ? hi : r);
    } catch (...) {
        return def;
    }
}

long long idOf(const httplib::Request& req, const char* key = "id") {
    return std::stoll(req.path_params.at(key));
}

}  // namespace

void ReviewController::registerRoutes(httplib::Server& svr) {
    // 商户店内评价（filter: type= 全部/merchant/store/service/package）
    svr.Get("/api/merchants/:id/reviews", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            sendOk(res, ReviewService::listMerchantReviews(
                            idOf(req), req.get_param_value("type"),
                            intParam(req, "page", 1, 1, 100000), intParam(req, "size", 10, 1, 50)));
        }, res);
    });

    // 商户店内各对象类型评价汇总（评论区 tab 徽标）
    svr.Get("/api/merchants/:id/reviews/summary", [](const httplib::Request& req,
                                                     httplib::Response& res) {
        guard([&] {
            sendOk(res, ReviewService::merchantReviewSummary(idOf(req)));
        }, res);
    });

    // 目标对象独立评论区（店铺/门店/服务/套餐）
    svr.Get("/api/targets/:type/:id/reviews", [](const httplib::Request& req,
                                                 httplib::Response& res) {
        guard([&] {
            sendOk(res, ReviewService::listTargetReviews(
                            req.path_params.at("type"), idOf(req),
                            intParam(req, "page", 1, 1, 100000), intParam(req, "size", 10, 1, 50)));
        }, res);
    });

    // 评价对象头部信息（名称 / 归属商户 / 汇总）
    svr.Get("/api/targets/:type/:id/info", [](const httplib::Request& req,
                                              httplib::Response& res) {
        guard([&] {
            sendOk(res, ReviewService::objectInfo(req.path_params.at("type"), idOf(req)));
        }, res);
    });

    // 我的评价（消费者）
    svr.Get("/api/my/reviews", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                sendOk(res, ReviewService::listMyReviews(
                                ctx.userId, intParam(req, "page", 1, 1, 100000),
                                intParam(req, "size", 10, 1, 50)));
            });
        }, res);
    });

    // POST /api/review 发表评价（target_type 支持 merchant/store/service/package）
    svr.Post("/api/review", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                sendOk(res, ReviewService::postReview(ctx.userId, parseBody(req)));
            });
        }, res);
    });

    // DELETE /api/review/:id 删除评价（作者本人或平台管理员；级联清除其全部评论）
    svr.Delete("/api/review/:id", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "", [&](const api::AuthCtx& ctx) {
                long long n = ReviewService::deleteReview(ctx.userId, ctx.role, idOf(req));
                sendOk(res, {{"deleted_comments", n}});
            });
        }, res);
    });

    // 点赞 / 取消（登录即可参与：消费者与商户经营者）
    svr.Post("/api/review/:id/like", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "", [&](const api::AuthCtx& ctx) {
                sendOk(res, ReviewService::toggleLike(ctx.userId, idOf(req), true));
            });
        }, res);
    });
    svr.Delete("/api/review/:id/like", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "", [&](const api::AuthCtx& ctx) {
                sendOk(res, ReviewService::toggleLike(ctx.userId, idOf(req), false));
            });
        }, res);
    });

    // 评论：body {content, parent_id?}（消费者/商户经营者均可参与评论与回复）
    svr.Post("/api/review/:id/comment", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "", [&](const api::AuthCtx& ctx) {
                auto body = parseBody(req);
                ReviewService::commentReview(ctx.userId, idOf(req), jsonStr(body, "content"),
                                             jsonInt(body, "parent_id"));
                sendOk(res);
            });
        }, res);
    });

    // 评论列表（含楼中楼平铺；登录后返回各评论是否已被我“有用”）
    svr.Get("/api/review/:id/comments", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);  // 可选登录
            sendOk(res, ReviewService::listComments(idOf(req), ctx ? ctx->userId : 0));
        }, res);
    });

    // 删除评论：作者本人或平台管理员（任意登录用户可调，越权 403）；主评论级联删除子评论
    svr.Delete("/api/comments/:id", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "", [&](const api::AuthCtx& ctx) {
                long long n = ReviewService::deleteComment(ctx.userId, ctx.role, idOf(req));
                sendOk(res, {{"deleted", n}});
            });
        }, res);
    });

    // 评论“有用”点赞：每条评论都可被点赞/取消
    svr.Post("/api/comments/:id/like", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "", [&](const api::AuthCtx& ctx) {
                sendOk(res, ReviewService::commentLikeToggle(ctx.userId, idOf(req), true));
            });
        }, res);
    });
    svr.Delete("/api/comments/:id/like", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "", [&](const api::AuthCtx& ctx) {
                sendOk(res, ReviewService::commentLikeToggle(ctx.userId, idOf(req), false));
            });
        }, res);
    });

    // 举报某条评论
    svr.Post("/api/comments/:id/report", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "", [&](const api::AuthCtx& ctx) {
                auto body = parseBody(req);
                ReviewService::reportComment(ctx.userId, idOf(req), jsonStr(body, "reason"));
                sendOk(res);
            });
        }, res);
    });

    // 举报评价（消费者/商户经营者均可）
    svr.Post("/api/review/:id/report", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "", [&](const api::AuthCtx& ctx) {
                auto body = parseBody(req);
                ReviewService::reportReview(ctx.userId, idOf(req), jsonStr(body, "reason"));
                sendOk(res);
            });
        }, res);
    });

    // 商户回复评价
    svr.Post("/api/merchant/review/:id/reply", [](const httplib::Request& req,
                                                  httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "merchant", [&](const api::AuthCtx& ctx) {
                auto body = parseBody(req);
                ReviewService::merchantReply(ctx.userId, idOf(req), jsonStr(body, "content"));
                sendOk(res);
            });
        }, res);
    });

    // ---- 平台举报审核 ----
    svr.Get("/api/admin/reports", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "admin", [&](const api::AuthCtx&) {
                sendOk(res, ReviewService::listReports(req.get_param_value("status"),
                                                       intParam(req, "page", 1, 1, 100000),
                                                       intParam(req, "size", 20, 1, 100)));
            });
        }, res);
    });
    svr.Put("/api/admin/reports/:id", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "admin", [&](const api::AuthCtx&) {
                auto body = parseBody(req);
                ReviewService::handleReport(idOf(req), jsonStr(body, "action"),
                                            jsonStr(body, "reason"));
                sendOk(res);
            });
        }, res);
    });

    // ---- 评论举报（平台）----
    svr.Get("/api/admin/comment-reports", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "admin", [&](const api::AuthCtx&) {
                sendOk(res, ReviewService::listCommentReports(req.get_param_value("status"),
                                                              intParam(req, "page", 1, 1, 100000),
                                                              intParam(req, "size", 20, 1, 100)));
            });
        }, res);
    });
    svr.Put("/api/admin/comment-reports/:id", [](const httplib::Request& req,
                                                 httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "admin", [&](const api::AuthCtx&) {
                auto body = parseBody(req);
                ReviewService::handleCommentReport(idOf(req), jsonStr(body, "action"),
                                                   jsonStr(body, "reason"));
                sendOk(res);
            });
        }, res);
    });
}

