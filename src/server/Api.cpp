#include "server/Api.h"

#include "controller/AdminController.h"
#include "controller/AuthController.h"
#include "controller/CommonController.h"
#include "controller/CouponController.h"
#include "controller/InteractionController.h"
#include "controller/MerchantController.h"
#include "controller/OrderController.h"
#include "controller/ReviewController.h"
#include "controller/SearchController.h"
#include "controller/StatController.h"
#include "controller/UploadController.h"
#include "dao/UserDao.h"
#include "util/Time.h"

namespace api {

std::string bearerToken(const httplib::Request& req) {
    auto h = req.get_header_value("Authorization");
    if (h.rfind("Bearer ", 0) == 0) return h.substr(7);
    return "";
}

std::optional<AuthCtx> authenticate(const httplib::Request& req) {
    auto token = bearerToken(req);
    if (token.empty()) return std::nullopt;

    auto row = UserDao::findSessionUser(token);
    if (row.is_null()) return std::nullopt;
    if (jsonStr(row, "status") != "active") return std::nullopt;
    // 会话过期则清理并视为未登录
    if (jsonStr(row, "expires_at") <= timeutil::nowStr()) {
        UserDao::deleteSession(token);
        return std::nullopt;
    }

    AuthCtx ctx;
    ctx.userId = row.value("user_id", 0LL);
    ctx.username = jsonStr(row, "username");
    ctx.role = jsonStr(row, "role");
    ctx.nickname = jsonStr(row, "nickname");
    if (ctx.userId <= 0) return std::nullopt;
    return ctx;
}

void registerRoutes(httplib::Server& svr) {
    AuthController::registerRoutes(svr);         // 注册 / 登录 / 个人中心
    ReviewController::registerRoutes(svr);       // 评价 / 点赞 / 评论 / 举报审核
    AdminController::registerRoutes(svr);        // 平台管理员
    CommonController::registerRoutes(svr);       // 公共（类别等）
    CouponController::registerRoutes(svr);       // 优惠活动（商户/领取/核销）
    MerchantController::registerRoutes(svr);     // 商户中心（入驻/门店/服务/套餐）
    OrderController::registerRoutes(svr);        // 套餐订单（购买/核销/退款）
    UploadController::registerRoutes(svr);       // 图片上传
    SearchController::registerRoutes(svr);       // 公开检索 / 榜单 / 商户详情
    InteractionController::registerRoutes(svr);  // 收藏 / 关注 / 历史 / 消费
    StatController::registerRoutes(svr);         // 经营 / 运营统计
}

}  // namespace api
