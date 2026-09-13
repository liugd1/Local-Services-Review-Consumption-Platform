#pragma once
// API 通用层：鉴权中间件、统一响应工具、路由注册入口
#include <httplib.h>

#include <optional>
#include <string>

#include "util/BizError.h"

namespace api {

// 请求鉴权上下文（token 校验通过后）
struct AuthCtx {
    long long userId = 0;
    std::string username;
    std::string role;
    std::string nickname;
    bool valid() const { return userId > 0; }
};

// 取 Authorization: Bearer <token>
std::string bearerToken(const httplib::Request& req);

// 鉴权中间件：校验会话有效性（token 存在、未过期、用户启用）
std::optional<AuthCtx> authenticate(const httplib::Request& req);

// 全部业务路由注册入口（各 Controller::registerRoutes 汇聚于此）
void registerRoutes(httplib::Server& svr);

}  // namespace api

// ---------------- 统一 JSON 响应工具 ----------------
inline void sendJson(httplib::Response& res, int httpStatus, const nlohmann::json& body) {
    res.status = httpStatus;
    res.set_content(body.dump(), "application/json; charset=utf-8");
}

inline void sendOk(httplib::Response& res, nlohmann::json data = nullptr) {
    sendJson(res, 200, resp::ok(std::move(data)));
}

inline void sendBizError(httplib::Response& res, const BizError& e) {
    int http = (e.code >= 400 && e.code < 600) ? e.code : 400;
    sendJson(res, http, resp::err(e.code, e.what()));
}

inline void sendUnauthorized(httplib::Response& res, const std::string& msg = "未登录或会话已过期") {
    sendJson(res, 401, resp::err(resp::UNAUTHORIZED, msg));
}

inline void sendForbidden(httplib::Response& res, const std::string& msg = "无权限执行此操作") {
    sendJson(res, 403, resp::err(resp::FORBIDDEN, msg));
}

// 登录 + 角色检查：通过则执行 handler(ctx)，否则直接响应 401/403
inline bool requireRole(const std::optional<api::AuthCtx>& ctx, const std::string& role,
                        httplib::Response& res) {
    if (!ctx) {
        sendUnauthorized(res);
        return false;
    }
    if (ctx->role != role) {
        sendForbidden(res);
        return false;
    }
    return true;
}

// 业务异常保护：所有路由 handler 用其包裹，统一异常转 JSON
inline void guard(const std::function<void()>& fn, httplib::Response& res) {
    try {
        fn();
    } catch (const BizError& e) {
        sendBizError(res, e);
    } catch (const nlohmann::json::exception& e) {
        sendJson(res, 400, resp::err(resp::PARAM_ERROR, std::string("请求体格式错误：") + e.what()));
    } catch (const std::exception& e) {
        sendJson(res, 500, resp::err(resp::SERVER_ERROR, std::string("服务器错误：") + e.what()));
    }
}

// 解析 JSON 请求体（空请求体视为空对象；格式错误抛 json 异常由 guard 处理）
inline nlohmann::json parseBody(const httplib::Request& req) {
    if (req.body.empty()) return nlohmann::json::object();
    return nlohmann::json::parse(req.body);
}
