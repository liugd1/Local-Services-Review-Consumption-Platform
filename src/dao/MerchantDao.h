#pragma once
// 商户/门店/服务/套餐 数据访问
#include <nlohmann/json.hpp>
#include <string>

namespace MerchantDao {

// ---- 商户 ----
// 创建商户（内部自动填充 user_id / status=pending / created_at）；返回新 id
long long create(long long userId, const nlohmann::json& m);
nlohmann::json byId(long long id);      // 返回行对象；不存在返回 nullptr
nlohmann::json byOwner(long long userId);  // 用户最新一条商户
nlohmann::json storeById(long long id);  // 门店行对象（无则 nullptr）
// 更新资料（name/category_id/area/business_hours/phone/intro/price_min/price_max/logo/images），
// 由 service 层合并字段后全量传入
void updateProfile(long long id, const nlohmann::json& full);
bool setStatus(long long id, const std::string& status, const std::string& reason = "");
void incViewCount(long long id);
// 管理端分页（可按 status / keyword 过滤，keyword 匹配名称/电话/介绍）
nlohmann::json adminList(const std::string& status, const std::string& keyword, int page,
                         int size, long long& total);
nlohmann::json countByStatus();  // 各审核状态数量（管理端角标）

// ---- 门店 ----
nlohmann::json listStores(long long merchantId);
long long addStore(long long merchantId, const nlohmann::json& s);
void updateStore(long long id, const nlohmann::json& s);
// 删除门店（先解除其下服务的门店引用）
bool removeStore(long long id);

// ---- 服务项目 ----
nlohmann::json listServices(long long merchantId, const std::string& status = "");
nlohmann::json serviceById(long long id);  // 行对象（含 merchant_id 供归属校验），无则 nullptr
long long addService(long long merchantId, const nlohmann::json& s);
void updateService(long long id, const nlohmann::json& s);
bool updateServiceStatus(long long id, const std::string& status);

// ---- 消费套餐 ----
nlohmann::json listPackages(long long merchantId, const std::string& status = "");
nlohmann::json packageById(long long id);
long long addPackage(long long merchantId, const nlohmann::json& p);
void updatePackage(long long id, const nlohmann::json& p);
bool updatePackageStatus(long long id, const std::string& status);

}  // namespace MerchantDao
