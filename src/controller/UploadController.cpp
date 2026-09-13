#include "controller/UploadController.h"

#include <cctype>
#include <fstream>

#include "server/Api.h"
#include "util/Crypto.h"
#include "util/Paths.h"
#include "util/Time.h"

namespace {

// 取文件扩展名（小写），不在白名单返回空
std::string checkExt(const std::string& filename) {
    auto pos = filename.find_last_of('.');
    if (pos == std::string::npos) return "";
    std::string ext = filename.substr(pos + 1);
    for (auto& c : ext) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "webp") return ext;
    return "";
}

}  // namespace

void UploadController::registerRoutes(httplib::Server& svr) {
    // POST /api/upload   multipart/form-data, 字段名 file
    svr.Post("/api/upload", [](const httplib::Request& req, httplib::Response& res) {
        guard([&] {
            auto ctx = api::authenticate(req);
            if (!ctx) throw BizError(resp::UNAUTHORIZED, "未登录");

            if (!req.form.has_file("file")) throw BizError(resp::PARAM_ERROR, "请选择要上传的图片");
            auto file = req.form.get_file("file");
            std::string rawName = !file.filename.empty() ? file.filename : file.name;
            if (rawName.empty()) throw BizError(resp::PARAM_ERROR, "上传文件名无效");
            if (file.content.size() > 8 * 1024 * 1024)
                throw BizError(resp::PARAM_ERROR, "图片大小不能超过 8MB");

            auto ext = checkExt(rawName);
            if (ext.empty())
                throw BizError(resp::PARAM_ERROR,
                               "仅支持 jpg / jpeg / png / gif / webp 格式的图片");

            // 生成唯一文件名并保存到 public/uploads
            std::string name = "u_" + std::to_string(timeutil::nowUnix()) + "_" +
                               crypto::randomHex(4) + "." + ext;
            auto path = paths::publicDir() / "uploads" / name;
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out) throw BizError(resp::SERVER_ERROR, "图片保存失败");
            out.write(file.content.data(),
                      static_cast<std::streamsize>(file.content.size()));
            out.close();

            nlohmann::json data = nlohmann::json::object();
            data["url"] = "/uploads/" + name;
            data["size"] = file.content.size();
            sendOk(res, data);
        }, res);
    });
}
