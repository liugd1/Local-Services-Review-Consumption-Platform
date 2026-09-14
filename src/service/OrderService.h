#pragma once
// 门店订单业务：在「门店」下购买服务项目/优惠套餐 → 核销（联动消费记录）→ 退款
#include <nlohmann/json.hpp>

#include <string>

class OrderService {
public:
    // 消费者在指定门店下单（item_type: service / package）
    static nlohmann::json buyAtStore(long long userId, long long storeId,
                                     const std::string& itemType, long long itemId);
    // 兼容旧接口：按套餐下单（自动选取在售该套餐的门店）
    static nlohmann::json buy(long long userId, long long packageId);

    static void useOrder(long long userId, long long orderId);
    static void refundOrder(long long userId, long long orderId);
    static nlohmann::json myOrders(long long userId, const std::string& status, int page,
                                   int size);
};
