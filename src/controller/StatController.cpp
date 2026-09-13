#include "controller/StatController.h"

#include "server/Api.h"
#include "service/StatService.h"
#include "util/Json.h"

void StatController::registerRoutes(httplib::Server& svr) {
    // 商户经营看板（经营者）
    svr.Get("/api/merchant/stats", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!ctx) {
                sendUnauthorized(res);
                return;
            }
            if (ctx->role != "merchant") {
                sendForbidden(res);
                return;
            }
            sendOk(res, StatService::merchantStats(ctx->userId));
        }, res);
    });

    // 平台运营看板（管理员）
    svr.Get("/api/admin/stats", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!ctx) {
                sendUnauthorized(res);
                return;
            }
            if (ctx->role != "admin") {
                sendForbidden(res);
                return;
            }
            sendOk(res, StatService::platformStats());
        }, res);
    });
}

