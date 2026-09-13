#include "service/CouponService.h"

#include <algorithm>
#include <cctype>

#include "dao/CouponDao.h"
#include "dao/MerchantDao.h"
#include "model/Models.h"
#include "util/BizError.h"
#include "util/Time.h"

namespace {

nlohmann::json myMerchantApproved(long long userId) {
    auto m = MerchantDao::byOwner(userId);
    if (m.is_null()) throw BizError(resp::CONFLICT, "尚未入驻商户");
    if (jsonStr(m, "status") != "approved")
        throw BizError(resp::FORBIDDEN, "商户尚未通过审核");
    return m;
}

// 宽松时间规整："2026-09-08T10:00" -> "2026-09-08 10:00:00"
std::string normDt(std::string s) {
    for (auto& ch : s)
        if (ch == 'T' || ch == 't') ch = ' ';
    size_t sp = s.find_first_not_of(' ');
    if (sp == std::string::npos) return "";
    s = s.substr(sp);
    if (s.size() == 16) s += ":00";         // YYYY-MM-DD HH:MM
    else if (s.size() == 10) s += " 00:00:00";  // YYYY-MM-DD
    return s;
}

}  // namespace

nlohmann::json CouponService::create(long long merchantUserId, const nlohmann::json& body) {
    auto m = myMerchantApproved(merchantUserId);
    std::string type = jsonStr(body, "type", "coupon");
    if (type != "coupon" && type != "full_reduction" && type != "discount" && type != "package")
        throw BizError(resp::PARAM_ERROR, "活动类型仅支持 coupon / full_reduction / discount / package");
    if (jsonStr(body, "name").empty()) throw BizError(resp::PARAM_ERROR, "请填写活动名称");
    double face = jsonNum(body, "face_value");
    double thr = jsonNum(body, "threshold");
    double rate = jsonNum(body, "discount_rate");
    long long total = jsonInt(body, "total");
    if (face < 0 || thr < 0 || rate < 0 || total < 0)
        throw BizError(resp::PARAM_ERROR, "数值不能为负");
    if (type == "discount" && (rate <= 0 || rate > 1))
        throw BizError(resp::PARAM_ERROR, "折扣率需在 0~1 之间（如 0.8 表示 8 折）");
    if (type == "coupon" && face <= 0) throw BizError(resp::PARAM_ERROR, "请填写券面金额");

    std::string start = normDt(jsonStr(body, "start_time"));
    if (start.empty()) start = timeutil::nowStr();
    std::string end = normDt(jsonStr(body, "end_time"));
    if (end.empty()) end = timeutil::afterDaysStr(30);
    if (start > end) throw BizError(resp::PARAM_ERROR, "结束时间需晚于开始时间");

    nlohmann::json c = {{"type", type}, {"name", jsonStr(body, "name")},
                        {"face_value", face}, {"threshold", thr}, {"discount_rate", rate},
                        {"total", total}, {"start_time", start}, {"end_time", end},
                        {"scope", jsonStr(body, "scope")}};
    auto id = CouponDao::create(m.value("id", 0LL), c);
    if (id == 0) throw BizError(resp::SERVER_ERROR, "创建优惠活动失败");
    return CouponDao::byId(id);
}

nlohmann::json CouponService::listByMerchant(long long merchantUserId) {
    auto m = myMerchantApproved(merchantUserId);
    return CouponDao::listByMerchant(m.value("id", 0LL));
}

void CouponService::setStatus(long long merchantUserId, long long couponId,
                              const std::string& status) {
    if (status != "draft" && status != "published" && status != "offline")
        throw BizError(resp::PARAM_ERROR, "状态仅支持 draft / published / offline");
    auto m = myMerchantApproved(merchantUserId);
    auto c = CouponDao::byId(couponId);
    if (c.is_null() || c.value("merchant_id", 0LL) != m.value("id", 0LL))
        throw BizError(resp::NOT_FOUND, "优惠活动不存在");
    CouponDao::updateStatus(couponId, status);
}

nlohmann::json CouponService::listPublished(long long merchantId) {
    if (MerchantDao::byId(merchantId).is_null())
        throw BizError(resp::NOT_FOUND, "商户不存在");
    return CouponDao::listPublished(merchantId);
}

nlohmann::json CouponService::receive(long long userId, long long couponId) {
    auto c = CouponDao::byId(couponId);
    if (c.is_null()) throw BizError(resp::NOT_FOUND, "优惠活动不存在");
    if (CouponDao::alreadyClaimed(couponId, userId))
        throw BizError(resp::CONFLICT, "您已领取过该券，每人限领一张");
    int rc = CouponDao::tryReceive(couponId, userId);
    if (rc == 0) throw BizError(resp::CONFLICT, "该券已领完或活动不在进行中");
    if (rc == 2) throw BizError(resp::CONFLICT, "您已领取过该券");
    return CouponDao::claimByUserAndCoupon(couponId, userId);
}

nlohmann::json CouponService::myClaims(long long userId, const std::string& status) {
    // 对已过期未使用的券在返回前统一标记为 expired，避免前端误用
    auto rows = CouponDao::listUserClaims(userId, status.empty() ? "all" : status);
    std::string now = timeutil::nowStr();
    for (auto& row : rows) {
        if (jsonStr(row, "claim_status") == "unused" &&
            (jsonStr(row, "coupon_status") != "published" ||
             now > jsonStr(row, "end_time"))) {
            row["claim_status"] = "expired";
        }
    }
    return rows;
}

nlohmann::json CouponService::verify(long long merchantUserId, const std::string& code) {
    auto m = myMerchantApproved(merchantUserId);
    std::string c = code;
    // 大小写不敏感 / 去空白
    c.erase(std::remove_if(c.begin(), c.end(), ::isspace), c.end());
    for (auto& ch : c) ch = static_cast<char>(::toupper(static_cast<unsigned char>(ch)));
    if (c.empty()) throw BizError(resp::PARAM_ERROR, "请输入核销码");

    auto claim = CouponDao::byCode(c);
    if (claim.is_null()) throw BizError(resp::NOT_FOUND, "核销码不存在");
    if (claim.value("merchant_id", 0LL) != m.value("id", 0LL))
        throw BizError(resp::NOT_FOUND, "该券不属于本店");
    if (jsonStr(claim, "claim_status") != "unused")
        throw BizError(resp::CONFLICT, "该券已被核销过，请勿重复核销");
    std::string now = timeutil::nowStr();
    if (jsonStr(claim, "status") != "published" || now < jsonStr(claim, "start_time") ||
        now > jsonStr(claim, "end_time"))
        throw BizError(resp::CONFLICT, "该券不在有效期内或已下线");

    if (!CouponDao::tryVerify(claim.value("coupon_id", 0LL), m.value("id", 0LL),
                              claim.value("claim_id", 0LL)))
        throw BizError(resp::CONFLICT, "核销失败，请刷新后重试");

    return {{"code", c},
            {"coupon", jsonStr(claim, "name")},
            {"claim_user", jsonStr(claim, "claim_user")},
            {"used_time", timeutil::nowStr()}};
}
