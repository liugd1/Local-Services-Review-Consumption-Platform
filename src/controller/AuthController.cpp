#include "controller/AuthController.h"

#include "model/Models.h"
#include "server/Api.h"
#include "service/UserService.h"
#include "util/Json.h"

void AuthController::registerRoutes(httplib::Server& svr) {
    // POST /api/auth/register 用户注册（默认 consumer 角色）
    svr.Post("/api/auth/register", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto body = parseBody(req);
            auto user = UserService::registerUser(jsonStr(body, "username"),
                                                  jsonStr(body, "password"),
                                                  jsonStr(body, "nickname"));
            sendOk(res, nlohmann::json{{"user", user}});
        }, res);
    });

    // POST /api/auth/login 登录，返回 token
    svr.Post("/api/auth/login", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto body = parseBody(req);
            auto data = UserService::login(jsonStr(body, "username"), jsonStr(body, "password"));
            sendOk(res, data);
        }, res);
    });

    // POST /api/auth/logout 登出
    svr.Post("/api/auth/logout", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            if (!api::authenticate(req)) throw BizError(resp::UNAUTHORIZED, "未登录");
            UserService::logout(api::bearerToken(req));
            sendOk(res);
        }, res);
    });

    // GET /api/user/profile 个人资料（含消费偏好）
    svr.Get("/api/user/profile", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!ctx) throw BizError(resp::UNAUTHORIZED, "未登录");
            sendOk(res, UserService::getProfile(ctx->userId));
        }, res);
    });

    // PUT /api/user/profile 修改个人资料（nickname / phone / avatar）
    svr.Put("/api/user/profile", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!ctx) throw BizError(resp::UNAUTHORIZED, "未登录");
            UserService::updateProfile(ctx->userId, parseBody(req));
            sendOk(res);
        }, res);
    });

    // PUT /api/user/preferences 设置消费偏好
    svr.Put("/api/user/preferences", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!ctx) throw BizError(resp::UNAUTHORIZED, "未登录");
            UserService::updatePreferences(ctx->userId, parseBody(req));
            sendOk(res);
        }, res);
    });
}
