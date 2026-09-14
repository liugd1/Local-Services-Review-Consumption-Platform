#include "controller/SearchController.h"

#include <algorithm>
#include <string>

#include "server/Api.h"
#include "service/SearchService.h"
#include "util/Json.h"

void SearchController::registerRoutes(httplib::Server& svr) {
    // GET /api/search?keyword=&category=&area=&price_min=&price_max=&min_score=&sort=&page=&size=
    svr.Get("/api/search", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto param = [&req](const char* key) { return req.get_param_value(key); };
            auto num = [&param](const char* key, double def) {
                auto v = param(key);
                if (v.empty()) return def;
                try {
                    return std::stod(v);
                } catch (...) {
                    return def;
                }
            };
            auto page = [&param]() {
                auto v = param("page");
                try {
                    return std::max(1, std::stoi(v.empty() ? "1" : v));
                } catch (...) {
                    return 1;
                }
            }();
            auto size = [&param]() {
                auto v = param("size");
                try {
                    return std::min(50, std::max(1, std::stoi(v.empty() ? "10" : v)));
                } catch (...) {
                    return 10;
                }
            }();
            long long categoryId = 0;
            auto cv = param("category");
            if (!cv.empty()) {
                try {
                    categoryId = std::stoll(cv);
                } catch (...) {
                    categoryId = 0;
                }
            }

            sendOk(res, SearchService::search(param("keyword"), categoryId, param("area"),
                                              num("price_min", 0), num("price_max", 0),
                                              num("min_score", 0), param("sort"), page, size));
        }, res);
    });

    // GET /api/merchants/{id} 商户详情（可选登录：所有者/管理员可见未上架状态）
    svr.Get(R"(/api/merchants/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);  // 可选鉴权
            long long viewerId = ctx ? ctx->userId : 0;
            std::string role = ctx ? ctx->role : "";
            sendOk(res, SearchService::detail(std::stoll(req.matches[1].str()), viewerId, role));
        }, res);
    });

    // GET /api/stores/{id} 门店详情（消费者选购入口：本店在售服务/套餐/活动 + 门店口碑）
    svr.Get(R"(/api/stores/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);  // 可选鉴权
            long long viewerId = ctx ? ctx->userId : 0;
            std::string role = ctx ? ctx->role : "";
            sendOk(res, SearchService::storeDetail(std::stoll(req.matches[1].str()), viewerId, role));
        }, res);
    });

    // 榜单
    svr.Get("/api/rank/hot", [](const httplib::Request&, httplib::Response& res) {
        guard([&] { sendOk(res, SearchService::hotRank()); }, res);
    });
    svr.Get("/api/rank/new", [](const httplib::Request&, httplib::Response& res) {
        guard([&] { sendOk(res, SearchService::newRank()); }, res);
    });
    svr.Get(R"(/api/rank/category/(\d+))", [](const httplib::Request& req,
                                              httplib::Response& res) {
        guard([&] { sendOk(res, SearchService::categoryRank(std::stoll(req.matches[1].str()))); },
              res);
    });
}
