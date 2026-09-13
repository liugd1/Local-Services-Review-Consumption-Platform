#include "service/InteractionService.h"

#include "dao/InteractionDao.h"
#include "dao/MerchantDao.h"
#include "dao/SearchDao.h"
#include "model/Models.h"
#include "util/BizError.h"
#include "util/Time.h"

namespace {

// 校验收藏目标存在
void checkFavoriteTarget(const std::string& type, long long targetId) {
    if (type == "merchant") {
        if (SearchDao::detail(targetId).is_null())
            throw BizError(resp::NOT_FOUND, "商户不存在");
    } else if (type == "service") {
        if (MerchantDao::serviceById(targetId).is_null())
            throw BizError(resp::NOT_FOUND, "服务不存在");
    } else {
        throw BizError(resp::PARAM_ERROR, "收藏类型仅支持 merchant / service");
    }
}

void checkMerchantExists(long long merchantId) {
    if (SearchDao::detail(merchantId).is_null())
        throw BizError(resp::NOT_FOUND, "商户不存在");
}

}  // namespace

nlohmann::json InteractionService::addFavorite(long long userId, const std::string& type,
                                               long long targetId) {
    checkFavoriteTarget(type, targetId);
    InteractionDao::addFavorite(userId, type, targetId);
    return {{"favorited", true}};
}

void InteractionService::removeFavorite(long long userId, const std::string& type,
                                        long long targetId) {
    InteractionDao::removeFavorite(userId, type, targetId);
}

nlohmann::json InteractionService::listFavorites(long long userId, const std::string& type) {
    return InteractionDao::listFavorites(userId, type);
}

void InteractionService::addFollow(long long userId, long long merchantId) {
    checkMerchantExists(merchantId);
    InteractionDao::addFollow(userId, merchantId);
}

void InteractionService::removeFollow(long long userId, long long merchantId) {
    InteractionDao::removeFollow(userId, merchantId);
}

nlohmann::json InteractionService::listFollows(long long userId) {
    return InteractionDao::listFollows(userId);
}

nlohmann::json InteractionService::listHistory(long long userId) {
    return InteractionDao::listHistory(userId, 100);
}

void InteractionService::addConsumption(long long userId, const nlohmann::json& body) {
    long long merchantId = jsonInt(body, "merchant_id");
    if (merchantId <= 0) throw BizError(resp::PARAM_ERROR, "请选择消费的商户");
    checkMerchantExists(merchantId);
    if (jsonNum(body, "amount") <= 0) throw BizError(resp::PARAM_ERROR, "请输入正确的消费金额");

    long long serviceId = jsonInt(body, "service_id");
    long long packageId = jsonInt(body, "package_id");
    if (serviceId > 0) {
        auto svc = MerchantDao::serviceById(serviceId);
        if (svc.is_null() || svc.value("merchant_id", 0LL) != merchantId)
            throw BizError(resp::PARAM_ERROR, "服务不存在或不属于该商户");
    }
    if (packageId > 0) {
        auto pkg = MerchantDao::packageById(packageId);
        if (pkg.is_null() || pkg.value("merchant_id", 0LL) != merchantId)
            throw BizError(resp::PARAM_ERROR, "套餐不存在或不属于该商户");
    }
    if (!InteractionDao::addConsumption(userId, body))
        throw BizError(resp::SERVER_ERROR, "保存消费记录失败");
}

nlohmann::json InteractionService::listConsumptions(long long userId, int page, int size) {
    if (page < 1) page = 1;
    if (size < 1 || size > 100) size = 20;
    long long total = 0;
    auto rows = InteractionDao::listConsumptions(userId, page, size, total);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)}};
}
