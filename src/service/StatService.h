#pragma once
// 经营 / 运营统计业务
#include <nlohmann/json.hpp>

class StatService {
public:
    // 商户经营者查看自己的经营看板
    static nlohmann::json merchantStats(long long merchantUserId);
    // 平台运营看板
    static nlohmann::json platformStats();
};
