#pragma once
// 统计看板控制器
namespace httplib {
class Server;
}

class StatController {
public:
    static void registerRoutes(httplib::Server& svr);
};
