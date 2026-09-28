#include "service/OrderService.h"

#include <random>
#include <string>

#include "dao/CouponDao.h"
#include "dao/MerchantDao.h"
#include "dao/OrderDao.h"
#include "model/Models.h"
#include "util/BizError.h"
#include "util/Time.h"

namespace {

std::string genOrderNo() {
    static std::mt19937_64 gen(std::random_device{}());
    char buf[32];
    std::time_t t = timeutil::now();
    std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&t));
    std::string s = buf;
    s += std::to_string(gen() % 9000 + 1000);
    return s;
}

// 可空外键安全取值（NULL / 缺失 → 0）
long long idOrZero(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null()) return 0;
    return it->get<long long>();
}

}  // namespace

nlohmann::json OrderService::buyAtStore(long long userId, long long storeId,
                                        const std::string& itemType, long long itemId,
                                        long long couponClaimId) {
    if (itemType != "service" && itemType != "package")
        throw BizError(resp::PARAM_ERROR, "item_type 仅支持 service / package");
    auto store = MerchantDao::storeById(storeId);
    if (store.is_null()) throw BizError(resp::NOT_FOUND, "门店不存在");
    if (jsonStr(store, "status") == "closed")
        throw BizError(resp::CONFLICT, "该门店已打烊，暂不能下单");
    auto m = MerchantDao::byId(store.value("merchant_id", 0LL));
    if (m.is_null() || jsonStr(m, "status") != "approved")
        throw BizError(resp::FORBIDDEN, "门店所属店铺未上架，暂不能下单");
    // 该门店是否运营此项目（由门店决定）
    if (!MerchantDao::isOnSaleAtStore(storeId, itemType, itemId))
        throw BizError(resp::CONFLICT, "该门店未上架该项目，请选择其他门店");

    nlohmann::json item;
    if (itemType == "service") {
        item = MerchantDao::serviceById(itemId);
        if (item.is_null() || item.value("merchant_id", 0LL) != store.value("merchant_id", 0LL))
            throw BizError(resp::NOT_FOUND, "服务项目不存在或不属于该门店的店铺");
        if (!jsonStr(item, "deleted_at").empty())
            throw BizError(resp::NOT_FOUND, "该服务项目已被删除");
        if (jsonStr(item, "status") != "on")
            throw BizError(resp::CONFLICT, "该服务项目已下架");
    } else {
        item = MerchantDao::packageById(itemId);
        if (item.is_null() || item.value("merchant_id", 0LL) != store.value("merchant_id", 0LL))
            throw BizError(resp::NOT_FOUND, "优惠套餐不存在或不属于该门店的店铺");
        if (!jsonStr(item, "deleted_at").empty())
            throw BizError(resp::NOT_FOUND, "该优惠套餐已被删除");
        if (jsonStr(item, "status") != "on")
            throw BizError(resp::CONFLICT, "该优惠套餐已下架");
    }

    long long limit = jsonInt(item, "limit_count");
    if (limit > 0 && OrderDao::countPurchased(userId, itemType, itemId) >= limit)
        throw BizError(resp::CONFLICT, "该项目每人限购 " + std::to_string(limit) + " 份");

    double origin = item.value("price", 0.0);
    double discount = 0;
    long long usedClaimId = 0;

    // 使用优惠券：校验归属 / 状态 / 有效期 / 门店参与 / 适用对象 / 门槛，并计算抵扣
    if (couponClaimId > 0) {
        auto claim = CouponDao::claimById(couponClaimId);
        if (claim.is_null() || claim.value("user_id", 0LL) != userId)
            throw BizError(resp::NOT_FOUND, "优惠券不存在或不属于你");
        if (jsonStr(claim, "claim_status") != "unused")
            throw BizError(resp::CONFLICT, "该优惠券已使用或已过期");
        if (claim.value("merchant_id", 0LL) != store.value("merchant_id", 0LL))
            throw BizError(resp::CONFLICT, "该优惠券不适用于本店铺");
        if (jsonStr(claim, "status") != "published")
            throw BizError(resp::CONFLICT, "该优惠活动已下线");
        std::string now = timeutil::nowStr();
        if (now < jsonStr(claim, "start_time") || now > jsonStr(claim, "end_time"))
            throw BizError(resp::CONFLICT, "该优惠券不在有效期内");
        if (!MerchantDao::isOnSaleAtStore(storeId, "coupon", claim.value("coupon_id", 0LL)))
            throw BizError(resp::CONFLICT, "本门店不参与该优惠活动");
        // 适用范围：券绑定对象时，只能在绑定的服务项目 / 优惠套餐中使用
        if (!CouponDao::isApplicable(claim.value("coupon_id", 0LL), itemType, itemId))
            throw BizError(resp::CONFLICT, "该优惠券仅可在指定的服务项目/优惠套餐中使用");
        if (origin < claim.value("threshold", 0.0))
            throw BizError(resp::CONFLICT, "未达到该券的使用门槛");
        if (jsonStr(claim, "type") == "discount") {
            double rate = claim.value("discount_rate", 0.0);
            if (rate > 0 && rate <= 1) discount = origin * (1 - rate);
        } else {
            discount = claim.value("face_value", 0.0);
        }
        if (discount <= 0) throw BizError(resp::CONFLICT, "该券无可抵扣金额");
        if (discount > origin) discount = origin;
        usedClaimId = couponClaimId;
    }

    double payable = origin - discount;
    auto id = OrderDao::create(userId, storeId, itemType, itemId, payable, genOrderNo(), discount,
                               usedClaimId);
    if (id == 0) throw BizError(resp::SERVER_ERROR, "下单失败");
    // 券在下单时即占用核销（退款会自动归还）
    if (usedClaimId > 0) CouponDao::markClaimUsed(usedClaimId);
    auto order = OrderDao::orderById(id);
    order["origin_amount"] = origin;
    return order;
}

nlohmann::json OrderService::buy(long long userId, long long packageId) {
    // 兼容旧接口：自动选取任一已上架该套餐的门店（不再支持在店铺层级直接下单）
    auto p = MerchantDao::packageById(packageId);
    if (p.is_null()) throw BizError(resp::NOT_FOUND, "套餐不存在");
    for (auto& s : MerchantDao::listStores(p.value("merchant_id", 0LL))) {
        long long storeId = s.value("id", 0LL);
        if (MerchantDao::isOnSaleAtStore(storeId, "package", packageId))
            return buyAtStore(userId, storeId, "package", packageId);
    }
    throw BizError(resp::CONFLICT, "该套餐暂无可下单的门店");
}

void OrderService::useOrder(long long userId, long long orderId) {
    auto o = OrderDao::orderById(orderId);
    if (o.is_null()) throw BizError(resp::NOT_FOUND, "订单不存在");
    if (o.value("user_id", 0LL) != userId)
        throw BizError(resp::FORBIDDEN, "只能核销自己的订单");
    if (jsonStr(o, "status") != "purchased")
        throw BizError(resp::CONFLICT, "当前状态不可核销（仅待使用订单可核销）");

    if (!OrderDao::markUsed(orderId, userId))
        throw BizError(resp::CONFLICT, "核销失败，可能已被处理");
    // 联动：核销即生成一条到店消费记录（含门店与项目，服务/套餐二选一）
    long long merchantId = idOrZero(o, "merchant_id");
    long long storeId = idOrZero(o, "store_id");
    if (jsonStr(o, "item_type") == "service") {
        OrderDao::writeConsumption(userId, merchantId, storeId, idOrZero(o, "service_id"), 0,
                                   o.value("amount", 0.0));
    } else {
        OrderDao::writeConsumption(userId, merchantId, storeId, 0, idOrZero(o, "package_id"),
                                   o.value("amount", 0.0));
    }
}

// 删除订单：待使用(purchased)的订单不允许删除（属于未使用且仍可正常使用的权益）
void OrderService::deleteOrder(long long userId, long long orderId) {
    auto o = OrderDao::orderById(orderId);
    if (o.is_null() || o.value("user_id", 0LL) != userId)
        throw BizError(resp::NOT_FOUND, "订单不存在或不属于你");
    if (jsonStr(o, "status") == "purchased")
        throw BizError(resp::CONFLICT, "该订单尚未使用（可正常核销或退款），不能删除");
    if (!OrderDao::softDeleteOrder(orderId))
        throw BizError(resp::CONFLICT, "删除失败，订单可能已被删除");
}

void OrderService::refundOrder(long long userId, long long orderId) {
    auto o = OrderDao::orderById(orderId);
    if (o.is_null()) throw BizError(resp::NOT_FOUND, "订单不存在");
    if (o.value("user_id", 0LL) != userId)
        throw BizError(resp::FORBIDDEN, "只能退自己的订单");
    if (jsonStr(o, "status") != "purchased")
        throw BizError(resp::CONFLICT, jsonStr(o, "status") == "used" ? "已使用，无法退款"
                                                                      : "当前状态不可退款");
    if (!OrderDao::markRefunded(orderId, userId))
        throw BizError(resp::CONFLICT, "退款失败，可能已被处理");
    // 使用过优惠券的订单退款：把券归还为「未使用」
    long long claimId = idOrZero(o, "coupon_claim_id");
    if (claimId > 0) CouponDao::releaseClaim(claimId);
}

nlohmann::json OrderService::myOrders(long long userId, const std::string& status, int page,
                                      int size) {
    if (page < 1) page = 1;
    if (size < 1 || size > 100) size = 20;
    long long total = 0;
    auto rows = OrderDao::listByUser(userId, status, page, size, total);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)}};
}
