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

// ---- 门店经营项目上架关系（itemKind: service / package / coupon）----
// 门店维度：列出该商户全部项目 + 本门店上架状态（未建关系视为未上架）
nlohmann::json storeOfferings(long long storeId, long long merchantId);
// 上/下架单个项目
bool setStoreOffering(long long storeId, const std::string& itemKind, long long itemId,
                      const std::string& status);
// 批量上/下架某类项目（该商户全部）
void bulkStoreOffering(long long storeId, long long merchantId, const std::string& itemKind,
                       const std::string& status);
// 门店在售数量 { services, packages, coupons }
nlohmann::json storeOnSaleCounts(long long storeId);
// 新增项目后：为该商户所有门店默认上架
void fanoutItemToStores(long long merchantId, const std::string& itemKind, long long itemId);
// 新增门店后：把该商户现有项目默认上架到新门店
void fanoutStoreItems(long long merchantId, long long storeId);
// 门店级可见列表（消费者端，仅 status='on' 且项目本身 on/上架）
nlohmann::json listStoreServices(long long storeId);
nlohmann::json listStorePackages(long long storeId);
nlohmann::json listStoreCoupons(long long storeId);
// 判断某项目在某门店是否在售
bool isOnSaleAtStore(long long storeId, const std::string& itemKind, long long itemId);

}  // namespace MerchantDao
