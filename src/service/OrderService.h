#pragma once
// 套餐订单业务：购买 / 使用（联动消费记录）/ 退款 / 我的订单
#include <nlohmann/json.hpp>

#include <string>

class OrderService {
public:
    static nlohmann::json buy(long long userId, long long packageId);
    static void useOrder(long long userId, long long orderId);
    static void refundOrder(long long userId, long long orderId);
    static nlohmann::json myOrders(long long userId, const std::string& status, int page,
                                   int size);
};
