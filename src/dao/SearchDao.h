#pragma once
// 商户检索 / 榜单 / 详情 数据访问
#include <nlohmann/json.hpp>
#include <string>

struct SearchParams {
    std::string keyword;
    long long categoryId = 0;
    std::string area;
    double priceMin = 0;  // 0 表示不限
    double priceMax = 0;
    double minScore = 0;
    std::string sort;  // "" / score / popularity / newest
    int page = 1;
    int size = 10;
};

namespace SearchDao {

// 组合条件分页检索（仅 approved 商户），返回行数组；同时回写 total
nlohmann::json search(const SearchParams& p, long long& total);

// 商户详情（approved 方可公开查看）；含聚合评分与评价数
nlohmann::json detail(long long merchantId);

// 热门榜单（view_count + favorite 综合，简单按人气排序）
nlohmann::json hotRank(int limit);
// 新店榜
nlohmann::json newRank(int limit);
// 分类榜单（该分类内按评分/人气）
nlohmann::json categoryRank(long long categoryId, int limit);

}  // namespace SearchDao
