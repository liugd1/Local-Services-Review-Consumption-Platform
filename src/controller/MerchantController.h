#pragma once
// 商户中心控制器（入驻申请、资料、门店、服务、套餐）
namespace httplib {
class Server;
}

class MerchantController {
public:
    static void registerRoutes(httplib::Server& svr);
};
