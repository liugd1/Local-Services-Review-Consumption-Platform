#pragma once
// 套餐订单（购买/使用/退款）数据访问
#include <nlohmann/json.hpp>

#include <string>

namespace OrderDao {

// 创建订单（校验在 Service），返回订单 id
long long create(long long userId, long long packageId, double amount, const std::string& orderNo);
// 用户已购买未退款数量（限购判断）
long long countPurchased(long long userId, long long packageId);
nlohmann::json orderById(long long id);  // join 套餐/商户名
nlohmann::json listByUser(long long userId, const std::string& status, int page, int size,
                          long long& total);
// 使用：purchased -> used；成功返回 1
bool markUsed(long long orderId, long long userId);
// 退款：purchased -> refunded
bool markRefunded(long long orderId, long long userId);
// 生成联动消费记录
bool writeConsumption(long long userId, long long merchantId, long long packageId, double amount);

}  // namespace OrderDao
