#pragma once
// 收藏 / 关注 / 浏览历史 / 消费记录
#include <nlohmann/json.hpp>
#include <string>

class InteractionService {
public:
    // ---- 收藏（target_type: merchant / service）----
    static nlohmann::json addFavorite(long long userId, const std::string& type,
                                      long long targetId);
    static void removeFavorite(long long userId, const std::string& type, long long targetId);
    static nlohmann::json listFavorites(long long userId, const std::string& type);

    // ---- 关注商户 ----
    static void addFollow(long long userId, long long merchantId);
    static void removeFollow(long long userId, long long merchantId);
    static nlohmann::json listFollows(long long userId);

    // ---- 浏览历史（查看商户详情时自动记录）----
    static nlohmann::json listHistory(long long userId);

    // ---- 消费记录 ----
    static void addConsumption(long long userId, const nlohmann::json& body);
    static nlohmann::json listConsumptions(long long userId, int page, int size);
};
