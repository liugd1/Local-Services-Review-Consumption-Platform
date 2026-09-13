#include "service/SearchService.h"

#include "dao/InteractionDao.h"
#include "dao/MerchantDao.h"
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
