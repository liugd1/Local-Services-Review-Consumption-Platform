// LocalLife Platform —— 程序入口
#include <httplib.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "db/Database.h"
#include "db/Migrate.h"
#include "server/Api.h"
#include "service/UserService.h"
#include "util/Json.h"
#include "util/Paths.h"

namespace fs = std::filesystem;

// 读取整个文件内容（UTF-8）
static std::string readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc, char* argv[]) {
    // 以 exe 位置为锚定位项目根（public/data/uploads 统一解析）
    paths::init(argc > 0 ? argv[0] : "");
    std::error_code ec;
    fs::create_directories(paths::dataDir(), ec);
    fs::create_directories(paths::publicDir() / "uploads", ec);

    // 1) 打开数据库
    auto& db = Database::instance();
    if (!db.open((paths::dataDir() / "locallife.db").string())) {
        std::cerr << "[fatal] 打开数据库失败: " << db.lastError() << std::endl;
        return 1;
    }

    // 2) 初始化表结构（幂等）
    auto schemaPath = paths::root() / "sql" / "schema.sql";
    if (!fs::exists(schemaPath, ec)) {
        std::cerr << "[fatal] 未找到 sql/schema.sql: " << schemaPath << std::endl;
        return 1;
    }
    if (!db.execScript(readFile(schemaPath))) {
        std::cerr << "[fatal] 初始化数据库失败: " << db.lastError() << std::endl;
        return 1;
    }
    // 2.1) 老库增量迁移（评价对象化 / 评论楼中楼）
    migrate::run();

    // 3) 确保平台管理员存在
    UserService::ensureAdmin();

    // 4) 启动 HTTP 服务
    httplib::Server svr;
    svr.set_mount_point("/", paths::publicDir().string());
    api::registerRoutes(svr);

    // 404 → JSON（仅当响应还未写入内容时才补默认文案，避免覆盖业务 404）
    svr.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (res.status == 404 && res.body.empty()) {
            sendJson(res, 404, resp::err(resp::NOT_FOUND, "接口不存在"));
        }
    });
    // 未捕获异常 → 500 JSON
    svr.set_exception_handler([](const httplib::Request&, httplib::Response& res,
                                 std::exception_ptr ep) {
        std::string msg = "服务器内部错误";
        try {
            if (ep) std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            msg = e.what();
        }
        sendJson(res, 500, resp::err(resp::SERVER_ERROR, msg));
    });

    std::cout << "=============================================" << std::endl;
    std::cout << "  LocalLife Platform 本地生活服务平台" << std::endl;
    std::cout << "  服务地址: http://127.0.0.1:8080" << std::endl;
    std::cout << "  管理账号: admin / admin123" << std::endl;
    std::cout << "=============================================" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
