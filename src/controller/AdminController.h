#pragma once
// 平台管理员控制器（用户管理；后续追加商户/评价审核等）
namespace httplib {
class Server;
}

class AdminController {
public:
    static void registerRoutes(httplib::Server& svr);
};
