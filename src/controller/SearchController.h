#pragma once
// 公开浏览控制器：检索 / 榜单 / 商户详情
namespace httplib {
class Server;
}

class SearchController {
public:
    static void registerRoutes(httplib::Server& svr);
};
