#include "service/StatService.h"

#include "dao/MerchantDao.h"
#include "dao/StatDao.h"
#include "model/Models.h"
#include "util/BizError.h"

nlohmann::json StatService::merchantStats(long long merchantUserId) {
    auto m = MerchantDao::byOwner(merchantUserId);
    if (m.is_null()) throw BizError(resp::CONFLICT, "尚未入驻商户");
    return StatDao::merchant(m.value("id", 0LL));
}

nlohmann::json StatService::platformStats() { return StatDao::platform(); }
