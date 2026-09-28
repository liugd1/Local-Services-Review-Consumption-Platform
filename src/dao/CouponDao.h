#pragma once
// 优惠活动（优惠券/满减/折扣/套餐券）与领取核销
#include <nlohmann/json.hpp>

#include <string>

namespace CouponDao {

// 创建活动（status=draft），返回 id
long long create(long long merchantId, const nlohmann::json& body);
nlohmann::json byId(long long id);             // 行对象（无则 nullptr）
nlohmann::json byCode(const std::string& code);  // 按核销码（join 商户名）
nlohmann::json listByMerchant(long long merchantId);
nlohmann::json listPublished(long long merchantId);  // 正在生效可领（时间窗内）
bool updateStatus(long long id, const std::string& status);

// ---- 优惠活动适用对象（为空 = 全场通用）----
// targets = [{target_type: 'service'|'package', target_id}]
void setTargets(long long couponId, const nlohmann::json& targets);
nlohmann::json listTargets(long long couponId);   // [{target_type,target_id,name}]
// 给券列表补充 targets / targets_text / scope_desc（一次查询，避免 N+1）
nlohmann::json withTargets(const nlohmann::json& coupons, long long merchantId);
// 判断券是否适用于某项目（无任何绑定 → 全场通用）
bool isApplicable(long long couponId, const std::string& itemType, long long itemId);
// 我的卡券记录（含券模板信息）
nlohmann::json claimById(long long claimId);
// 下单占用 / 退款归还
bool markClaimUsed(long long claimId);
bool releaseClaim(long long claimId);
// 软删除：用户从前端删除卡券（数据保留）
bool softDeleteClaim(long long claimId);
// 软删除：掌柜删除优惠活动（数据保留）
bool softDeleteCoupon(long long couponId);

// 领取：防超发 + 幂等（同一用户对同一活动限领一张）
int tryReceive(long long couponId, long long userId);
bool alreadyClaimed(long long couponId, long long userId);
// 核销（商户端扫码模拟）：校验归属与状态，事务更新
bool tryVerify(long long couponId, long long merchantId, long long claimId);
// 我的卡券列表
nlohmann::json listUserClaims(long long userId, const std::string& status);
nlohmann::json claimByUserAndCoupon(long long couponId, long long userId);

}  // namespace CouponDao
