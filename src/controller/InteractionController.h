#pragma once
// 消费者互动控制器：收藏 / 关注 / 浏览历史 / 消费记录
namespace httplib {
class Server;
}

class InteractionController {
public:
    static void registerRoutes(httplib::Server& svr);
};
