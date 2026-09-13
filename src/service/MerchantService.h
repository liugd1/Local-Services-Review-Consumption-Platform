#pragma once
// 商户入驻与门店/服务/套餐经营逻辑（含归属与审核状态校验）
#include <nlohmann/json.hpp>
#include <string>

class MerchantService {
public:
    // 入驻申请：无商户→新建 pending；被驳回→更新资料重新提交 pending；
    // 已有待审/已通过商户则 409。申请成功后用户角色升级为 merchant。
    static nlohmann::json apply(long long userId, const nlohmann::json& body);

    // 我的商户（商家端，含门店/服务/套餐聚合）
    static nlohmann::json getMyMerchant(long long userId);
    // 更新我的商户资料（部分更新）
    static void updateMyMerchant(long long userId, const nlohmann::json& body);

    // 管理端
    static nlohmann::json adminList(const std::string& status, const std::string& keyword,
                                    int page, int size);
    static nlohmann::json adminCounts();
    static void audit(long long merchantId, const nlohmann::json& body);

    // ---- 门店（商家端，须归属且商户已通过审核）----
    static nlohmann::json listStores(long long userId);
    static nlohmann::json addStore(long long userId, const nlohmann::json& body);
    static void updateStore(long long userId, long long storeId, const nlohmann::json& body);
    static void deleteStore(long long userId, long long storeId);

    // ---- 服务项目 ----
    static nlohmann::json listServices(long long userId, const std::string& status);
    static nlohmann::json addService(long long userId, const nlohmann::json& body);
    static void updateService(long long userId, long long serviceId, const nlohmann::json& body);
    static void setServiceStatus(long long userId, long long serviceId, const std::string& status);

    // ---- 消费套餐 ----
    static nlohmann::json listPackages(long long userId, const std::string& status);
    static nlohmann::json addPackage(long long userId, const nlohmann::json& body);
    static void updatePackage(long long userId, long long packageId, const nlohmann::json& body);
    static void setPackageStatus(long long userId, long long packageId,
                                 const std::string& status);
};
