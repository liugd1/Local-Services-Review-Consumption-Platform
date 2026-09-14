#pragma once
// 门店订单（下单主体为「门店」，商品支持服务项目 / 优惠套餐）数据访问
#include <nlohmann/json.hpp>

#include <string>

namespace OrderDao {

// 创建订单（归属与上架校验在 Service），itemType: package / service
long long create(long long userId, long long storeId, const std::string& itemType,
                 long long itemId, double amount, const std::string& orderNo);
// 用户对某商品的有效购买数量（限购判断，已退款不计）
long long countPurchased(long long userId, const std::string& itemType, long long itemId);
// 订单详情（含门店名、项目名、商户 id）
nlohmann::json orderById(long long id);
nlohmann::json listByUser(long long userId, const std::string& status, int page, int size,
                          long long& total);
// 使用：purchased -> used；成功返回 1
bool markUsed(long long orderId, long long userId);
// 退款：purchased -> refunded
bool markRefunded(long long orderId, long long userId);
// 生成联动消费记录（服务或套餐二选一，另一项写 NULL）
bool writeConsumption(long long userId, long long merchantId, long long storeId,
                      long long serviceId, long long packageId, double amount);

}  // namespace OrderDao
