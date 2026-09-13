#pragma once
// 图片上传控制器
namespace httplib {
class Server;
}

class UploadController {
public:
    static void registerRoutes(httplib::Server& svr);
};
