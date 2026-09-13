#pragma once
// 用户互动（收藏/关注/浏览历史/消费记录）数据访问
#include <nlohmann/json.hpp>
#include <string>

namespace InteractionDao {

// ---- 收藏 ----
bool isFavorite(long long userId, const std::string& type, long long targetId);
bool addFavorite(long long userId, const std::string& type, long long targetId);
bool removeFavorite(long long userId, const std::string& type, long long targetId);
nlohmann::json listFavorites(long long userId, const std::string& type);
long long countFavorites(const std::string& type, long long targetId);

// ---- 关注 ----
bool isFollowing(long long userId, long long merchantId);
bool addFollow(long long userId, long long merchantId);
bool removeFollow(long long userId, long long merchantId);
nlohmann::json listFollows(long long userId);
long long countFollowers(long long merchantId);

// ---- 浏览历史 ----
bool addHistory(long long userId, long long merchantId);
nlohmann::json listHistory(long long userId, int limit);

// ---- 消费记录 ----
bool addConsumption(long long userId, const nlohmann::json& rec);
nlohmann::json listConsumptions(long long userId, int page, int size, long long& total);

}  // namespace InteractionDao
