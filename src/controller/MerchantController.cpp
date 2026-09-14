#include "controller/MerchantController.h"

#include <string>

#include "model/Models.h"
#include "server/Api.h"
#include "service/MerchantService.h"
#include "util/Json.h"

void MerchantController::registerRoutes(httplib::Server& svr) {
    // POST /api/merchant/apply 入驻申请（登录用户；consumer/被驳回商户）
    svr.Post("/api/merchant/apply", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!ctx) throw BizError(resp::UNAUTHORIZED, "未登录");
            if (ctx->role == "admin")
                throw BizError(resp::FORBIDDEN, "平台管理员账号不能申请入驻，请使用普通用户身份提交");
            sendOk(res, MerchantService::apply(ctx->userId, parseBody(req)));
        }, res);
    });

    // GET /api/merchant/me 我的商户（资料 + 门店 + 服务 + 套餐）
    svr.Get("/api/merchant/me", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            sendOk(res, MerchantService::getMyMerchant(ctx->userId));
        }, res);
    });

    // PUT /api/merchant/me 更新我的商户资料
    svr.Put("/api/merchant/me", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            MerchantService::updateMyMerchant(ctx->userId, parseBody(req));
            sendOk(res);
        }, res);
    });

    // ---- 门店 ----
    svr.Get("/api/merchant/stores", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            sendOk(res, MerchantService::listStores(ctx->userId));
        }, res);
    });
    svr.Post("/api/merchant/stores", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            sendOk(res, MerchantService::addStore(ctx->userId, parseBody(req)));
        }, res);
    });
    svr.Put(R"(/api/merchant/stores/(\d+))", [](const httplib::Request& req,
                                                httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            MerchantService::updateStore(ctx->userId, std::stoll(req.matches[1].str()),
                                         parseBody(req));
            sendOk(res);
        }, res);
    });
    svr.Delete(R"(/api/merchant/stores/(\d+))", [](const httplib::Request& req,
                                                    httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            MerchantService::deleteStore(ctx->userId, std::stoll(req.matches[1].str()));
            sendOk(res);
        }, res);
    });

    // ---- 服务项目 ----
    svr.Get("/api/merchant/services", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            sendOk(res, MerchantService::listServices(ctx->userId, req.get_param_value("status")));
        }, res);
    });
    svr.Post("/api/merchant/services", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            sendOk(res, MerchantService::addService(ctx->userId, parseBody(req)));
        }, res);
    });
    svr.Put(R"(/api/merchant/services/(\d+))", [](const httplib::Request& req,
                                                  httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            MerchantService::updateService(ctx->userId, std::stoll(req.matches[1].str()),
                                           parseBody(req));
            sendOk(res);
        }, res);
    });
    svr.Put(R"(/api/merchant/services/(\d+)/status)", [](const httplib::Request& req,
                                                          httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            MerchantService::setServiceStatus(ctx->userId, std::stoll(req.matches[1].str()),
                                              jsonStr(parseBody(req), "status"));
            sendOk(res);
        }, res);
    });

    // ---- 消费套餐 ----
    svr.Get("/api/merchant/packages", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            sendOk(res, MerchantService::listPackages(ctx->userId, req.get_param_value("status")));
        }, res);
    });
    svr.Post("/api/merchant/packages", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            sendOk(res, MerchantService::addPackage(ctx->userId, parseBody(req)));
        }, res);
    });
    svr.Put(R"(/api/merchant/packages/(\d+))", [](const httplib::Request& req,
                                                   httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            MerchantService::updatePackage(ctx->userId, std::stoll(req.matches[1].str()),
                                           parseBody(req));
            sendOk(res);
        }, res);
    });
    svr.Put(R"(/api/merchant/packages/(\d+)/status)", [](const httplib::Request& req,
                                                          httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "merchant", res)) return;
            MerchantService::setPackageStatus(ctx->userId, std::stoll(req.matches[1].str()),
                                              jsonStr(parseBody(req), "status"));
            sendOk(res);
        }, res);
    });
}
