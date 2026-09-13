#include "controller/CommonController.h"

#include "dao/CategoryDao.h"
#include "server/Api.h"

void CommonController::registerRoutes(httplib::Server& svr) {
    // GET /api/categories 商户类别列表（公开）
    svr.Get("/api/categories", [](const httplib::Request&, httplib::Response& res) {
        guard([&] { sendOk(res, CategoryDao::listAll()); }, res);
    });
}
