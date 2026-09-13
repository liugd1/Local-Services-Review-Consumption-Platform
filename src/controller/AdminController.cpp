#include "controller/AdminController.h"

#include "model/Models.h"
#include "server/Api.h"
#include "service/MerchantService.h"
#include "service/UserService.h"
#include "util/Json.h"

namespace {

// 解析分页等整型查询参数
int intParam(const httplib::Request& req, const char* key, int def, int minV, int maxV) {
    auto v = req.get_param_value(key);
    if (v.empty()) return def;
    try {
        int r = std::stoi(v);
        return r < minV ? minV : (r > maxV ? maxV : r);
    } catch (...) {
        return def;
    }
}

}  // namespace

void AdminController::registerRoutes(httplib::Server& svr) {
    // GET /api/admin/users?page=&size= 用户分页列表（admin）
    svr.Get("/api/admin/users", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "admin", res)) return;
            int page = intParam(req, "page", 1, 1, 100000);
            int size = intParam(req, "size", 20, 1, 100);
            sendOk(res, UserService::listUsers(page, size));
        }, res);
    });

    // PUT /api/admin/users/{id}/status  启用/禁用用户  body: {"status":"active"|"disabled"}
    svr.Put(R"(/api/admin/users/(\d+)/status)", [](const httplib::Request& req,
                                                   httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "admin", res)) return;

            long long targetId = 0;
            try {
                targetId = std::stoll(req.matches[1].str());
            } catch (...) {
                throw BizError(resp::PARAM_ERROR, "用户 id 非法");
            }
            auto body = parseBody(req);
            UserService::setUserStatus(targetId, jsonStr(body, "status"));
            sendOk(res);
        }, res);
    });

    // ---- 商户入驻审核 ----
    // GET /api/admin/merchants?status=&keyword=&page=&size=
    svr.Get("/api/admin/merchants", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "admin", res)) return;
            int page = intParam(req, "page", 1, 1, 100000);
            int size = intParam(req, "size", 20, 1, 100);
            sendOk(res, MerchantService::adminList(req.get_param_value("status"),
                                                   req.get_param_value("keyword"), page, size));
        }, res);
    });

    // GET /api/admin/merchants/counts 各审核状态数量
    svr.Get("/api/admin/merchants/counts", [](const httplib::Request& req,
                                              httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "admin", res)) return;
            sendOk(res, MerchantService::adminCounts());
        }, res);
    });

    // PUT /api/admin/merchants/{id}/audit 审核  body: {"status":"approved|rejected","reason":""}
    svr.Put(R"(/api/admin/merchants/(\d+)/audit)", [](const httplib::Request& req,
                                                       httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!requireRole(ctx, "admin", res)) return;
            MerchantService::audit(std::stoll(req.matches[1].str()), parseBody(req));
            sendOk(res);
        }, res);
    });
}
