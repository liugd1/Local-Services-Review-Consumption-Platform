#pragma once
// 公开商户检索 / 榜单 / 详情
#include <nlohmann/json.hpp>
#include <string>

class SearchService {
public:
    // 组合条件分页检索
    static nlohmann::json search(const std::string& keyword, long long categoryId,
                                 const std::string& area, double priceMin, double priceMax,
                                 double minScore, const std::string& sort, int page, int size);

    // 商户详情：仅 approved 公开可见；所有者本人/管理员可见任意状态。
    // 访问即增加浏览量；登录用户（消费者）自动记录浏览历史。
    static nlohmann::json detail(long long merchantId, long long viewerId,
                                 const std::string& viewerRole);

    static nlohmann::json hotRank();
    static nlohmann::json newRank();
    static nlohmann::json categoryRank(long long categoryId);
};
