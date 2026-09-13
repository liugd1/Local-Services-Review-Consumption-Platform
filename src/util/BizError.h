#pragma once
// 业务异常：带业务错误码，controller 层捕获后转为统一 JSON 响应
#include "util/Json.h"

#include <stdexcept>
#include <string>

struct BizError : std::runtime_error {
    explicit BizError(int code, const std::string& msg) : std::runtime_error(msg), code(code) {}
    int code;
};
