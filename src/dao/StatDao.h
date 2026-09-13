#pragma once
// 经营 / 运营统计
#include <nlohmann/json.hpp>

namespace StatDao {

// 商户维度经营看板
nlohmann::json merchant(long long merchantId);
// 平台运营看板
nlohmann::json platform();

}  // namespace StatDao
