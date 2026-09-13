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

}  // namespace

nlohmann::json OrderService::buy(long long userId, long long packageId) {
    auto p = MerchantDao::packageById(packageId);
    if (p.is_null()) throw BizError(resp::NOT_FOUND, "套餐不存在");
    if (jsonStr(p, "status") != "on") throw BizError(resp::CONFLICT, "该套餐已下架，暂不能购买");
    auto m = MerchantDao::byId(p.value("merchant_id", 0LL));
    if (m.is_null() || jsonStr(m, "status") != "approved")
        throw BizError(resp::FORBIDDEN, "套餐所属商户未上架");

    long long limit = jsonInt(p, "limit_count");
    if (limit > 0 && OrderDao::countPurchased(userId, packageId) >= limit)
        throw BizError(resp::CONFLICT, "该套餐每人限购 " + std::to_string(limit) + " 份");

    double amount = p.value("price", 0.0);
    auto id = OrderDao::create(userId, packageId, amount, genOrderNo());
    if (id == 0) throw BizError(resp::SERVER_ERROR, "下单失败");
    return OrderDao::orderById(id);
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
    // 联动：核销即生成一条到店消费记录
    OrderDao::writeConsumption(userId, o.value("merchant_id", 0LL),
                               o.value("package_id", 0LL), o.value("amount", 0.0));
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
