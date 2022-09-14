/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MOCK_OHOS_ABILITY_RUNTIME_MOCK_BUNDLE_MANAGER_FORM_H
#define MOCK_OHOS_ABILITY_RUNTIME_MOCK_BUNDLE_MANAGER_FORM_H

#include <vector>

#include "ability_info.h"
#include "application_info.h"
#include "bundle_mgr_interface.h"
#include "gmock/gmock.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "want.h"

namespace OHOS {
namespace AppExecFwk {
class BundleMgrProxy : public IRemoteProxy<IBundleMgr> {
public:
    explicit BundleMgrProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IBundleMgr>(impl)
    {}
    virtual ~BundleMgrProxy()
    {}

    bool QueryAbilityInfo(const AAFwk::Want &want, AbilityInfo &abilityInfo) override
    {
        return true;
    }
    bool QueryAbilityInfoByUri(const std::string &uri, AbilityInfo &abilityInfo) override
    {
        return true;
    }

    std::string GetAppType(const std::string &bundleName) override
    {
        return "system";
    }

    bool GetApplicationInfo(
        const std::string &appName,
        const ApplicationFlag flag,
        const int userId,
        ApplicationInfo &appInfo) override
    {
        return true;
    }
    bool GetApplicationInfos(
        const ApplicationFlag flag, const int userId, std::vector<ApplicationInfo> &appInfos) override
    {
        return true;
    }

    bool GetBundleInfos(
        const BundleFlag flag, std::vector<BundleInfo> &bundleInfos, int32_t userId) override
    {
        return true;
    }
    int GetUidByBundleName(const std::string &bundleName, const int userId) override
    {
        if (bundleName.compare("com.form.host.app600") == 0) {
            return 600;
        }
        return 0;
    }
    std::string GetAppIdByBundleName(const std::string &bundleName, const int userId) override
    {
        return "";
    }
    bool GetBundleNameForUid(const int uid, std::string &bundleName) override
    {
        bundleName = "com.form.provider.service";
        return true;
    }
    bool GetBundleGids(const std::string &bundleName, std::vector<int> &gids) override
    {
        return true;
    }
    bool GetBundleInfosByMetaData(const std::string &metaData, std::vector<BundleInfo> &bundleInfos) override
    {
        return true;
    }
    bool QueryKeepAliveBundleInfos(std::vector<BundleInfo> &bundleInfos) override
    {
        return true;
    }

    bool GetBundleArchiveInfo(
        const std::string &hapFilePath, const BundleFlag flag, BundleInfo &bundleInfo) override
    {
        return true;
    }

    bool GetLaunchWantForBundle(const std::string &bundleName, Want &want) override
    {
        return true;
    }

    int CheckPublicKeys(const std::string &firstBundleName, const std::string &secondBundleName) override
    {
        return 0;
    }

    bool GetPermissionDef(const std::string &permissionName, PermissionDef &permissionDef) override
    {
        return true;
    }
    bool HasSystemCapability(const std::string &capName) override
    {
        return true;
    }
    bool GetSystemAvailableCapabilities(std::vector<std::string> &systemCaps) override
    {
        return true;
    }
    bool IsSafeMode() override
    {
        return true;
    }
    bool CleanBundleDataFiles(const std::string &bundleName, const int userId) override
    {
        return true;
    }
    bool RegisterBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback) override
    {
        return true;
    }
    bool ClearBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback) override
    {
        return true;
    }
    // unregister callback of all application
    bool UnregisterBundleStatusCallback() override
    {
        return true;
    }
    bool DumpInfos(
        const DumpFlag flag, const std::string &bundleName, int32_t userId, std::string &result) override
    {
        return true;
    }
    sptr<IBundleInstaller> GetBundleInstaller() override
    {
        return nullptr;
    }
    sptr<IBundleUserMgr> GetBundleUserMgr() override
    {
        return nullptr;
    };

    /**
     *  @brief Obtains information about the shortcuts of the application.
     *  @param bundleName Indicates the name of the bundle to shortcut.
     *  @param form Indicates the callback a list to shortcutinfo.
     *  @return Returns true if shortcutinfo get success
     */
    bool GetShortcutInfos(const std::string &bundleName, std::vector<ShortcutInfo> &shortcut) override
    {
        return true;
    }
    /**
     * @brief Obtain the HAP module info of a specific ability.
     * @param abilityInfo Indicates the ability.
     * @param hapModuleInfo Indicates the obtained HapModuleInfo object.
     * @return Returns true if the HapModuleInfo is successfully obtained; returns false otherwise.
     */
    bool GetHapModuleInfo(const AbilityInfo &abilityInfo, HapModuleInfo &hapModuleInfo) override
    {
        return true;
    }

    bool GetHapModuleInfo(const AbilityInfo &abilityInfo, int32_t userId, HapModuleInfo &hapModuleInfo) override
    {
        return true;
    }

    bool CheckIsSystemAppByUid(const int uid) override
    {
        if (uid == 600) {
            return true;
        }
        return false;
    }

    virtual ErrCode IsApplicationEnabled(const std::string &bundleName, bool &isEnable) override
    {
        return ERR_OK;
    }

    bool GetBundleInfo(
        const std::string &bundleName, const BundleFlag flag, BundleInfo &bundleInfo, int32_t userId) override;
    bool GetAllFormsInfo(std::vector<FormInfo> &formInfo) override;
    bool GetFormsInfoByApp(const std::string &bundleName, std::vector<FormInfo> &formInfo) override;
    bool GetFormsInfoByModule(
        const std::string &bundleName,
        const std::string &moduleName,
        std::vector<FormInfo> &formInfo) override;
};

class BundleMgrStub : public IRemoteStub<IBundleMgr> {
public:
    ~BundleMgrStub() = default;
    int OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
};

class BundleMgrService : public BundleMgrStub {
public:
    ~BundleMgrService() = default;
    MOCK_METHOD2(GetAppIdByBundleName, std::string(const std::string &bundleName, const int userId));
    MOCK_METHOD2(CheckPermission, int(const std::string &bundleName, const std::string &permission));
    MOCK_METHOD2(CleanBundleDataFiles, bool(const std::string &bundleName, const int userId));
    MOCK_METHOD3(
        CanRequestPermission, bool(const std::string &bundleName, const std::string &permissionName, const int userId));
    MOCK_METHOD3(RequestPermissionFromUser,
        bool(const std::string &bundleName, const std::string &permission, const int userId));
    MOCK_METHOD2(GetNameForUid, bool(const int uid, std::string &name));
    MOCK_METHOD2(GetBundlesForUid, bool(const int uid, std::vector<std::string> &));
    MOCK_METHOD2(IsAbilityEnabled, ErrCode(const AbilityInfo &, bool &isEnable));
    bool QueryAbilityInfo(const AAFwk::Want &want, AbilityInfo &abilityInfo) override;
    bool QueryAbilityInfoByUri(const std::string &uri, AbilityInfo &abilityInfo) override;

    std::string GetAppType(const std::string &bundleName) override;
    int GetUidByBundleName(const std::string &bundleName, const int userId) override;

    bool GetApplicationInfo(
        const std::string &appName, const ApplicationFlag flag, const int userId, ApplicationInfo &appInfo) override;
    bool GetApplicationInfos(
        const ApplicationFlag flag, const int userId, std::vector<ApplicationInfo> &appInfos) override
    {
        return true;
    };
    bool GetBundleInfo(
        const std::string &bundleName, const BundleFlag flag, BundleInfo &bundleInfo, int32_t userId) override;
    bool GetBundleInfos(
        const BundleFlag flag, std::vector<BundleInfo> &bundleInfos, int32_t userId) override
    {
        return true;
    };
    bool GetBundleNameForUid(const int uid, std::string &bundleName) override
    {
        bundleName = "com.form.provider.service";
        return true;
    };
    bool GetBundleGids(const std::string &bundleName, std::vector<int> &gids) override;
    bool GetBundleInfosByMetaData(const std::string &metaData, std::vector<BundleInfo> &bundleInfos) override
    {
        return true;
    };
    bool QueryKeepAliveBundleInfos(std::vector<BundleInfo> &bundleInfos) override
    {
        return true;
    };
    // obtains information about an application bundle contained in an ohos Ability Package (HAP).
    bool GetBundleArchiveInfo(
        const std::string &hapFilePath, const BundleFlag flag, BundleInfo &bundleInfo) override
    {
        return true;
    };
    bool GetHapModuleInfo(const AbilityInfo &abilityInfo, HapModuleInfo &hapModuleInfo) override
    {
        return true;
    }
    bool GetHapModuleInfo(const AbilityInfo &abilityInfo, int32_t userId, HapModuleInfo &hapModuleInfo) override
    {
        return true;
    }
    // obtains the Want for starting the main ability of an application based on the given bundle name.
    bool GetLaunchWantForBundle(const std::string &bundleName, Want &want) override
    {
        return true;
    };
    // checks whether the publickeys of two bundles are the same.
    int CheckPublicKeys(const std::string &firstBundleName, const std::string &secondBundleName) override
    {
        return 0;
    };
    // checks whether a specified bundle has been granted a specific permission.
    bool GetPermissionDef(const std::string &permissionName, PermissionDef &permissionDef) override
    {
        return true;
    };
    bool HasSystemCapability(const std::string &capName) override
    {
        return true;
    };
    bool GetSystemAvailableCapabilities(std::vector<std::string> &systemCaps) override
    {
        return true;
    };
    bool IsSafeMode() override
    {
        return true;
    };

    bool RegisterBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback) override
    {
        return true;
    };
    bool ClearBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback) override
    {
        return true;
    };
    // unregister callback of all application
    bool UnregisterBundleStatusCallback() override
    {
        return true;
    };
    bool DumpInfos(
        const DumpFlag flag, const std::string &bundleName, int32_t userId, std::string &result) override
    {
        return true;
    };
    sptr<IBundleInstaller> GetBundleInstaller() override
    {
        return nullptr;
    };
    sptr<IBundleUserMgr> GetBundleUserMgr() override
    {
        return nullptr;
    };

    virtual ErrCode IsApplicationEnabled(const std::string &bundleName, bool &isEnable) override
    {
        return ERR_OK;
    };
    bool CheckIsSystemAppByUid(const int uid) override
    {
        if (uid == 600) {
            return false;
        }

        return true;
    };

    /**
     *  @brief Obtains information about the shortcuts of the application.
     *  @param bundleName Indicates the name of the bundle to shortcut.
     *  @param form Indicates the callback a list to shortcutinfo.
     *  @return Returns true if shortcutinfo get success
     */
    bool GetShortcutInfos(const std::string &bundleName, std::vector<ShortcutInfo> &shortcut) override
    {
        return true;
    }
    bool GetAllFormsInfo(std::vector<FormInfo> &formInfo) override;
    bool GetFormsInfoByApp(const std::string &bundleName, std::vector<FormInfo> &formInfo) override;
    bool GetFormsInfoByModule(
        const std::string &bundleName,
        const std::string &moduleName,
        std::vector<FormInfo> &formInfo) override;
    bool QueryAbilityInfos(const Want &want, std::vector<AbilityInfo> &abilityInfos) override
    {
        return true;
    };
    bool GetBundleGidsByUid(const std::string &bundleName, const int &uid, std::vector<int> &gids) override
    {
        return true;
    }
    bool QueryAbilityInfosByUri(const std::string &abilityUri, std::vector<AbilityInfo> &abilityInfos) override
    {
        return true;
    }
    bool GetAllCommonEventInfo(const std::string &eventKey,
        std::vector<CommonEventInfo> &commonEventInfos) override
    {
        return true;
    }
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // MOCK_OHOS_ABILITY_RUNTIME_MOCK_BUNDLE_MANAGER_FORM_H
