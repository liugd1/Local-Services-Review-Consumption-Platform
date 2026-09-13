#include "service/MerchantService.h"

#include "dao/MerchantDao.h"
#include "dao/UserDao.h"
#include "model/Models.h"
#include "util/BizError.h"

namespace {

nlohmann::json myMerchantOrThrow(long long userId) {
    auto m = MerchantDao::byOwner(userId);
    if (m.is_null()) throw BizError(resp::CONFLICT, "尚未入驻商户，请先提交入驻申请");
    return m;
}

long long merchantIdOf(const nlohmann::json& m) { return m.value("id", 0LL); }

void requireApproved(const nlohmann::json& m) {
    if (jsonStr(m, "status") != "approved")
        throw BizError(resp::FORBIDDEN, "商户尚未通过平台审核，不能执行该操作");
}

// 把 body 出现的字段覆盖到 base（部分更新）
void mergePresent(nlohmann::json& base, const nlohmann::json& body) {
    for (auto it = body.begin(); it != body.end(); ++it) {
        base[it.key()] = it.value();
    }
}

// 校验门店归属当前商户，返回门店行；不存在则抛 404
nlohmann::json assertStoreOwned(long long merchantId, long long storeId) {
    for (const auto& s : MerchantDao::listStores(merchantId)) {
        if (s.value("id", 0LL) == storeId) return s;
    }
    throw BizError(resp::NOT_FOUND, "门店不存在或不属于当前商户");
}

// 门店营业状态合法性
void checkStoreStatus(const std::string& st) {
    if (st != "open" && st != "rest" && st != "closed")
        throw BizError(resp::PARAM_ERROR, "门店状态仅支持 open / rest / closed");
}

nlohmann::json assertServiceOwned(long long merchantId, long long serviceId) {
    auto s = MerchantDao::serviceById(serviceId);
    if (s.is_null() || s.value("merchant_id", 0LL) != merchantId)
        throw BizError(resp::NOT_FOUND, "服务不存在或不属于当前商户");
    return s;
}

nlohmann::json assertPackageOwned(long long merchantId, long long packageId) {
    auto p = MerchantDao::packageById(packageId);
    if (p.is_null() || p.value("merchant_id", 0LL) != merchantId)
        throw BizError(resp::NOT_FOUND, "套餐不存在或不属于当前商户");
    return p;
}

}  // namespace

// ---------------- 入驻与资料 ----------------
nlohmann::json MerchantService::apply(long long userId, const nlohmann::json& body) {
    if (jsonStr(body, "name").empty()) throw BizError(resp::PARAM_ERROR, "商户名称不能为空");
    if (jsonStr(body, "category_id").empty()) throw BizError(resp::PARAM_ERROR, "请选择商户类别");
    if (jsonStr(body, "phone").empty()) throw BizError(resp::PARAM_ERROR, "请填写联系电话");
    double pmin = jsonNum(body, "price_min");
    double pmax = jsonNum(body, "price_max");
    if (pmin < 0 || pmax < 0) throw BizError(resp::PARAM_ERROR, "价格不能为负数");
    if (pmax > 0 && pmin > pmax) throw BizError(resp::PARAM_ERROR, "价格区间设置不正确");

    auto cur = MerchantDao::byOwner(userId);
    if (!cur.is_null()) {
        auto st = jsonStr(cur, "status");
        if (st == "pending") throw BizError(resp::CONFLICT, "入驻申请审核中，请耐心等待");
        if (st == "approved") throw BizError(resp::CONFLICT, "您已入驻平台");
        // 被驳回 → 合并新资料后重新提交
        nlohmann::json full = cur;
        mergePresent(full, body);
        full["images"] = jsonArrayToCsv(body, "images");
        MerchantDao::updateProfile(merchantIdOf(cur), full);
        MerchantDao::setStatus(merchantIdOf(cur), "pending", "");
        UserDao::updateRole(userId, "merchant");
        return MerchantDao::byId(merchantIdOf(cur));
    }

    nlohmann::json m = {
        {"name", jsonStr(body, "name")},
        {"category_id", jsonStr(body, "category_id")},
        {"area", jsonStr(body, "area")},
        {"business_hours", jsonStr(body, "business_hours")},
        {"phone", jsonStr(body, "phone")},
        {"intro", jsonStr(body, "intro")},
        {"price_min", pmin},
        {"price_max", pmax},
        {"logo", jsonStr(body, "logo")},
        {"images", jsonArrayToCsv(body, "images")},
    };
    auto id = MerchantDao::create(userId, m);
    if (id == 0) throw BizError(resp::SERVER_ERROR, "创建商户失败");
    UserDao::updateRole(userId, "merchant");  // 升级为商户经营者角色
    return MerchantDao::byId(id);
}

nlohmann::json MerchantService::getMyMerchant(long long userId) {
    auto m = myMerchantOrThrow(userId);
    auto mid = merchantIdOf(m);
    return {{"merchant", m},
            {"stores", MerchantDao::listStores(mid)},
            {"services", MerchantDao::listServices(mid)},
            {"packages", MerchantDao::listPackages(mid)}};
}

void MerchantService::updateMyMerchant(long long userId, const nlohmann::json& body) {
    auto cur = myMerchantOrThrow(userId);
    auto name = jsonStr(body, "name");
    if (!name.empty()) cur["name"] = name;
    if (body.contains("category_id")) cur["category_id"] = jsonStr(body, "category_id");
    for (const char* k : {"area", "business_hours", "phone", "intro", "logo"}) {
        if (body.contains(k)) cur[k] = jsonStr(body, k);
    }
    if (body.contains("price_min")) cur["price_min"] = jsonNum(body, "price_min");
    if (body.contains("price_max")) cur["price_max"] = jsonNum(body, "price_max");
    if (body.contains("images")) cur["images"] = jsonArrayToCsv(body, "images");
    MerchantDao::updateProfile(merchantIdOf(cur), cur);
}

// ---------------- 管理端 ----------------
nlohmann::json MerchantService::adminList(const std::string& status, const std::string& keyword,
                                          int page, int size) {
    long long total = 0;
    auto rows = MerchantDao::adminList(status, keyword, page, size, total);
    return {{"total", total}, {"page", page}, {"size", size}, {"list", std::move(rows)}};
}

nlohmann::json MerchantService::adminCounts() {
    nlohmann::json counts = {{"pending", 0}, {"approved", 0}, {"rejected", 0}, {"total", 0}};
    long long total = 0;
    for (const auto& r : MerchantDao::countByStatus()) {
        counts[jsonStr(r, "status")] = r.value("cnt", 0LL);
        total += r.value("cnt", 0LL);
    }
    counts["total"] = total;
    return counts;
}

void MerchantService::audit(long long merchantId, const nlohmann::json& body) {
    auto m = MerchantDao::byId(merchantId);
    if (m.is_null()) throw BizError(resp::NOT_FOUND, "商户不存在");
    auto st = jsonStr(body, "status");
    if (st != "approved" && st != "rejected")
        throw BizError(resp::PARAM_ERROR, "审核结果仅支持 approved / rejected");
    if (st == "rejected" && jsonStr(body, "reason").empty())
        throw BizError(resp::PARAM_ERROR, "驳回时必须填写原因");
    MerchantDao::setStatus(merchantId, st, st == "rejected" ? jsonStr(body, "reason") : "");
}

// ---------------- 门店 ----------------
nlohmann::json MerchantService::listStores(long long userId) {
    return MerchantDao::listStores(merchantIdOf(myMerchantOrThrow(userId)));
}

nlohmann::json MerchantService::addStore(long long userId, const nlohmann::json& body) {
    auto m = myMerchantOrThrow(userId);
    requireApproved(m);
    if (jsonStr(body, "name").empty()) throw BizError(resp::PARAM_ERROR, "门店名称不能为空");
    std::string st = body.contains("status") ? jsonStr(body, "status") : "open";
    checkStoreStatus(st);
    nlohmann::json data = body;
    data["status"] = st;
    auto id = MerchantDao::addStore(merchantIdOf(m), data);
    if (id == 0) throw BizError(resp::SERVER_ERROR, "新增门店失败");
    return {{"id", id}};
}

void MerchantService::updateStore(long long userId, long long storeId,
                                  const nlohmann::json& body) {
    auto m = myMerchantOrThrow(userId);
    requireApproved(m);
    auto cur = assertStoreOwned(merchantIdOf(m), storeId);
    std::string st = body.contains("status") ? jsonStr(body, "status") : jsonStr(cur, "status");
    checkStoreStatus(st);
    mergePresent(cur, body);
    cur["status"] = st;
    MerchantDao::updateStore(storeId, cur);
}

void MerchantService::deleteStore(long long userId, long long storeId) {
    auto m = myMerchantOrThrow(userId);
    requireApproved(m);
    assertStoreOwned(merchantIdOf(m), storeId);
    MerchantDao::removeStore(storeId);
}

// ---------------- 服务项目 ----------------
nlohmann::json MerchantService::listServices(long long userId, const std::string& status) {
    return MerchantDao::listServices(merchantIdOf(myMerchantOrThrow(userId)), status);
}

nlohmann::json MerchantService::addService(long long userId, const nlohmann::json& body) {
    auto m = myMerchantOrThrow(userId);
    requireApproved(m);
    if (jsonStr(body, "name").empty()) throw BizError(resp::PARAM_ERROR, "服务名称不能为空");
    if (jsonNum(body, "price") < 0) throw BizError(resp::PARAM_ERROR, "价格不能为负数");
    auto id = MerchantDao::addService(merchantIdOf(m), body);
    if (id == 0) throw BizError(resp::SERVER_ERROR, "新增服务失败");
    return {{"id", id}};
}

void MerchantService::updateService(long long userId, long long serviceId,
                                    const nlohmann::json& body) {
    auto m = myMerchantOrThrow(userId);
    requireApproved(m);
    auto cur = assertServiceOwned(merchantIdOf(m), serviceId);
    mergePresent(cur, body);
    MerchantDao::updateService(serviceId, cur);
}

void MerchantService::setServiceStatus(long long userId, long long serviceId,
                                       const std::string& status) {
    if (status != "on" && status != "off")
        throw BizError(resp::PARAM_ERROR, "服务状态仅支持 on / off");
    auto m = myMerchantOrThrow(userId);
    requireApproved(m);
    assertServiceOwned(merchantIdOf(m), serviceId);
    MerchantDao::updateServiceStatus(serviceId, status);
}

// ---------------- 消费套餐 ----------------
nlohmann::json MerchantService::listPackages(long long userId, const std::string& status) {
    return MerchantDao::listPackages(merchantIdOf(myMerchantOrThrow(userId)), status);
}

nlohmann::json MerchantService::addPackage(long long userId, const nlohmann::json& body) {
    auto m = myMerchantOrThrow(userId);
    requireApproved(m);
    if (jsonStr(body, "name").empty()) throw BizError(resp::PARAM_ERROR, "套餐名称不能为空");
    if (jsonNum(body, "price") < 0) throw BizError(resp::PARAM_ERROR, "价格不能为负数");
    auto id = MerchantDao::addPackage(merchantIdOf(m), body);
    if (id == 0) throw BizError(resp::SERVER_ERROR, "新增套餐失败");
    return {{"id", id}};
}

void MerchantService::updatePackage(long long userId, long long packageId,
                                    const nlohmann::json& body) {
    auto m = myMerchantOrThrow(userId);
    requireApproved(m);
    auto cur = assertPackageOwned(merchantIdOf(m), packageId);
    mergePresent(cur, body);
    MerchantDao::updatePackage(packageId, cur);
}

void MerchantService::setPackageStatus(long long userId, long long packageId,
                                       const std::string& status) {
    if (status != "on" && status != "off")
        throw BizError(resp::PARAM_ERROR, "套餐状态仅支持 on / off");
    auto m = myMerchantOrThrow(userId);
    requireApproved(m);
    assertPackageOwned(merchantIdOf(m), packageId);
    MerchantDao::updatePackageStatus(packageId, status);
}

