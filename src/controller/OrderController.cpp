#include "controller/OrderController.h"

#include "dao/MerchantDao.h"
#include "model/Models.h"
#include "server/Api.h"
#include "service/CouponService.h"
#include "service/OrderService.h"
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
int intParam(const httplib::Request& req, const char* key, int def, int lo, int hi) {
    auto v = req.get_param_value(key);
    try {
        int r = std::stoi(v);
        return r < lo ? lo : (r > hi ? hi : r);
    } catch (...) {
        return def;
    }
}
}  // namespace

void OrderController::registerRoutes(httplib::Server& svr) {
    // 购买套餐
    svr.Post("/api/package/:id/buy", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                sendOk(res, OrderService::buy(ctx.userId, idOf(req)));
            });
        }, res);
    });

    // POST /api/store/{id}/order 在指定门店下单（item_type: service / package；coupon_claim_id 可选）
    svr.Post(R"(/api/store/(\d+)/order)", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                auto body = parseBody(req);
                sendOk(res, OrderService::buyAtStore(ctx.userId, std::stoll(req.matches[1].str()),
                                                     jsonStr(body, "item_type"),
                                                     jsonInt(body, "item_id"),
                                                     jsonInt(body, "coupon_claim_id")));
            });
        }, res);
    });

    // GET /api/store/{id}/coupons/usable 该门店某项目可用的优惠券（消费者）
    svr.Get(R"(/api/store/(\d+)/coupons/usable)",
            [](const httplib::Request& req, httplib::Response& res) {
                guard([&] {
                    needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                        std::string itemType = req.get_param_value("item_type");
                        long long itemId = 0;
                        try {
                            itemId = std::stoll(req.get_param_value("item_id"));
                        } catch (...) {
                            throw BizError(resp::PARAM_ERROR, "缺少 item_id");
                        }
                        double amt = 0;
                        try {
                            amt = std::stod(req.get_param_value("amount"));
                        } catch (...) {
                            amt = 0;
                        }
                        // 未显式传金额时，按项目原价计算
                        if (amt <= 0) {
                            if (itemType == "package") {
                                auto p = MerchantDao::packageById(itemId);
                                if (!p.is_null()) amt = p.value("price", 0.0);
                            } else {
                                auto s = MerchantDao::serviceById(itemId);
                                if (!s.is_null()) amt = s.value("price", 0.0);
                            }
                        }
                        sendOk(res, CouponService::usableFor(ctx.userId,
                                                             std::stoll(req.matches[1].str()),
                                                             itemType, itemId, amt));
                    });
                }, res);
            });

    // 我的订单（?status=purchased/used/refunded/all）
    svr.Get("/api/my/orders", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                sendOk(res, OrderService::myOrders(ctx.userId, req.get_param_value("status"),
                                                   intParam(req, "page", 1, 1, 100000),
                                                   intParam(req, "size", 20, 1, 100)));
            });
        }, res);
    });

    // 使用 / 核销套餐（联动生成消费记录）
    svr.Post("/api/order/:id/use", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                OrderService::useOrder(ctx.userId, idOf(req));
                sendOk(res);
            });
        }, res);
    });

    // 退款（仅未使用）
    svr.Post("/api/order/:id/refund", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                OrderService::refundOrder(ctx.userId, idOf(req));
                sendOk(res);
            });
        }, res);
    });

    // 删除订单（软删除；待使用订单不允许删除）
    svr.Delete(R"(/api/my/order/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                OrderService::deleteOrder(ctx.userId, std::stoll(req.matches[1].str()));
                sendOk(res);
            });
        }, res);
    });
}
