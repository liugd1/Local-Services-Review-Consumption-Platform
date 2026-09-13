#include "controller/InteractionController.h"

#include <functional>

#include "model/Models.h"
#include "server/Api.h"
#include "service/InteractionService.h"
#include "util/Json.h"

void InteractionController::registerRoutes(httplib::Server& svr) {
    // 通用：需消费者登录
    auto requireConsumer = [](const httplib::Request& req, httplib::Response& res,
                              const std::function<void(const api::AuthCtx&)>& fn) {
        auto ctx = api::authenticate(req);
        if (!ctx) {
            sendUnauthorized(res);
            return;
        }
        if (ctx->role != "consumer") {
            sendForbidden(res);
            return;
        }
        fn(*ctx);
    };

    // ---- 收藏 ----
    svr.Get("/api/my/favorites", [&](const httplib::Request& req, httplib::Response& res) {
        requireConsumer(req, res, [&](const api::AuthCtx& ctx) {
            auto type = req.get_param_value("type");
            if (type.empty()) type = "merchant";
            sendOk(res, InteractionService::listFavorites(ctx.userId, type));
        });
    });
    svr.Post("/api/favorite", [&](const httplib::Request& req, httplib::Response& res) {
        requireConsumer(req, res, [&](const api::AuthCtx& ctx) {
            auto body = parseBody(req);
            sendOk(res, InteractionService::addFavorite(ctx.userId, jsonStr(body, "target_type", "merchant"),
                                                        jsonInt(body, "target_id")));
        });
    });
    svr.Delete("/api/favorite", [&](const httplib::Request& req, httplib::Response& res) {
        requireConsumer(req, res, [&](const api::AuthCtx& ctx) {
            auto body = parseBody(req);
            InteractionService::removeFavorite(ctx.userId, jsonStr(body, "target_type", "merchant"),
                                               jsonInt(body, "target_id"));
            sendOk(res);
        });
    });

    // ---- 关注 ----
    svr.Get("/api/my/follows", [&](const httplib::Request& req, httplib::Response& res) {
        requireConsumer(req, res, [&](const api::AuthCtx& ctx) {
            sendOk(res, InteractionService::listFollows(ctx.userId));
        });
    });
    svr.Post("/api/follow", [&](const httplib::Request& req, httplib::Response& res) {
        requireConsumer(req, res, [&](const api::AuthCtx& ctx) {
            InteractionService::addFollow(ctx.userId, jsonInt(parseBody(req), "merchant_id"));
            sendOk(res);
        });
    });
    svr.Delete("/api/follow", [&](const httplib::Request& req, httplib::Response& res) {
        requireConsumer(req, res, [&](const api::AuthCtx& ctx) {
            InteractionService::removeFollow(ctx.userId, jsonInt(parseBody(req), "merchant_id"));
            sendOk(res);
        });
    });

    // ---- 浏览历史 ----
    svr.Get("/api/my/history", [&](const httplib::Request& req, httplib::Response& res) {
        requireConsumer(req, res, [&](const api::AuthCtx& ctx) {
            sendOk(res, InteractionService::listHistory(ctx.userId));
        });
    });

    // ---- 消费记录 ----
    svr.Post("/api/consume", [&](const httplib::Request& req, httplib::Response& res) {
        requireConsumer(req, res, [&](const api::AuthCtx& ctx) {
            InteractionService::addConsumption(ctx.userId, parseBody(req));
            sendOk(res);
        });
    });
    svr.Get("/api/my/consumptions", [&](const httplib::Request& req, httplib::Response& res) {
        requireConsumer(req, res, [&](const api::AuthCtx& ctx) {
            auto getInt = [&](const char* key, int def) {
                auto v = req.get_param_value(key);
                try {
                    return std::stoi(v);
                } catch (...) {
                    return def;
                }
            };
            sendOk(res, InteractionService::listConsumptions(ctx.userId, getInt("page", 1),
                                                             getInt("size", 20)));
        });
    });
}
