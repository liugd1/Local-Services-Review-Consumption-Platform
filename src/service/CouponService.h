#pragma once
// 优惠活动业务
#include <nlohmann/json.hpp>

#include <string>

class CouponService {
public:
    // 商户创建优惠活动（经营者本人，商户已过审）
    static nlohmann::json create(long long merchantUserId, const nlohmann::json& body);
    static nlohmann::json listByMerchant(long long merchantUserId);
    static void setStatus(long long merchantUserId, long long couponId, const std::string& status);
    // 公开：某商户当前生效可领的券
    static nlohmann::json listPublished(long long merchantId);

    // 消费者领取（一券一人一张；防超发；重复领取 409）
    static nlohmann::json receive(long long userId, long long couponId);
    // 我的卡券
    static nlohmann::json myClaims(long long userId, const std::string& status);

    // 商户核销（输入券码，校验归属/状态/有效期后核销）
    static nlohmann::json verify(long long merchantUserId, const std::string& code);
};
