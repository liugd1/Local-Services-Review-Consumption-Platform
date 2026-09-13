#pragma once
// 通用控制器：公开类别列表等
namespace httplib {
class Server;
}

class CommonController {
public:
    static void registerRoutes(httplib::Server& svr);
};
