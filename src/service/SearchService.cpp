#include "service/SearchService.h"

#include "dao/InteractionDao.h"
#include "dao/MerchantDao.h"
#include "dao/ReviewDao.h"
#include "dao/SearchDao.h"
#include "model/Models.h"
#include "util/BizError.h"

namespace {
const int kRankLimit = 10;
}

nlohmann::json SearchService::search(const std::string& keyword, long long categoryId,
                                     const std::string& area, double priceMin, double priceMax,
                                     double minScore, const std::string& sort, int page,
                                     int size) {
    SearchParams p;
    p.keyword = keyword;
    p.categoryId = categoryId;
    p.area = area;
    p.priceMin = priceMin;
    p.priceMax = priceMax;
    p.minScore = minScore;
    p.sort = sort;
    p.page = page;
    p.size = size;
    if (p.page < 1) p.page = 1;
    if (p.size < 1 || p.size > 50) p.size = 10;

    long long total = 0;
    auto rows = SearchDao::search(p, total);
    return {{"total", total}, {"page", p.page}, {"size", p.size}, {"list", std::move(rows)}};
}

nlohmann::json SearchService::detail(long long merchantId, long long viewerId,
                                     const std::string& viewerRole) {
    auto m = SearchDao::detail(merchantId);
    if (m.is_null()) throw BizError(resp::NOT_FOUND, "商户不存在");

    auto st = jsonStr(m, "status");
    long long ownerId = m.value("user_id", 0LL);
    bool ownOrAdmin = viewerId > 0 && (viewerId == ownerId || viewerRole == "admin");
    if (st != "approved" && !ownOrAdmin)
        throw BizError(resp::NOT_FOUND, "商户不存在或尚未上架");

    // 访问计数
    MerchantDao::incViewCount(merchantId);
    // 登录用户记录浏览历史
    if (viewerId > 0) InteractionDao::addHistory(viewerId, merchantId);

    // 公开数据仅展示上架的服务与套餐
    m["stores"] = MerchantDao::listStores(merchantId);
    m["services"] = MerchantDao::listServices(merchantId, "on");
    m["packages"] = MerchantDao::listPackages(merchantId, "on");
    // 每个门店的在售数量 + 每个项目「在售门店」映射（详情页据此引导到具体门店下单）
    nlohmann::json serviceStores = nlohmann::json::object();
    nlohmann::json packageStores = nlohmann::json::object();
    for (auto& st : m["stores"]) {
        long long sid = st.value("id", 0LL);
        st["on_sale"] = MerchantDao::storeOnSaleCounts(sid);
        for (auto& sv : MerchantDao::listStoreServices(sid))
            serviceStores[std::to_string(sv.value("id", 0LL))].push_back(sid);
        for (auto& pk : MerchantDao::listStorePackages(sid))
            packageStores[std::to_string(pk.value("id", 0LL))].push_back(sid);
    }
    m["service_stores"] = serviceStores;
    m["package_stores"] = packageStores;
    m["favorite_count"] = InteractionDao::countFavorites("merchant", merchantId);
    m["follower_count"] = InteractionDao::countFollowers(merchantId);
    return m;
}

nlohmann::json SearchService::hotRank() { return SearchDao::hotRank(kRankLimit); }

nlohmann::json SearchService::newRank() { return SearchDao::newRank(kRankLimit); }

nlohmann::json SearchService::categoryRank(long long categoryId) {
    if (categoryId <= 0) throw BizError(resp::PARAM_ERROR, "类别参数错误");
    return SearchDao::categoryRank(categoryId, kRankLimit);
}

nlohmann::json SearchService::storeDetail(long long storeId, long long viewerId,
                                          const std::string& viewerRole) {
    auto s = MerchantDao::storeById(storeId);
    if (s.is_null()) throw BizError(resp::NOT_FOUND, "门店不存在");
    long long merchantId = s.value("merchant_id", 0LL);

    auto m = SearchDao::detail(merchantId);
    if (m.is_null()) throw BizError(resp::NOT_FOUND, "门店所属店铺不存在");
    bool ownOrAdmin =
        viewerId > 0 && (viewerId == m.value("user_id", 0LL) || viewerRole == "admin");
    if (jsonStr(m, "status") != "approved" && !ownOrAdmin)
        throw BizError(resp::NOT_FOUND, "门店暂未开放");

    MerchantDao::incViewCount(merchantId);
    if (viewerId > 0) InteractionDao::addHistory(viewerId, merchantId);

    nlohmann::json out = s;
    out["merchant"] = {{"id", merchantId},
                       {"name", jsonStr(m, "name")},
                       {"category_id", m.value("category_id", 0LL)},
                       {"area", jsonStr(m, "area")},
                       {"business_hours", jsonStr(m, "business_hours")},
                       {"phone", jsonStr(m, "phone")},
                       {"logo", jsonStr(m, "logo")},
                       {"intro", jsonStr(m, "intro")},
                       {"avg_score", m.value("avg_score", 0.0)},
                       {"review_count", m.value("review_count", 0LL)}};
    // 本门店在售项目（由门店决定是否运营）
    out["services"] = MerchantDao::listStoreServices(storeId);
    out["packages"] = MerchantDao::listStorePackages(storeId);
    out["coupons"] = MerchantDao::listStoreCoupons(storeId);
    // 门店维度口碑汇总
    out["summary"] = ReviewDao::targetSummary("store", storeId);
    out["favorite_count"] = InteractionDao::countFavorites("merchant", merchantId);
    out["follower_count"] = InteractionDao::countFollowers(merchantId);
    return out;
}
