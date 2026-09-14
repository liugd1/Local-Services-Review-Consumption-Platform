#include "service/OrderService.h"

#include <random>
#include <string>

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
                                        const std::string& itemType, long long itemId) {
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
        if (jsonStr(item, "status") != "on")
            throw BizError(resp::CONFLICT, "该服务项目已下架");
    } else {
        item = MerchantDao::packageById(itemId);
        if (item.is_null() || item.value("merchant_id", 0LL) != store.value("merchant_id", 0LL))
            throw BizError(resp::NOT_FOUND, "优惠套餐不存在或不属于该门店的店铺");
        if (jsonStr(item, "status") != "on")
            throw BizError(resp::CONFLICT, "该优惠套餐已下架");
    }

    long long limit = jsonInt(item, "limit_count");
    if (limit > 0 && OrderDao::countPurchased(userId, itemType, itemId) >= limit)
        throw BizError(resp::CONFLICT, "该项目每人限购 " + std::to_string(limit) + " 份");

    auto id = OrderDao::create(userId, storeId, itemType, itemId, item.value("price", 0.0),
                               genOrderNo());
    if (id == 0) throw BizError(resp::SERVER_ERROR, "下单失败");
    return OrderDao::orderById(id);
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
}

nlohmann::json OrderService::myOrders(long long userId, const std::string& status, int page,
                                      int size) {
    if (page < 1) page = 1;
    if (size < 1 || size > 100) size = 20;
    long long total = 0;
    auto rows = OrderDao::listByUser(userId, status, page, size, total);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)}};
}
