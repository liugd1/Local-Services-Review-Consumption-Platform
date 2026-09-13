#pragma once
// 商户类别数据访问
#include <nlohmann/json.hpp>

namespace CategoryDao {
nlohmann::json listAll();  // 全部类别，按 sort, id 排序
}
