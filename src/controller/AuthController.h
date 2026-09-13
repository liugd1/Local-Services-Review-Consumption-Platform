#pragma once
// 用户认证与个人中心控制器
namespace httplib {
class Server;
}

class AuthController {
public:
    static void registerRoutes(httplib::Server& svr);
};
