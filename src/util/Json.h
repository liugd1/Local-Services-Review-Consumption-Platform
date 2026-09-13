#pragma once
// 统一 API 响应封装：{ code, message, data }
#include <nlohmann/json.hpp>

namespace resp {

inline nlohmann::json ok(nlohmann::json data = nullptr) {
    return {{"code", 0}, {"message", "OK"}, {"data", std::move(data)}};
}

inline nlohmann::json err(int code, const std::string& message) {
    return {{"code", code}, {"message", message}, {"data", nullptr}};
}

// 常用业务错误码
enum BizCode {
    OK_CODE = 0,
    PARAM_ERROR = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    CONFLICT = 409,
    SERVER_ERROR = 500,
};

}  // namespace resp
