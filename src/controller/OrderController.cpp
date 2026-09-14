#include "controller/OrderController.h"

#include "model/Models.h"
#include "server/Api.h"
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

    // POST /api/store/{id}/order 在指定门店下单（item_type: service / package）
    svr.Post(R"(/api/store/(\d+)/order)", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            needLogin(req, res, "consumer", [&](const api::AuthCtx& ctx) {
                auto body = parseBody(req);
                sendOk(res, OrderService::buyAtStore(ctx.userId, std::stoll(req.matches[1].str()),
                                                     jsonStr(body, "item_type"),
                                                     jsonInt(body, "item_id")));
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
}
