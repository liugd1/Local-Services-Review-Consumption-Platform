#pragma once
// 评价控制器（消费者发表/互动 + 商户回复 + 管理审核 + 公开列表）
namespace httplib {
class Server;
}

class ReviewController {
public:
    static void registerRoutes(httplib::Server& svr);
};
