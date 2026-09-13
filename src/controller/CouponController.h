#pragma once
// 优惠活动控制器
namespace httplib {
class Server;
}

class CouponController {
public:
    static void registerRoutes(httplib::Server& svr);
};
