#pragma once
// 套餐订单控制器
namespace httplib {
class Server;
}

class OrderController {
public:
    static void registerRoutes(httplib::Server& svr);
};
