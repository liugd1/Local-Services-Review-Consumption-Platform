#include "controller/CouponController.h"

#include <string>

#include "model/Models.h"
#include "server/Api.h"
#include "service/CouponService.h"
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
long long idOf(const httplib::Request& req) { return std::stoll(req.path_params.at("id")); }
}  // namespace

void CouponController::registerRoutes(httplib::Server& svr) {
    // ---- 商户端：优惠活动管理 ----
    svr.Get("/api/merchant/coupons", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "merchant", [&](const api::AuthCtx& ctx) {
                sendOk(res, CouponService::listByMerchant(ctx.userId));
            });
        }, res);
    });
    svr.Post("/api/merchant/coupons", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "merchant", [&](const api::AuthCtx& ctx) {
                sendOk(res, CouponService::create(ctx.userId, parseBody(req)));
            });
        }, res);
    });
    svr.Put("/api/merchant/coupons/:id/status", [](const httplib::Request& req,
                                                    httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "merchant", [&](const api::AuthCtx& ctx) {
                auto body = parseBody(req);
                CouponService::setStatus(ctx.userId, idOf(req), jsonStr(body, "status"));
                sendOk(res);
            });
        }, res);
    });

    // 商户核销（扫码模拟：输入券码）
    svr.Post("/api/merchant/coupon/verify", [](const httplib::Request& req,
                                               httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "merchant", [&](const api::AuthCtx& ctx) {
                auto body = parseBody(req);
                sendOk(res, CouponService::verify(ctx.userId, jsonStr(body, "code")));
            });
        }, res);
    });

    // ---- 消费者 / 公开 ----
    // 某商户当前可领券列表（公开，用于商户详情页展示）
    svr.Get("/api/coupons", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            long long merchantId = 0;
            auto mv = req.get_param_value("merchant_id");
            if (!mv.empty()) merchantId = std::stoll(mv);
            if (merchantId <= 0) throw BizError(resp::PARAM_ERROR, "缺少商户参数");
            sendOk(res, CouponService::listPublished(merchantId));
        }, res);
    });

    // 领取优惠券
    svr.Post("/api/coupon/:id/receive", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                sendOk(res, CouponService::receive(ctx.userId, idOf(req)));
            });
        }, res);
    });

    // 我的卡券（?status=unused/used/all）
    svr.Get("/api/my/coupons", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                sendOk(res, CouponService::myClaims(ctx.userId, req.get_param_value("status")));
            });
        }, res);
    });
}
