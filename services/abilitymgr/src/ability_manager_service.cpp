/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ability_manager_service.h"

#include <fstream>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <csignal>

#include "ability_info.h"
#include "ability_manager_errors.h"
#include "ability_util.h"
#include "bytrace.h"
#include "bundle_mgr_client.h"
#include "distributed_client.h"
#include "hilog_wrapper.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "locale_config.h"
#include "lock_screen_white_list.h"
#include "mission/mission_info_converter.h"
#include "sa_mgr_client.h"
#include "softbus_bus_center.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "png.h"
#include "ui_service_mgr_client.h"

using OHOS::AppExecFwk::ElementName;

namespace OHOS {
namespace AAFwk {
using namespace std::chrono;
const int32_t MAIN_USER_ID = 100;
const int32_t BASE_USER_RANGE = 200000;
static const int EXPERIENCE_MEM_THRESHOLD = 20;
constexpr auto DATA_ABILITY_START_TIMEOUT = 5s;
constexpr int32_t NON_ANONYMIZE_LENGTH = 6;
const int32_t EXTENSION_SUPPORT_API_VERSION = 8;
const int32_t MAX_NUMBER_OF_DISTRIBUTED_MISSIONS = 20;
const int32_t MAX_NUMBER_OF_CONNECT_BMS = 15;
const std::string EMPTY_DEVICE_ID = "";
const std::string PKG_NAME = "ohos.distributedhardware.devicemanager";
const std::string ACTION_CHOOSE = "ohos.want.action.select";
const std::map<std::string, AbilityManagerService::DumpKey> AbilityManagerService::dumpMap = {
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--all", KEY_DUMP_ALL),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-a", KEY_DUMP_ALL),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--stack-list", KEY_DUMP_STACK_LIST),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-l", KEY_DUMP_STACK_LIST),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--stack", KEY_DUMP_STACK),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-s", KEY_DUMP_STACK),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--mission", KEY_DUMP_MISSION),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-m", KEY_DUMP_MISSION),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--top", KEY_DUMP_TOP_ABILITY),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-t", KEY_DUMP_TOP_ABILITY),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--waitting-queue", KEY_DUMP_WAIT_QUEUE),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-w", KEY_DUMP_WAIT_QUEUE),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--serv", KEY_DUMP_SERVICE),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-e", KEY_DUMP_SERVICE),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--data", KEY_DUMP_DATA),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-d", KEY_DUMP_DATA),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--ui", KEY_DUMP_SYSTEM_UI),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-u", KEY_DUMP_SYSTEM_UI),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-focus", KEY_DUMP_FOCUS_ABILITY),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-f", KEY_DUMP_FOCUS_ABILITY),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--win-mode", KEY_DUMP_WINDOW_MODE),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-z", KEY_DUMP_WINDOW_MODE),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--mission-list", KEY_DUMP_MISSION_LIST),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-L", KEY_DUMP_MISSION_LIST),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("--mission-infos", KEY_DUMP_MISSION_INFOS),
    std::map<std::string, AbilityManagerService::DumpKey>::value_type("-S", KEY_DUMP_MISSION_INFOS),
};
const bool REGISTER_RESULT =
    SystemAbility::MakeAndRegisterAbility(DelayedSingleton<AbilityManagerService>::GetInstance().get());

AbilityManagerService::AbilityManagerService()
    : SystemAbility(ABILITY_MGR_SERVICE_ID, true),
      eventLoop_(nullptr),
      handler_(nullptr),
      state_(ServiceRunningState::STATE_NOT_START),
      iBundleManager_(nullptr)
{
    std::shared_ptr<AppScheduler> appScheduler(
        DelayedSingleton<AppScheduler>::GetInstance().get(), [](AppScheduler *x) { x->DecStrongRef(x); });
    appScheduler_ = appScheduler;
    DumpFuncInit();
}

AbilityManagerService::~AbilityManagerService()
{}

void AbilityManagerService::OnStart()
{
    if (state_ == ServiceRunningState::STATE_RUNNING) {
        HILOG_INFO("Ability manager service has already started.");
        return;
    }
    HILOG_INFO("Ability manager service started.");
    if (!Init()) {
        HILOG_ERROR("Failed to init service.");
        return;
    }
    state_ = ServiceRunningState::STATE_RUNNING;
    eventLoop_->Run();
    /* Publish service maybe failed, so we need call this function at the last,
     * so it can't affect the TDD test program */
    bool ret = Publish(DelayedSingleton<AbilityManagerService>::GetInstance().get());
    if (!ret) {
        HILOG_ERROR("Init publish failed!");
        return;
    }

    HILOG_INFO("Ability manager service start success.");
}

bool AbilityManagerService::Init()
{
    eventLoop_ = AppExecFwk::EventRunner::Create(AbilityConfig::NAME_ABILITY_MGR_SERVICE);
    CHECK_POINTER_RETURN_BOOL(eventLoop_);

    handler_ = std::make_shared<AbilityEventHandler>(eventLoop_, weak_from_this());
    CHECK_POINTER_RETURN_BOOL(handler_);

    // init user controller.
    userController_ = std::make_shared<UserController>();
    userController_->Init();
    int userId = MAIN_USER_ID;

    InitConnectManager(userId, true);
    InitDataAbilityManager(userId, true);
    InitPendWantManager(userId, true);
    systemDataAbilityManager_ = std::make_shared<DataAbilityManager>();

    amsConfigResolver_ = std::make_shared<AmsConfigurationParameter>();
    if (amsConfigResolver_) {
        amsConfigResolver_->Parse();
        HILOG_INFO("ams config parse");
    }
    useNewMission_ = amsConfigResolver_->IsUseNewMission();

    SetStackManager(userId, true);
    systemAppManager_ = std::make_shared<KernalSystemAppManager>(0);
    CHECK_POINTER_RETURN_BOOL(systemAppManager_);

    InitMissionListManager(userId, true);
    kernalAbilityManager_ = std::make_shared<KernalAbilityManager>(0);
    CHECK_POINTER_RETURN_BOOL(kernalAbilityManager_);

    auto startLauncherAbilityTask = [aams = shared_from_this()]() { aams->StartSystemApplication(); };
    handler_->PostTask(startLauncherAbilityTask, "startLauncherAbility");
    auto creatWhiteListTask = [aams = shared_from_this()]() {
        if (access(AmsWhiteList::AMS_WHITE_LIST_DIR_PATH.c_str(), F_OK) != 0) {
            if (mkdir(AmsWhiteList::AMS_WHITE_LIST_DIR_PATH.c_str(), S_IRWXO | S_IRWXG | S_IRWXU)) {
                HILOG_ERROR("mkdir AmsWhiteList::AMS_WHITE_LIST_DIR_PATH Fail");
                return;
            }
        }
        if (aams->IsExistFile(AmsWhiteList::AMS_WHITE_LIST_FILE_PATH)) {
            HILOG_INFO("file exists");
            return;
        }
        HILOG_INFO("no such file,creat...");
        std::ofstream outFile(AmsWhiteList::AMS_WHITE_LIST_FILE_PATH, std::ios::out);
        outFile.close();
    };
    handler_->PostTask(creatWhiteListTask, "creatWhiteList");
    HILOG_INFO("Init success.");
    return true;
}

void AbilityManagerService::OnStop()
{
    HILOG_INFO("Stop service.");
    eventLoop_.reset();
    handler_.reset();
    state_ = ServiceRunningState::STATE_NOT_START;
}

ServiceRunningState AbilityManagerService::QueryServiceState() const
{
    return state_;
}

int AbilityManagerService::StartAbility(const Want &want, int requestCode)
{
    HILOG_INFO("%{public}s", __func__);
    return StartAbility(want, nullptr, requestCode, -1);
}

int AbilityManagerService::StartAbility(const Want &want, const sptr<IRemoteObject> &callerToken, int requestCode)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    auto flags = want.GetFlags();
    if ((flags & Want::FLAG_ABILITY_CONTINUATION) == Want::FLAG_ABILITY_CONTINUATION) {
        HILOG_ERROR("StartAbility with continuation flags is not allowed!");
        return ERR_INVALID_VALUE;
    }
    HILOG_INFO("%{public}s", __func__);
    if (CheckIfOperateRemote(want)) {
        HILOG_INFO("AbilityManagerService::StartAbility. try to StartRemoteAbility");
        return StartRemoteAbility(want, requestCode);
    }
    HILOG_INFO("AbilityManagerService::StartAbility. try to StartLocalAbility");
    return StartAbility(want, callerToken, requestCode, -1);
}

int AbilityManagerService::StartAbility(
    const Want &want, const sptr<IRemoteObject> &callerToken, int requestCode, int callerUid)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("%{public}s begin.", __func__);
    if (callerToken != nullptr && !VerificationToken(callerToken)) {
        HILOG_ERROR("%{public}s VerificationToken failed.", __func__);
        return ERR_INVALID_VALUE;
    }
    AbilityRequest abilityRequest;
    int result = GenerateAbilityRequest(want, requestCode, abilityRequest, callerToken);
    if (result != ERR_OK) {
        HILOG_ERROR("Generate ability request error.");
        return result;
    }
    auto abilityInfo = abilityRequest.abilityInfo;
    result = AbilityUtil::JudgeAbilityVisibleControl(abilityInfo, callerUid);
    if (result != ERR_OK) {
        HILOG_ERROR("%{public}s JudgeAbilityVisibleControl error.", __func__);
        return result;
    }
    auto type = abilityInfo.type;
    HILOG_INFO("%{public}s Current ability type:%{public}d", __func__, type);
    if (type == AppExecFwk::AbilityType::DATA) {
        HILOG_ERROR("Cannot start data ability, use 'AcquireDataAbility()' instead.");
        return ERR_INVALID_VALUE;
    }
    if (!AbilityUtil::IsSystemDialogAbility(abilityInfo.bundleName, abilityInfo.name)) {
        HILOG_INFO("%{public}s PreLoadAppDataAbilities:%{public}s", __func__, abilityInfo.bundleName.c_str());
        result = PreLoadAppDataAbilities(abilityInfo.bundleName);
        if (result != ERR_OK) {
            HILOG_ERROR("StartAbility: App data ability preloading failed, '%{public}s', %{public}d",
                abilityInfo.bundleName.c_str(), result);
            return result;
        }
    }
    UpdateCallerInfo(abilityRequest.want);
    if (type == AppExecFwk::AbilityType::SERVICE || type == AppExecFwk::AbilityType::EXTENSION) {
        HILOG_INFO("%{public}s Start SERVICE or EXTENSION", __func__);
        return connectManager_->StartAbility(abilityRequest);
    }

    if (!IsAbilityControllerStart(want, abilityInfo.bundleName)) {
        HILOG_ERROR("IsAbilityControllerStart failed: %{public}s", abilityInfo.bundleName.c_str());
        return ERR_WOULD_BLOCK;
    }

    if (useNewMission_) {
        if (IsSystemUiApp(abilityRequest.abilityInfo)) {
            HILOG_INFO("%{public}s NewMission Start SystemUiApp", __func__);
            return kernalAbilityManager_->StartAbility(abilityRequest);
        }
        HILOG_INFO("%{public}s StartAbility by MissionList", __func__);
        return currentMissionListManager_->StartAbility(abilityRequest);
    } else {
        if (IsSystemUiApp(abilityRequest.abilityInfo)) {
            HILOG_INFO("%{public}s OldMission Start SystemUiApp", __func__);
            return systemAppManager_->StartAbility(abilityRequest);
        }
        HILOG_INFO("%{public}s StartAbility by StackManager", __func__);
        return currentStackManager_->StartAbility(abilityRequest);
    }
}

int AbilityManagerService::StartAbility(const Want &want, const AbilityStartSetting &abilityStartSetting,
    const sptr<IRemoteObject> &callerToken, int requestCode)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("Start ability setting.");
    if (callerToken != nullptr && !VerificationToken(callerToken)) {
        return ERR_INVALID_VALUE;
    }

    AbilityRequest abilityRequest;
    int result = GenerateAbilityRequest(want, requestCode, abilityRequest, callerToken);
    if (result != ERR_OK) {
        HILOG_ERROR("Generate ability request error.");
        return result;
    }
    auto abilityInfo = abilityRequest.abilityInfo;
    result = AbilityUtil::JudgeAbilityVisibleControl(abilityInfo);
    if (result != ERR_OK) {
        HILOG_ERROR("%{public}s JudgeAbilityVisibleControl error.", __func__);
        return result;
    }

    abilityRequest.startSetting = std::make_shared<AbilityStartSetting>(abilityStartSetting);

    if (abilityInfo.type == AppExecFwk::AbilityType::DATA) {
        HILOG_ERROR("Cannot start data ability, use 'AcquireDataAbility()' instead.");
        return ERR_INVALID_VALUE;
    }

    if (!AbilityUtil::IsSystemDialogAbility(abilityInfo.bundleName, abilityInfo.name)) {
        result = PreLoadAppDataAbilities(abilityInfo.bundleName);
        if (result != ERR_OK) {
            HILOG_ERROR("StartAbility: App data ability preloading failed, '%{public}s', %{public}d",
                abilityInfo.bundleName.c_str(),
                result);
            return result;
        }
    }

    if (abilityInfo.type != AppExecFwk::AbilityType::PAGE) {
        HILOG_ERROR("Only support for page type ability.");
        return ERR_INVALID_VALUE;
    }

    if (!IsAbilityControllerStart(want, abilityInfo.bundleName)) {
        return ERR_WOULD_BLOCK;
    }
    if (useNewMission_) {
        if (IsSystemUiApp(abilityRequest.abilityInfo)) {
            return kernalAbilityManager_->StartAbility(abilityRequest);
        }
        return currentMissionListManager_->StartAbility(abilityRequest);
    } else {
        if (IsSystemUiApp(abilityRequest.abilityInfo)) {
            return systemAppManager_->StartAbility(abilityRequest);
        }

        return currentStackManager_->StartAbility(abilityRequest);
    }
}

int AbilityManagerService::StartAbility(const Want &want, const StartOptions &startOptions,
    const sptr<IRemoteObject> &callerToken, int requestCode)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("Start ability options.");
    if (callerToken != nullptr && !VerificationToken(callerToken)) {
        return ERR_INVALID_VALUE;
    }

    AbilityRequest abilityRequest;
    int result = GenerateAbilityRequest(want, requestCode, abilityRequest, callerToken);
    if (result != ERR_OK) {
        HILOG_ERROR("Generate ability request error.");
        return result;
    }

    auto abilityInfo = abilityRequest.abilityInfo;
    result = AbilityUtil::JudgeAbilityVisibleControl(abilityInfo);
    if (result != ERR_OK) {
        HILOG_ERROR("%{public}s JudgeAbilityVisibleControl error.", __func__);
        return result;
    }

    if (abilityInfo.type != AppExecFwk::AbilityType::PAGE) {
        HILOG_ERROR("Only support for page type ability.");
        return ERR_INVALID_VALUE;
    }

    if (!AbilityUtil::IsSystemDialogAbility(abilityInfo.bundleName, abilityInfo.name)) {
        result = PreLoadAppDataAbilities(abilityInfo.bundleName);
        if (result != ERR_OK) {
            HILOG_ERROR("StartAbility: App data ability preloading failed, '%{public}s', %{public}d",
                abilityInfo.bundleName.c_str(),
                result);
            return result;
        }
    }

    if (!IsAbilityControllerStart(want, abilityInfo.bundleName)) {
        return ERR_WOULD_BLOCK;
    }
    if (IsSystemUiApp(abilityRequest.abilityInfo)) {
        if (useNewMission_) {
            return kernalAbilityManager_->StartAbility(abilityRequest);
        } else {
            return systemAppManager_->StartAbility(abilityRequest);
        }
    }
    abilityRequest.want.SetParam(StartOptions::STRING_DISPLAY_ID, startOptions.GetDisplayID());
    abilityRequest.want.SetParam(Want::PARAM_RESV_WINDOW_MODE, startOptions.GetWindowMode());
    if (useNewMission_) {
        return currentMissionListManager_->StartAbility(abilityRequest);
    } else {
        return currentStackManager_->StartAbility(abilityRequest);
    }
}

int AbilityManagerService::TerminateAbility(const sptr<IRemoteObject> &token, int resultCode, const Want *resultWant)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("Terminate ability for result: %{public}d", (resultWant != nullptr));
    if (!VerificationToken(token)) {
        return ERR_INVALID_VALUE;
    }

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);
    int result = AbilityUtil::JudgeAbilityVisibleControl(abilityRecord->GetAbilityInfo());
    if (result != ERR_OK) {
        HILOG_ERROR("%{public}s JudgeAbilityVisibleControl error.", __func__);
        return result;
    }

    if (IsSystemUiApp(abilityRecord->GetAbilityInfo())) {
        HILOG_ERROR("System ui not allow terminate.");
        return ERR_INVALID_VALUE;
    }

    auto type = abilityRecord->GetAbilityInfo().type;
    if (type == AppExecFwk::AbilityType::SERVICE || type == AppExecFwk::AbilityType::EXTENSION) {
        return connectManager_->TerminateAbility(token);
    }

    if (type == AppExecFwk::AbilityType::DATA) {
        HILOG_ERROR("Cannot terminate data ability, use 'ReleaseDataAbility()' instead.");
        return ERR_INVALID_VALUE;
    }

    if ((resultWant != nullptr) &&
        AbilityUtil::IsSystemDialogAbility(
        abilityRecord->GetAbilityInfo().bundleName, abilityRecord->GetAbilityInfo().name) &&
        resultWant->HasParameter(AbilityConfig::SYSTEM_DIALOG_KEY) &&
        resultWant->HasParameter(AbilityConfig::SYSTEM_DIALOG_CALLER_BUNDLENAME) &&
        resultWant->HasParameter(AbilityConfig::SYSTEM_DIALOG_REQUEST_PERMISSIONS)) {
        RequestPermission(resultWant);
    }

    if (!IsAbilityControllerForeground(abilityRecord->GetAbilityInfo().bundleName)) {
        return ERR_WOULD_BLOCK;
    }

    if (useNewMission_) {
        return currentMissionListManager_->TerminateAbility(abilityRecord, resultCode, resultWant);
    } else {
        return currentStackManager_->TerminateAbility(token, resultCode, resultWant);
    }
}

int AbilityManagerService::StartRemoteAbility(const Want &want, int requestCode)
{
    HILOG_INFO("%{public}s", __func__);
    want.DumpInfo(0);
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    DistributedClient dmsClient;
    HILOG_INFO("AbilityManagerService::Try to StartRemoteAbility, callerUid = %{public}d", callerUid);
    int result = dmsClient.StartRemoteAbility(want, callerUid, requestCode);
    if (result != ERR_NONE) {
        HILOG_ERROR("AbilityManagerService::StartRemoteAbility failed, result = %{public}d", result);
    }
    return result;
}

bool AbilityManagerService::CheckIsRemote(const std::string& deviceId)
{
    HILOG_INFO("check is remote, deviceId = %{public}s", AnonymizeDeviceId(deviceId).c_str());
    if (deviceId.empty()) {
        HILOG_ERROR("CheckIsRemote: device id is empty.");
        return false;
    }

    std::string localDeviceId;
    if (!GetLocalDeviceId(localDeviceId) || localDeviceId == deviceId) {
        HILOG_ERROR("CheckIsRemote: Check DeviceId failed");
        return false;
    }
    return true;
}

bool AbilityManagerService::CheckIfOperateRemote(const Want &want)
{
    std::string localDeviceId;
    std::string deviceId = want.GetElement().GetDeviceID();
    HILOG_INFO("get deviceId, deviceId = %{public}s", AnonymizeDeviceId(deviceId).c_str());
    if (deviceId.empty() || want.GetElement().GetBundleName().empty() ||
        want.GetElement().GetAbilityName().empty()) {
        HILOG_ERROR("CheckIfOperateRemote: GetDeviceId,or GetBundleName, or GetAbilityName failed");
        return false;
    }
    if (!GetLocalDeviceId(localDeviceId) || localDeviceId == deviceId) {
        HILOG_ERROR("CheckIfOperateRemote: Check DeviceId failed");
        return false;
    }
    return true;
}

bool AbilityManagerService::GetLocalDeviceId(std::string& localDeviceId)
{
    auto localNode = std::make_unique<NodeBasicInfo>();
    int32_t errCode = GetLocalNodeDeviceInfo(PKG_NAME.c_str(), localNode.get());
    if (errCode != ERR_OK) {
        HILOG_ERROR("AbilityManagerService::GetLocalNodeDeviceInfo errCode = %{public}d", errCode);
        return false;
    }
    if (localNode != nullptr) {
        localDeviceId = localNode->networkId;
        HILOG_INFO("get local deviceId, deviceId = %{public}s",
            AnonymizeDeviceId(localDeviceId).c_str());
        return true;
    }
    HILOG_ERROR("AbilityManagerService::GetLocalDeviceId localDeviceId null");
    return false;
}

std::string AbilityManagerService::AnonymizeDeviceId(const std::string& deviceId)
{
    if (deviceId.length() < NON_ANONYMIZE_LENGTH) {
        return EMPTY_DEVICE_ID;
    }
    std::string anonDeviceId = deviceId.substr(0, NON_ANONYMIZE_LENGTH);
    anonDeviceId.append("******");
    return anonDeviceId;
}

void AbilityManagerService::RequestPermission(const Want *resultWant)
{
    HILOG_INFO("Request permission.");
    CHECK_POINTER(iBundleManager_);
    CHECK_POINTER_IS_NULLPTR(resultWant);

    auto callerBundleName = resultWant->GetStringParam(AbilityConfig::SYSTEM_DIALOG_CALLER_BUNDLENAME);
    auto permissions = resultWant->GetStringArrayParam(AbilityConfig::SYSTEM_DIALOG_REQUEST_PERMISSIONS);

    for (auto &it : permissions) {
        auto ret = iBundleManager_->RequestPermissionFromUser(callerBundleName, it, GetUserId());
        HILOG_INFO("Request permission from user result :%{public}d, permission:%{public}s.", ret, it.c_str());
    }
}

int AbilityManagerService::TerminateAbilityByCaller(const sptr<IRemoteObject> &callerToken, int requestCode)
{
    HILOG_INFO("Terminate ability by caller.");
    if (!VerificationToken(callerToken)) {
        return ERR_INVALID_VALUE;
    }

    auto abilityRecord = Token::GetAbilityRecordByToken(callerToken);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);

    if (IsSystemUiApp(abilityRecord->GetAbilityInfo())) {
        HILOG_ERROR("System ui not allow terminate.");
        return ERR_INVALID_VALUE;
    }

    auto type = abilityRecord->GetAbilityInfo().type;
    switch (type) {
        case AppExecFwk::AbilityType::SERVICE:
        case AppExecFwk::AbilityType::EXTENSION: {
            auto result = connectManager_->TerminateAbility(abilityRecord, requestCode);
            if (result == NO_FOUND_ABILITY_BY_CALLER) {
                if (!IsAbilityControllerForeground(abilityRecord->GetAbilityInfo().bundleName)) {
                    return ERR_WOULD_BLOCK;
                }
                return currentStackManager_->TerminateAbility(abilityRecord, requestCode);
            }
            return result;
        }
        case AppExecFwk::AbilityType::PAGE: {
            if (!IsAbilityControllerForeground(abilityRecord->GetAbilityInfo().bundleName)) {
                return ERR_WOULD_BLOCK;
            }
            auto result = currentStackManager_->TerminateAbility(abilityRecord, requestCode);
            if (result == NO_FOUND_ABILITY_BY_CALLER) {
                return connectManager_->TerminateAbility(abilityRecord, requestCode);
            }
            return result;
        }
        default:
            return ERR_INVALID_VALUE;
    }
}

int AbilityManagerService::MinimizeAbility(const sptr<IRemoteObject> &token)
{
    HILOG_INFO("Minimize ability.");
    if (!VerificationToken(token) && !VerificationAllToken(token)) {
        return ERR_INVALID_VALUE;
    }

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);
    int result = AbilityUtil::JudgeAbilityVisibleControl(abilityRecord->GetAbilityInfo());
    if (result != ERR_OK) {
        HILOG_ERROR("%{public}s JudgeAbilityVisibleControl error.", __func__);
        return result;
    }

    auto type = abilityRecord->GetAbilityInfo().type;
    if (type != AppExecFwk::AbilityType::PAGE) {
        HILOG_ERROR("Cannot minimize except page ability.");
        return ERR_INVALID_VALUE;
    }

    if (!IsAbilityControllerForeground(abilityRecord->GetAbilityInfo().bundleName)) {
        return ERR_WOULD_BLOCK;
    }

    if (useNewMission_) {
        auto manager = GetListManagerByToken(token);
        CHECK_POINTER_AND_RETURN(manager, ERR_INVALID_VALUE);
        return manager->MinimizeAbility(token);
    } else {
        auto manager = GetStackManagerByToken(token);
        CHECK_POINTER_AND_RETURN(manager, ERR_INVALID_VALUE);
        return manager->MinimizeAbility(token);
    }
}

int AbilityManagerService::GetRecentMissions(
    const int32_t numMax, const int32_t flags, std::vector<AbilityMissionInfo> &recentList)
{
    HILOG_INFO("numMax: %{public}d, flags: %{public}d", numMax, flags);
    if (numMax < 0 || flags < 0) {
        HILOG_ERROR("numMax or flags is invalid.");
        return ERR_INVALID_VALUE;
    }
    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not systemApp");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    return currentStackManager_->GetRecentMissions(numMax, flags, recentList);
}

int AbilityManagerService::GetMissionSnapshot(const int32_t missionId, MissionPixelMap &missionPixelMap)
{
    if (missionId < 0) {
        HILOG_ERROR("GetMissionSnapshot failed.");
        return ERR_INVALID_VALUE;
    }
    return currentStackManager_->GetMissionSnapshot(missionId, missionPixelMap);
}

int AbilityManagerService::SetMissionDescriptionInfo(
    const sptr<IRemoteObject> &token, const MissionDescriptionInfo &description)
{
    HILOG_INFO("%{public}s called", __func__);
    CHECK_POINTER_AND_RETURN(token, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(currentStackManager_, INNER_ERR);

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);

    return currentStackManager_->SetMissionDescriptionInfo(abilityRecord, description);
}

int AbilityManagerService::GetMissionLockModeState()
{
    HILOG_INFO("%{public}s called", __func__);
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_INVALID_VALUE);
    return currentStackManager_->GetMissionLockModeState();
}

int AbilityManagerService::UpdateConfiguration(const AppExecFwk::Configuration &config)
{
    HILOG_INFO("%{public}s called", __func__);
    return DelayedSingleton<AppScheduler>::GetInstance()->UpdateConfiguration(config);
}

int AbilityManagerService::MoveMissionToTop(int32_t missionId)
{
    HILOG_INFO("Move mission to top.");
    if (missionId < 0) {
        HILOG_ERROR("Mission id is invalid.");
        return ERR_INVALID_VALUE;
    }

    return currentStackManager_->MoveMissionToTop(missionId);
}

int AbilityManagerService::MoveMissionToEnd(const sptr<IRemoteObject> &token, const bool nonFirst)
{
    HILOG_INFO("Move mission to end.");
    CHECK_POINTER_AND_RETURN(token, ERR_INVALID_VALUE);
    if (!VerificationToken(token)) {
        return ERR_INVALID_VALUE;
    }
    return currentStackManager_->MoveMissionToEnd(token, nonFirst);
}

int AbilityManagerService::RemoveMission(int id)
{
    HILOG_INFO("Remove mission.");
    if (id < 0) {
        HILOG_ERROR("Mission id is invalid.");
        return ERR_INVALID_VALUE;
    }
    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not systemApp");
        return CALLER_ISNOT_SYSTEMAPP;
    }
    return currentStackManager_->RemoveMissionById(id);
}

int AbilityManagerService::RemoveStack(int id)
{
    HILOG_INFO("Remove stack.");
    if (id < 0) {
        HILOG_ERROR("Stack id is invalid.");
        return ERR_INVALID_VALUE;
    }
    return currentStackManager_->RemoveStack(id);
}

int AbilityManagerService::ConnectAbility(
    const Want &want, const sptr<IAbilityConnection> &connect, const sptr<IRemoteObject> &callerToken)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("Connect ability.");
    CHECK_POINTER_AND_RETURN(connect, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(connect->AsObject(), ERR_INVALID_VALUE);

    if (CheckIfOperateRemote(want)) {
        HILOG_INFO("AbilityManagerService::ConnectAbility. try to ConnectRemoteAbility");
        return ConnectRemoteAbility(want, connect->AsObject());
    }
    return ConnectLocalAbility(want, connect, callerToken);
}

int AbilityManagerService::DisconnectAbility(const sptr<IAbilityConnection> &connect)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_DEBUG("Disconnect ability.");
    CHECK_POINTER_AND_RETURN(connect, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(connect->AsObject(), ERR_INVALID_VALUE);

    DisconnectLocalAbility(connect);
    DisconnectRemoteAbility(connect->AsObject());
    return ERR_OK;
}

int AbilityManagerService::ConnectLocalAbility(
    const Want &want, const sptr<IAbilityConnection> &connect, const sptr<IRemoteObject> &callerToken)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("%{public}s begin ConnectAbilityLocal", __func__);
    AbilityRequest abilityRequest;
    ErrCode result = GenerateAbilityRequest(want, DEFAULT_INVAL_VALUE, abilityRequest, callerToken);
    if (result != ERR_OK) {
        HILOG_ERROR("Generate ability request error.");
        return result;
    }
    auto abilityInfo = abilityRequest.abilityInfo;
    result = AbilityUtil::JudgeAbilityVisibleControl(abilityInfo);
    if (result != ERR_OK) {
        HILOG_ERROR("%{public}s JudgeAbilityVisibleControl error.", __func__);
        return result;
    }
    auto type = abilityInfo.type;
    if (type != AppExecFwk::AbilityType::SERVICE && type != AppExecFwk::AbilityType::EXTENSION) {
        HILOG_ERROR("Connect Ability failed, target Ability is not Service.");
        return TARGET_ABILITY_NOT_SERVICE;
    }
    result = PreLoadAppDataAbilities(abilityInfo.bundleName);
    if (result != ERR_OK) {
        HILOG_ERROR("ConnectAbility: App data ability preloading failed, '%{public}s', %{public}d",
            abilityInfo.bundleName.c_str(),
            result);
        return result;
    }
    return connectManager_->ConnectAbilityLocked(abilityRequest, connect, callerToken);
}

int AbilityManagerService::ConnectRemoteAbility(const Want &want, const sptr<IRemoteObject> &connect)
{
    HILOG_INFO("%{public}s begin ConnectAbilityRemote", __func__);
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    int32_t callerPid = IPCSkeleton::GetCallingPid();
    DistributedClient dmsClient;
    return dmsClient.ConnectRemoteAbility(want, connect, callerUid, callerPid);
}

int AbilityManagerService::DisconnectLocalAbility(const sptr<IAbilityConnection> &connect)
{
    HILOG_INFO("%{public}s begin DisconnectAbilityLocal", __func__);
    return connectManager_->DisconnectAbilityLocked(connect);
}

int AbilityManagerService::DisconnectRemoteAbility(const sptr<IRemoteObject> &connect)
{
    HILOG_INFO("%{public}s begin DisconnectAbilityRemote", __func__);
    DistributedClient dmsClient;
    return dmsClient.DisconnectRemoteAbility(connect);
}

int AbilityManagerService::ContinueMission(const std::string &srcDeviceId, const std::string &dstDeviceId,
    int32_t missionId, const sptr<IRemoteObject> &callBack, AAFwk::WantParams &wantParams)
{
    HILOG_INFO("ContinueMission srcDeviceId: %{public}s, dstDeviceId: %{public}s, missionId: %{public}d",
        srcDeviceId.c_str(), dstDeviceId.c_str(), missionId);
    DistributedClient dmsClient;
    return dmsClient.ContinueMission(srcDeviceId, dstDeviceId, missionId, callBack, wantParams);
}

int AbilityManagerService::ContinueAbility(const std::string &deviceId, int32_t missionId)
{
    HILOG_INFO("ContinueAbility deviceId : %{public}s, missionId = %{public}d.", deviceId.c_str(), missionId);

    sptr<IRemoteObject> abilityToken = GetAbilityTokenByMissionId(missionId);
    CHECK_POINTER_AND_RETURN(abilityToken, ERR_INVALID_VALUE);

    auto abilityRecord = Token::GetAbilityRecordByToken(abilityToken);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);

    abilityRecord->ContinueAbility(deviceId);
    return ERR_OK;
}

int AbilityManagerService::StartContinuation(const Want &want, const sptr<IRemoteObject> &abilityToken, int32_t status)
{
    HILOG_INFO("Start Continuation.");
    if (!CheckIfOperateRemote(want)) {
        HILOG_ERROR("deviceId or bundle name or abilityName empty");
        return ERR_INVALID_VALUE;
    }
    CHECK_POINTER_AND_RETURN(abilityToken, ERR_INVALID_VALUE);

    int32_t appUid = IPCSkeleton::GetCallingUid();
    int32_t missionId = GetMissionIdByAbilityToken(abilityToken);
    if (missionId == -1) {
        HILOG_ERROR("AbilityManagerService::StartContinuation failed to get missionId.");
        return ERR_INVALID_VALUE;
    }
    DistributedClient dmsClient;
    auto result =  dmsClient.StartContinuation(want, missionId, appUid, status);
    if (result != ERR_OK) {
        HILOG_ERROR("StartContinuation failed, result = %{public}d, notify caller", result);
        NotifyContinuationResult(missionId, result);
    }
    return result;
}

void AbilityManagerService::NotifyCompleteContinuation(const std::string &deviceId,
    int32_t sessionId, bool isSuccess)
{
    HILOG_INFO("NotifyCompleteContinuation.");
    DistributedClient dmsClient;
    dmsClient.NotifyCompleteContinuation(Str8ToStr16(deviceId), sessionId, isSuccess);
}

int AbilityManagerService::NotifyContinuationResult(int32_t missionId, const int32_t result)
{
    HILOG_INFO("Notify Continuation Result : %{public}d.", result);

    auto abilityToken = GetAbilityTokenByMissionId(missionId);
    CHECK_POINTER_AND_RETURN(abilityToken, ERR_INVALID_VALUE);

    auto abilityRecord = Token::GetAbilityRecordByToken(abilityToken);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);

    abilityRecord->NotifyContinuationResult(result);
    return ERR_OK;
}

int AbilityManagerService::StartSyncRemoteMissions(const std::string& devId, bool fixConflict, int64_t tag)
{
    DistributedClient dmsClient;
    return dmsClient.StartSyncRemoteMissions(devId, fixConflict, tag);
}

int AbilityManagerService::StopSyncRemoteMissions(const std::string& devId)
{
    DistributedClient dmsClient;
    return dmsClient.StopSyncRemoteMissions(devId);
}

int AbilityManagerService::RegisterMissionListener(const std::string &deviceId,
    const sptr<IRemoteMissionListener> &listener)
{
    std::string localDeviceId;
    if (!GetLocalDeviceId(localDeviceId) || localDeviceId == deviceId) {
        HILOG_ERROR("RegisterMissionListener: Check DeviceId failed");
        return REGISTER_REMOTE_MISSION_LISTENER_FAIL;
    }
    CHECK_POINTER_AND_RETURN(listener, ERR_INVALID_VALUE);
    DistributedClient dmsClient;
    return dmsClient.RegisterMissionListener(Str8ToStr16(deviceId), listener->AsObject());
}

int AbilityManagerService::UnRegisterMissionListener(const std::string &deviceId,
    const sptr<IRemoteMissionListener> &listener)
{
    std::string localDeviceId;
    if (!GetLocalDeviceId(localDeviceId) || localDeviceId == deviceId) {
        HILOG_ERROR("RegisterMissionListener: Check DeviceId failed");
        return REGISTER_REMOTE_MISSION_LISTENER_FAIL;
    }
    CHECK_POINTER_AND_RETURN(listener, ERR_INVALID_VALUE);
    DistributedClient dmsClient;
    return dmsClient.UnRegisterMissionListener(Str8ToStr16(deviceId), listener->AsObject());
}

void AbilityManagerService::RemoveAllServiceRecord()
{
    connectManager_->RemoveAll();
}

sptr<IWantSender> AbilityManagerService::GetWantSender(
    const WantSenderInfo &wantSenderInfo, const sptr<IRemoteObject> &callerToken)
{
    HILOG_INFO("Get want Sender.");
    CHECK_POINTER_AND_RETURN(pendingWantManager_, nullptr);

    auto bms = GetBundleManager();
    CHECK_POINTER_AND_RETURN(bms, nullptr);

    int32_t callerUid = IPCSkeleton::GetCallingUid();
    AppExecFwk::BundleInfo bundleInfo;
    if (!wantSenderInfo.bundleName.empty()) {
        bool bundleMgrResult =
            bms->GetBundleInfo(wantSenderInfo.bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo);
        if (!bundleMgrResult) {
            HILOG_ERROR("GetBundleInfo is fail.");
            return nullptr;
        }
    }

    HILOG_INFO("AbilityManagerService::GetWantSender: bundleName = %{public}s", wantSenderInfo.bundleName.c_str());
    return pendingWantManager_->GetWantSender(
        callerUid, bundleInfo.uid, bms->CheckIsSystemAppByUid(callerUid), wantSenderInfo, callerToken);
}

int AbilityManagerService::SendWantSender(const sptr<IWantSender> &target, const SenderInfo &senderInfo)
{
    HILOG_INFO("Send want sender.");
    CHECK_POINTER_AND_RETURN(pendingWantManager_, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(target, ERR_INVALID_VALUE);
    return pendingWantManager_->SendWantSender(target, senderInfo);
}

void AbilityManagerService::CancelWantSender(const sptr<IWantSender> &sender)
{
    HILOG_INFO("Cancel want sender.");
    CHECK_POINTER(pendingWantManager_);
    CHECK_POINTER(sender);

    auto bms = GetBundleManager();
    CHECK_POINTER(bms);

    int32_t callerUid = IPCSkeleton::GetCallingUid();
    sptr<PendingWantRecord> record = iface_cast<PendingWantRecord>(sender->AsObject());

    AppExecFwk::BundleInfo bundleInfo;
    bool bundleMgrResult =
        bms->GetBundleInfo(record->GetKey()->GetBundleName(), AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo);
    if (!bundleMgrResult) {
        HILOG_ERROR("GetBundleInfo is fail.");
        return;
    }

    pendingWantManager_->CancelWantSender(callerUid, bundleInfo.uid, bms->CheckIsSystemAppByUid(callerUid), sender);
}

int AbilityManagerService::GetPendingWantUid(const sptr<IWantSender> &target)
{
    HILOG_INFO("%{public}s:begin.", __func__);

    if (pendingWantManager_ == nullptr) {
        HILOG_ERROR("%s, pendingWantManager_ is nullptr", __func__);
        return -1;
    }
    if (target == nullptr) {
        HILOG_ERROR("%s, target is nullptr", __func__);
        return -1;
    }
    return pendingWantManager_->GetPendingWantUid(target);
}

int AbilityManagerService::GetPendingWantUserId(const sptr<IWantSender> &target)
{
    HILOG_INFO("%{public}s:begin.", __func__);

    if (pendingWantManager_ == nullptr) {
        HILOG_ERROR("%s, pendingWantManager_ is nullptr", __func__);
        return -1;
    }
    if (target == nullptr) {
        HILOG_ERROR("%s, target is nullptr", __func__);
        return -1;
    }
    return pendingWantManager_->GetPendingWantUserId(target);
}

std::string AbilityManagerService::GetPendingWantBundleName(const sptr<IWantSender> &target)
{
    HILOG_INFO("Get pending want bundle name.");
    CHECK_POINTER_AND_RETURN(pendingWantManager_, "");
    CHECK_POINTER_AND_RETURN(target, "");
    return pendingWantManager_->GetPendingWantBundleName(target);
}

int AbilityManagerService::GetPendingWantCode(const sptr<IWantSender> &target)
{
    HILOG_INFO("%{public}s:begin.", __func__);

    if (pendingWantManager_ == nullptr) {
        HILOG_ERROR("%s, pendingWantManager_ is nullptr", __func__);
        return -1;
    }
    if (target == nullptr) {
        HILOG_ERROR("%s, target is nullptr", __func__);
        return -1;
    }
    return pendingWantManager_->GetPendingWantCode(target);
}

int AbilityManagerService::GetPendingWantType(const sptr<IWantSender> &target)
{
    HILOG_INFO("%{public}s:begin.", __func__);

    if (pendingWantManager_ == nullptr) {
        HILOG_ERROR("%s, pendingWantManager_ is nullptr", __func__);
        return -1;
    }
    if (target == nullptr) {
        HILOG_ERROR("%s, target is nullptr", __func__);
        return -1;
    }
    return pendingWantManager_->GetPendingWantType(target);
}

void AbilityManagerService::RegisterCancelListener(const sptr<IWantSender> &sender, const sptr<IWantReceiver> &receiver)
{
    HILOG_INFO("Register cancel listener.");
    CHECK_POINTER(pendingWantManager_);
    CHECK_POINTER(sender);
    CHECK_POINTER(receiver);
    pendingWantManager_->RegisterCancelListener(sender, receiver);
}

void AbilityManagerService::UnregisterCancelListener(
    const sptr<IWantSender> &sender, const sptr<IWantReceiver> &receiver)
{
    HILOG_INFO("Unregister cancel listener.");
    CHECK_POINTER(pendingWantManager_);
    CHECK_POINTER(sender);
    CHECK_POINTER(receiver);
    pendingWantManager_->UnregisterCancelListener(sender, receiver);
}

int AbilityManagerService::GetPendingRequestWant(const sptr<IWantSender> &target, std::shared_ptr<Want> &want)
{
    HILOG_INFO("Get pending request want.");
    CHECK_POINTER_AND_RETURN(pendingWantManager_, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(target, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(want, ERR_INVALID_VALUE);
    return pendingWantManager_->GetPendingRequestWant(target, want);
}

int AbilityManagerService::SetShowOnLockScreen(bool isAllow)
{
    HILOG_INFO("SetShowOnLockScreen");
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_NO_INIT);
    auto bms = GetBundleManager();
    CHECK_POINTER_AND_RETURN(bms, GET_ABILITY_SERVICE_FAILED);
    int callerUid = IPCSkeleton::GetCallingUid();
    std::string bundleName;
    bool result = bms->GetBundleNameForUid(callerUid, bundleName);
    if (!result) {
        HILOG_ERROR("GetBundleNameForUid fail");
        return GET_BUNDLENAME_BY_UID_FAIL;
    }
    return currentStackManager_->SetShowOnLockScreen(bundleName, isAllow);
}

int AbilityManagerService::LockMissionForCleanup(int32_t missionId)
{
    HILOG_INFO("request unlock mission for clean up all, id :%{public}d", missionId);
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_NO_INIT);

    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not system app");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    return currentMissionListManager_->SetMissionLockedState(missionId, true);
}

int AbilityManagerService::UnlockMissionForCleanup(int32_t missionId)
{
    HILOG_INFO("request unlock mission for clean up all, id :%{public}d", missionId);
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_NO_INIT);

    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not system app");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    return currentMissionListManager_->SetMissionLockedState(missionId, false);
}

int AbilityManagerService::RegisterMissionListener(const sptr<IMissionListener> &listener)
{
    HILOG_INFO("request RegisterMissionListener ");
    CHECK_POINTER_AND_RETURN(currentMissionListManager_, ERR_NO_INIT);
    CHECK_POINTER_AND_RETURN(iBundleManager_, ERR_NO_INIT);

    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not system app");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    return currentMissionListManager_->RegisterMissionListener(listener);
}

int AbilityManagerService::UnRegisterMissionListener(const sptr<IMissionListener> &listener)
{
    HILOG_INFO("request RegisterMissionListener ");
    CHECK_POINTER_AND_RETURN(currentMissionListManager_, ERR_NO_INIT);
    CHECK_POINTER_AND_RETURN(iBundleManager_, ERR_NO_INIT);

    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not system app");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    return currentMissionListManager_->UnRegisterMissionListener(listener);
}

int AbilityManagerService::GetMissionInfos(const std::string& deviceId, int32_t numMax,
    std::vector<MissionInfo> &missionInfos)
{
    HILOG_INFO("request GetMissionInfos.");
    CHECK_POINTER_AND_RETURN(currentMissionListManager_, ERR_NO_INIT);
    CHECK_POINTER_AND_RETURN(iBundleManager_, ERR_NO_INIT);

    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not system app");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    if (CheckIsRemote(deviceId)) {
        return GetRemoteMissionInfos(deviceId, numMax, missionInfos);
    }

    return currentMissionListManager_->GetMissionInfos(numMax, missionInfos);
}

int AbilityManagerService::GetRemoteMissionInfos(const std::string& deviceId, int32_t numMax,
    std::vector<MissionInfo> &missionInfos)
{
    HILOG_INFO("GetRemoteMissionInfos begin");
    DistributedClient dmsClient;
    int result = dmsClient.GetMissionInfos(deviceId, numMax, missionInfos);
    if (result != ERR_OK) {
        HILOG_ERROR("GetRemoteMissionInfos failed, result = %{public}d", result);
        return result;
    }
    return ERR_OK;
}

int AbilityManagerService::GetMissionInfo(const std::string& deviceId, int32_t missionId,
    MissionInfo &missionInfo)
{
    HILOG_INFO("request GetMissionInfo, missionId:%{public}d", missionId);
    CHECK_POINTER_AND_RETURN(currentMissionListManager_, ERR_NO_INIT);
    CHECK_POINTER_AND_RETURN(iBundleManager_, ERR_NO_INIT);

    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not system app");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    if (CheckIsRemote(deviceId)) {
        return GetRemoteMissionInfo(deviceId, missionId, missionInfo);
    }

    return currentMissionListManager_->GetMissionInfo(missionId, missionInfo);
}

int AbilityManagerService::GetRemoteMissionInfo(const std::string& deviceId, int32_t missionId,
    MissionInfo &missionInfo)
{
    HILOG_INFO("GetMissionInfoFromDms begin");
    std::vector<MissionInfo> missionVector;
    int result = GetRemoteMissionInfos(deviceId, MAX_NUMBER_OF_DISTRIBUTED_MISSIONS, missionVector);
    if (result != ERR_OK) {
        return result;
    }
    for (auto iter = missionVector.begin(); iter != missionVector.end(); iter++) {
        if (iter->id == missionId) {
            missionInfo = *iter;
            return ERR_OK;
        }
    }
    HILOG_WARN("missionId not found");
    return ERR_INVALID_VALUE;
}

int AbilityManagerService::CleanMission(int32_t missionId)
{
    HILOG_INFO("request CleanMission, missionId:%{public}d", missionId);
    CHECK_POINTER_AND_RETURN(currentMissionListManager_, ERR_NO_INIT);
    CHECK_POINTER_AND_RETURN(iBundleManager_, ERR_NO_INIT);

    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not system app");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    return currentMissionListManager_->ClearMission(missionId);
}

int AbilityManagerService::CleanAllMissions()
{
    HILOG_INFO("request CleanAllMissions ");
    CHECK_POINTER_AND_RETURN(currentMissionListManager_, ERR_NO_INIT);
    CHECK_POINTER_AND_RETURN(iBundleManager_, ERR_NO_INIT);

    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not system app");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    return currentMissionListManager_->ClearAllMissions();
}

int AbilityManagerService::MoveMissionToFront(int32_t missionId)
{
    HILOG_INFO("request MoveMissionToFront, missionId:%{public}d", missionId);
    CHECK_POINTER_AND_RETURN(currentMissionListManager_, ERR_NO_INIT);
    CHECK_POINTER_AND_RETURN(iBundleManager_, ERR_NO_INIT);

    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not system app");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    return currentMissionListManager_->MoveMissionToFront(missionId);
}

std::shared_ptr<AbilityRecord> AbilityManagerService::GetServiceRecordByElementName(const std::string &element)
{
    return connectManager_->GetServiceRecordByElementName(element);
}

std::list<std::shared_ptr<ConnectionRecord>> AbilityManagerService::GetConnectRecordListByCallback(
    sptr<IAbilityConnection> callback)
{
    return connectManager_->GetConnectRecordListByCallback(callback);
}

sptr<IAbilityScheduler> AbilityManagerService::AcquireDataAbility(
    const Uri &uri, bool tryBind, const sptr<IRemoteObject> &callerToken)
{
    HILOG_INFO("%{public}s, called. uid %{public}d", __func__, IPCSkeleton::GetCallingUid());
    bool isSystem = (IPCSkeleton::GetCallingUid() <= AppExecFwk::Constants::BASE_SYS_UID);
    if (!isSystem) {
        HILOG_INFO("callerToken not system %{public}s", __func__);
        if (!VerificationToken(callerToken)) {
            HILOG_INFO("VerificationToken fail");
            return nullptr;
        }
    }

    auto bms = GetBundleManager();
    CHECK_POINTER_AND_RETURN(bms, nullptr);

    auto localUri(uri);
    if (localUri.GetScheme() != AbilityConfig::SCHEME_DATA_ABILITY) {
        HILOG_ERROR("Acquire data ability with invalid uri scheme.");
        return nullptr;
    }
    std::vector<std::string> pathSegments;
    localUri.GetPathSegments(pathSegments);
    if (pathSegments.empty()) {
        HILOG_ERROR("Acquire data ability with invalid uri path.");
        return nullptr;
    }

    auto userId = GetUserId();
    AbilityRequest abilityRequest;
    std::string dataAbilityUri = localUri.ToString();
    HILOG_INFO("%{public}s, called. userId %{public}d", __func__, userId);
    bool queryResult = iBundleManager_->QueryAbilityInfoByUri(dataAbilityUri, userId, abilityRequest.abilityInfo);
    if (!queryResult || abilityRequest.abilityInfo.name.empty() || abilityRequest.abilityInfo.bundleName.empty()) {
        HILOG_ERROR("Invalid ability info for data ability acquiring.");
        return nullptr;
    }

    if (!CheckDataAbilityRequest(abilityRequest)) {
        HILOG_ERROR("Invalid ability request info for data ability acquiring.");
        return nullptr;
    }

    HILOG_DEBUG("Query data ability info: %{public}s|%{public}s|%{public}s",
        abilityRequest.appInfo.name.c_str(),
        abilityRequest.appInfo.bundleName.c_str(),
        abilityRequest.abilityInfo.name.c_str());

    CHECK_POINTER_AND_RETURN(dataAbilityManager_, nullptr);
    return dataAbilityManager_->Acquire(abilityRequest, tryBind, callerToken, isSystem);
}

bool AbilityManagerService::CheckDataAbilityRequest(AbilityRequest &abilityRequest)
{
    int result = AbilityUtil::JudgeAbilityVisibleControl(abilityRequest.abilityInfo);
    if (result != ERR_OK) {
        HILOG_ERROR("%{public}s JudgeAbilityVisibleControl error.", __func__);
        return false;
    }
    abilityRequest.appInfo = abilityRequest.abilityInfo.applicationInfo;
    if (abilityRequest.appInfo.name.empty() || abilityRequest.appInfo.bundleName.empty()) {
        HILOG_ERROR("Invalid app info for data ability acquiring.");
        return false;
    }
    if (abilityRequest.abilityInfo.type != AppExecFwk::AbilityType::DATA) {
        HILOG_ERROR("BMS query result is not a data ability.");
        return false;
    }
    abilityRequest.uid = abilityRequest.appInfo.uid;
    return true;
}

int AbilityManagerService::ReleaseDataAbility(
    sptr<IAbilityScheduler> dataAbilityScheduler, const sptr<IRemoteObject> &callerToken)
{
    HILOG_INFO("%{public}s, called.", __func__);
    bool isSystem = (IPCSkeleton::GetCallingUid() <= AppExecFwk::Constants::BASE_SYS_UID);
    if (!isSystem) {
        HILOG_INFO("callerToken not system %{public}s", __func__);
        if (!VerificationToken(callerToken)) {
            HILOG_INFO("VerificationToken fail");
            return ERR_INVALID_STATE;
        }
    }

    return dataAbilityManager_->Release(dataAbilityScheduler, callerToken, isSystem);
}

int AbilityManagerService::AttachAbilityThread(
    const sptr<IAbilityScheduler> &scheduler, const sptr<IRemoteObject> &token)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("Attach ability thread.");
    CHECK_POINTER_AND_RETURN(scheduler, ERR_INVALID_VALUE);

    if (!VerificationToken(token)) {
        return ERR_INVALID_VALUE;
    }

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);

    auto abilityInfo = abilityRecord->GetAbilityInfo();
    auto type = abilityInfo.type;

    int returnCode = -1;
    if (type == AppExecFwk::AbilityType::SERVICE || type == AppExecFwk::AbilityType::EXTENSION) {
        returnCode = connectManager_->AttachAbilityThreadLocked(scheduler, token);
    } else if (type == AppExecFwk::AbilityType::DATA) {
        returnCode = dataAbilityManager_->AttachAbilityThread(scheduler, token);
    } else if (IsSystemUiApp(abilityInfo)) {
        if (useNewMission_) {
            returnCode = kernalAbilityManager_->AttachAbilityThread(scheduler, token);
        } else {
            returnCode = systemAppManager_->AttachAbilityThread(scheduler, token);
        }
    } else {
        if (useNewMission_) {
            returnCode = currentMissionListManager_->AttachAbilityThread(scheduler, token);
        } else {
            returnCode = currentStackManager_->AttachAbilityThread(scheduler, token);
        }
    }

    return returnCode;
}

void AbilityManagerService::DumpFuncInit()
{
    dumpFuncMap_[KEY_DUMP_ALL] = &AbilityManagerService::DumpInner;
    dumpFuncMap_[KEY_DUMP_STACK_LIST] = &AbilityManagerService::DumpStackListInner;
    dumpFuncMap_[KEY_DUMP_STACK] = &AbilityManagerService::DumpStackInner;
    dumpFuncMap_[KEY_DUMP_MISSION] = &AbilityManagerService::DumpMissionInner;
    dumpFuncMap_[KEY_DUMP_TOP_ABILITY] = &AbilityManagerService::DumpTopAbilityInner;
    dumpFuncMap_[KEY_DUMP_WAIT_QUEUE] = &AbilityManagerService::DumpWaittingAbilityQueueInner;
    dumpFuncMap_[KEY_DUMP_SERVICE] = &AbilityManagerService::DumpStateInner;
    dumpFuncMap_[KEY_DUMP_DATA] = &AbilityManagerService::DataDumpStateInner;
    dumpFuncMap_[KEY_DUMP_SYSTEM_UI] = &AbilityManagerService::SystemDumpStateInner;
    dumpFuncMap_[KEY_DUMP_FOCUS_ABILITY] = &AbilityManagerService::DumpFocusMapInner;
    dumpFuncMap_[KEY_DUMP_WINDOW_MODE] = &AbilityManagerService::DumpWindowModeInner;
    dumpFuncMap_[KEY_DUMP_MISSION_LIST] = &AbilityManagerService::DumpMissionListInner;
    dumpFuncMap_[KEY_DUMP_MISSION_INFOS] = &AbilityManagerService::DumpMissionInfosInner;
}

void AbilityManagerService::DumpInner(const std::string &args, std::vector<std::string> &info)
{
    if (useNewMission_) {
        if (currentMissionListManager_) {
            currentMissionListManager_->Dump(info);
        }
    } else {
        if (currentStackManager_) {
            currentStackManager_->Dump(info);
        }
    }
}

void AbilityManagerService::DumpStackListInner(const std::string &args, std::vector<std::string> &info)
{
    currentStackManager_->DumpStackList(info);
}

void AbilityManagerService::DumpFocusMapInner(const std::string &args, std::vector<std::string> &info)
{
    currentStackManager_->DumpFocusMap(info);
}

void AbilityManagerService::DumpWindowModeInner(const std::string &args, std::vector<std::string> &info)
{
    currentStackManager_->DumpWindowMode(info);
}

void AbilityManagerService::DumpMissionListInner(const std::string &args, std::vector<std::string> &info)
{
    if (currentMissionListManager_) {
        currentMissionListManager_->DumpMissionList(info);
    }
}

void AbilityManagerService::DumpMissionInfosInner(const std::string &args, std::vector<std::string> &info)
{
    if (currentMissionListManager_) {
        currentMissionListManager_->DumpMissionInfos(info);
    }
}

void AbilityManagerService::DumpStackInner(const std::string &args, std::vector<std::string> &info)
{
    std::vector<std::string> argList;
    SplitStr(args, " ", argList);
    if (argList.empty()) {
        return;
    }
    if (argList.size() < MIN_DUMP_ARGUMENT_NUM) {
        info.push_back("error: invalid argument, please see 'ability dump -h'.");
        return;
    }
    int stackId = DEFAULT_INVAL_VALUE;
    (void)StrToInt(argList[1], stackId);
    currentStackManager_->DumpStack(stackId, info);
}

void AbilityManagerService::DumpMissionInner(const std::string &args, std::vector<std::string> &info)
{
    std::vector<std::string> argList;
    SplitStr(args, " ", argList);
    if (argList.empty()) {
        return;
    }
    if (argList.size() < MIN_DUMP_ARGUMENT_NUM) {
        info.push_back("error: invalid argument, please see 'ability dump -h'.");
        return;
    }
    int missionId = DEFAULT_INVAL_VALUE;
    (void)StrToInt(argList[1], missionId);
    if (useNewMission_) {
        currentMissionListManager_->DumpMission(missionId, info);
    } else {
        currentStackManager_->DumpMission(missionId, info);
    }
}

void AbilityManagerService::DumpTopAbilityInner(const std::string &args, std::vector<std::string> &info)
{
    currentStackManager_->DumpTopAbility(info);
}

void AbilityManagerService::DumpWaittingAbilityQueueInner(const std::string &args, std::vector<std::string> &info)
{
    std::string result;
    DumpWaittingAbilityQueue(result);
    info.push_back(result);
}

void AbilityManagerService::DumpStateInner(const std::string &args, std::vector<std::string> &info)
{
    std::vector<std::string> argList;
    SplitStr(args, " ", argList);
    if (argList.empty()) {
        return;
    }
    if (argList.size() == MIN_DUMP_ARGUMENT_NUM) {
        connectManager_->DumpState(info, argList[1]);
    } else if (argList.size() < MIN_DUMP_ARGUMENT_NUM) {
        connectManager_->DumpState(info);
    } else {
        info.emplace_back("error: invalid argument, please see 'ability dump -h'.");
    }
}

bool AbilityManagerService::IsExistFile(const std::string &path)
{
    HILOG_INFO("%{public}s", __func__);
    if (path.empty()) {
        return false;
    }
    struct stat buf = {};
    if (stat(path.c_str(), &buf) != 0) {
        return false;
    }
    HILOG_INFO("%{public}s  :file exists", __func__);
    return S_ISREG(buf.st_mode);
}

void AbilityManagerService::DataDumpStateInner(const std::string &args, std::vector<std::string> &info)
{
    std::vector<std::string> argList;
    SplitStr(args, " ", argList);
    if (argList.empty()) {
        return;
    }
    if (argList.size() == MIN_DUMP_ARGUMENT_NUM) {
        dataAbilityManager_->DumpState(info, argList[1]);
    } else if (argList.size() < MIN_DUMP_ARGUMENT_NUM) {
        dataAbilityManager_->DumpState(info);
    } else {
        info.emplace_back("error: invalid argument, please see 'ability dump -h'.");
    }
}

void AbilityManagerService::SystemDumpStateInner(const std::string &args, std::vector<std::string> &info)
{
    systemAppManager_->DumpState(info);
}

void AbilityManagerService::DumpState(const std::string &args, std::vector<std::string> &info)
{
    std::vector<std::string> argList;
    SplitStr(args, " ", argList);
    if (argList.empty()) {
        return;
    }
    auto it = dumpMap.find(argList[0]);
    if (it == dumpMap.end()) {
        return;
    }
    DumpKey key = it->second;
    auto itFunc = dumpFuncMap_.find(key);
    if (itFunc != dumpFuncMap_.end()) {
        auto dumpFunc = itFunc->second;
        if (dumpFunc != nullptr) {
            (this->*dumpFunc)(args, info);
            return;
        }
    }
    info.push_back("error: invalid argument, please see 'ability dump -h'.");
}

int AbilityManagerService::AbilityTransitionDone(const sptr<IRemoteObject> &token, int state, const PacMap &saveData)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("Ability transition done, state:%{public}d", state);
    if (!VerificationToken(token) && !VerificationAllToken(token)) {
        return ERR_INVALID_VALUE;
    }
    int32_t userId = IPCSkeleton::GetCallingUid() / BASE_USER_RANGE; // get caller user id.

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER_AND_RETURN_LOG(abilityRecord, ERR_INVALID_VALUE, "Ability record is nullptr.");

    auto abilityInfo = abilityRecord->GetAbilityInfo();
    HILOG_DEBUG("state:%{public}d  name:%{public}s", state, abilityInfo.name.c_str());
    auto type = abilityInfo.type;

    if (type == AppExecFwk::AbilityType::SERVICE || type == AppExecFwk::AbilityType::EXTENSION) {
        auto manager = GetConnectManagerByUserId(userId);
        CHECK_POINTER_AND_RETURN_LOG(manager, ERR_INVALID_VALUE, "no connectManager.");
        return manager->AbilityTransitionDone(token, state);
    }
    if (type == AppExecFwk::AbilityType::DATA) {
        auto manager = GetDataAbilityManagerByUserId(userId);
        CHECK_POINTER_AND_RETURN_LOG(manager, ERR_INVALID_VALUE, "no data ability Manager.");
        return manager->AbilityTransitionDone(token, state);
    }
    if (useNewMission_) {
        if (IsSystemUiApp(abilityInfo)) {
            return kernalAbilityManager_->AbilityTransitionDone(token, state);
        }

        auto manager = GetListManagerByUserId(userId);
        CHECK_POINTER_AND_RETURN_LOG(manager, ERR_INVALID_VALUE, "no mission list manager.");
        return manager->AbilityTransactionDone(token, state, saveData);
    } else {
        if (IsSystemUiApp(abilityInfo)) {
            return systemAppManager_->AbilityTransitionDone(token, state);
        }

        auto manager = GetStackManagerByUserId(userId);
        CHECK_POINTER_AND_RETURN_LOG(manager, ERR_INVALID_VALUE, "no ability stack manager.");
        return manager->AbilityTransitionDone(token, state, saveData);
    }
}

int AbilityManagerService::ScheduleConnectAbilityDone(
    const sptr<IRemoteObject> &token, const sptr<IRemoteObject> &remoteObject)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("Schedule connect ability done.");
    if (!VerificationToken(token)) {
        return ERR_INVALID_VALUE;
    }

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);

    auto type = abilityRecord->GetAbilityInfo().type;
    if (type != AppExecFwk::AbilityType::SERVICE && type != AppExecFwk::AbilityType::EXTENSION) {
        HILOG_ERROR("Connect ability failed, target ability is not service.");
        return TARGET_ABILITY_NOT_SERVICE;
    }

    return connectManager_->ScheduleConnectAbilityDoneLocked(token, remoteObject);
}

int AbilityManagerService::ScheduleDisconnectAbilityDone(const sptr<IRemoteObject> &token)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("Schedule disconnect ability done.");
    if (!VerificationToken(token)) {
        return ERR_INVALID_VALUE;
    }

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);

    auto type = abilityRecord->GetAbilityInfo().type;
    if (type != AppExecFwk::AbilityType::SERVICE && type != AppExecFwk::AbilityType::EXTENSION) {
        HILOG_ERROR("Connect ability failed, target ability is not service.");
        return TARGET_ABILITY_NOT_SERVICE;
    }

    return connectManager_->ScheduleDisconnectAbilityDoneLocked(token);
}

int AbilityManagerService::ScheduleCommandAbilityDone(const sptr<IRemoteObject> &token)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("Schedule command ability done.");
    if (!VerificationToken(token)) {
        return ERR_INVALID_VALUE;
    }

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);

    auto type = abilityRecord->GetAbilityInfo().type;
    if (type != AppExecFwk::AbilityType::SERVICE && type != AppExecFwk::AbilityType::EXTENSION) {
        HILOG_ERROR("Connect ability failed, target ability is not service.");
        return TARGET_ABILITY_NOT_SERVICE;
    }

    return connectManager_->ScheduleCommandAbilityDoneLocked(token);
}

void AbilityManagerService::AddWindowInfo(const sptr<IRemoteObject> &token, int32_t windowToken)
{
    HILOG_DEBUG("Add window id.");
    if (!VerificationToken(token)) {
        return;
    }
    currentStackManager_->AddWindowInfo(token, windowToken);
}

void AbilityManagerService::OnAbilityRequestDone(const sptr<IRemoteObject> &token, const int32_t state)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_INFO("On ability request done.");

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER(abilityRecord);

    auto type = abilityRecord->GetAbilityInfo().type;
    switch (type) {
        case AppExecFwk::AbilityType::SERVICE:
        case AppExecFwk::AbilityType::EXTENSION: {
            auto manager = GetConnectManagerByToken(token);
            if (manager) {
                manager->OnAbilityRequestDone(token, state);
            }
            break;
        }
        case AppExecFwk::AbilityType::DATA: {
            auto manager = GetDataAbilityManagerByToken(token);
            if (manager) {
                manager->OnAbilityRequestDone(token, state);
            }
            break;
        }
        default: {
            if (useNewMission_) {
                if (IsSystemUiApp(abilityRecord->GetAbilityInfo())) {
                    kernalAbilityManager_->OnAbilityRequestDone(token, state);
                    break;
                }

                auto manager = GetListManagerByToken(token);
                if (manager) {
                    manager->OnAbilityRequestDone(token, state);
                }
            } else {
                if (IsSystemUiApp(abilityRecord->GetAbilityInfo())) {
                    systemAppManager_->OnAbilityRequestDone(token, state);
                    break;
                }

                auto manager = GetStackManagerByToken(token);
                if (manager) {
                    manager->OnAbilityRequestDone(token, state);
                }
            }
            break;
        }
    }
}

void AbilityManagerService::OnAppStateChanged(const AppInfo &info)
{
    HILOG_INFO("On app state changed.");
    currentStackManager_->OnAppStateChanged(info);
    connectManager_->OnAppStateChanged(info);
    if (useNewMission_) {
        kernalAbilityManager_->OnAppStateChanged(info);
    } else {
        systemAppManager_->OnAppStateChanged(info);
    }
    dataAbilityManager_->OnAppStateChanged(info);
}

std::shared_ptr<AbilityEventHandler> AbilityManagerService::GetEventHandler()
{
    return handler_;
}

void AbilityManagerService::SetStackManager(int userId, bool switchUser)
{
    auto iterator = stackManagers_.find(userId);
    if (iterator != stackManagers_.end()) {
        if (switchUser) {
            currentStackManager_ = iterator->second;
        }
    } else {
        auto manager = std::make_shared<AbilityStackManager>(userId);
        manager->Init();
        stackManagers_.emplace(userId, manager);
        if (switchUser) {
            currentStackManager_ = manager;
        }
    }
}

void AbilityManagerService::InitMissionListManager(int userId, bool switchUser)
{
    auto iterator = missionListManagers_.find(userId);
    if (iterator != missionListManagers_.end()) {
        if (switchUser) {
            currentMissionListManager_ = iterator->second;
        }
    } else {
        auto manager = std::make_shared<MissionListManager>(userId);
        manager->Init();
        missionListManagers_.emplace(userId, manager);
        if (switchUser) {
            currentMissionListManager_ = manager;
        }
    }
}

std::shared_ptr<AbilityStackManager> AbilityManagerService::GetStackManager()
{
    return currentStackManager_;
}

void AbilityManagerService::DumpWaittingAbilityQueue(std::string &result)
{
    currentStackManager_->DumpWaittingAbilityQueue(result);
    return;
}

// multi user scene
int AbilityManagerService::GetUserId()
{
    if (userController_) {
        return userController_->GetCurrentUserId();
    }
    return DEFAULT_USER_ID;
}

void AbilityManagerService::StartingLauncherAbility()
{
    HILOG_DEBUG("%{public}s", __func__);
    if (!iBundleManager_) {
        HILOG_INFO("bms service is null");
        return;
    }

    /* query if launcher ability has installed */
    AppExecFwk::AbilityInfo abilityInfo;
    /* First stage, hardcoding for the first launcher App */
    auto userId = GetUserId();
    Want want;
    want.SetElementName(AbilityConfig::LAUNCHER_BUNDLE_NAME, AbilityConfig::LAUNCHER_ABILITY_NAME);
    while (!(iBundleManager_->QueryAbilityInfo(want, AppExecFwk::AbilityInfoFlag::GET_ABILITY_INFO_WITH_APPLICATION,
        userId, abilityInfo))) {
        HILOG_INFO("Waiting query launcher ability info completed.");
        usleep(REPOLL_TIME_MICRO_SECONDS);
    }

    HILOG_INFO("Start Home Launcher Ability.");
    /* start launch ability */
    (void)StartAbility(want, DEFAULT_INVAL_VALUE);
    return;
}

void AbilityManagerService::StartingPhoneServiceAbility()
{
    HILOG_DEBUG("%{public}s", __func__);
    if (!iBundleManager_) {
        HILOG_INFO("bms service is null");
        return;
    }

    AppExecFwk::AbilityInfo phoneServiceInfo;
    Want phoneServiceWant;
    phoneServiceWant.SetElementName(AbilityConfig::PHONE_SERVICE_BUNDLE_NAME,
        AbilityConfig::PHONE_SERVICE_ABILITY_NAME);

    auto userId = GetUserId();
    int attemptNums = 1;
    while (!(iBundleManager_->QueryAbilityInfo(phoneServiceWant,
        OHOS::AppExecFwk::AbilityInfoFlag::GET_ABILITY_INFO_DEFAULT, userId, phoneServiceInfo)) &&
        attemptNums <= MAX_NUMBER_OF_CONNECT_BMS) {
        HILOG_INFO("Waiting query phone service ability info completed.");
        usleep(REPOLL_TIME_MICRO_SECONDS);
        attemptNums++;
    }

    (void)StartAbility(phoneServiceWant, DEFAULT_INVAL_VALUE);
}

void AbilityManagerService::StartingContactsAbility()
{
    HILOG_DEBUG("%{public}s", __func__);
    if (!iBundleManager_) {
        HILOG_INFO("bms service is null");
        return;
    }

    AppExecFwk::AbilityInfo contactsInfo;
    Want contactsWant;
    contactsWant.SetElementName(AbilityConfig::CONTACTS_BUNDLE_NAME, AbilityConfig::CONTACTS_ABILITY_NAME);

    auto userId = GetUserId();
    int attemptNums = 1;
    while (!(iBundleManager_->QueryAbilityInfo(contactsWant,
        OHOS::AppExecFwk::AbilityInfoFlag::GET_ABILITY_INFO_DEFAULT, userId, contactsInfo)) &&
        attemptNums <= MAX_NUMBER_OF_CONNECT_BMS) {
        HILOG_INFO("Waiting query contacts service completed.");
        usleep(REPOLL_TIME_MICRO_SECONDS);
        attemptNums++;
    }

    HILOG_INFO("attemptNums : %{public}d", attemptNums);
    if (attemptNums <= MAX_NUMBER_OF_CONNECT_BMS) {
        (void)StartAbility(contactsWant, DEFAULT_INVAL_VALUE);
    }
}

void AbilityManagerService::StartingMmsAbility()
{
    HILOG_DEBUG("%{public}s", __func__);
    if (!iBundleManager_) {
        HILOG_INFO("bms service is null");
        return;
    }

    AppExecFwk::AbilityInfo mmsInfo;
    Want mmsWant;
    mmsWant.SetElementName(AbilityConfig::MMS_BUNDLE_NAME, AbilityConfig::MMS_ABILITY_NAME);
 
    auto userId = GetUserId();
    int attemptNums = 1;
    while (!(iBundleManager_->QueryAbilityInfo(mmsWant,
        OHOS::AppExecFwk::AbilityInfoFlag::GET_ABILITY_INFO_DEFAULT, userId, mmsInfo)) &&
        attemptNums <= MAX_NUMBER_OF_CONNECT_BMS) {
        HILOG_INFO("Waiting query mms service completed.");
        usleep(REPOLL_TIME_MICRO_SECONDS);
        attemptNums++;
    }

    HILOG_INFO("attemptNums : %{public}d", attemptNums);
    if (attemptNums <= MAX_NUMBER_OF_CONNECT_BMS) {
        (void)StartAbility(mmsWant, DEFAULT_INVAL_VALUE);
    }
}

void AbilityManagerService::StartSystemUi(const std::string abilityName)
{
    HILOG_INFO("Starting system ui app.");
    Want want;
    want.SetElementName(AbilityConfig::SYSTEM_UI_BUNDLE_NAME, abilityName);
    HILOG_INFO("Ability name: %{public}s.", abilityName.c_str());
    (void)StartAbility(want, DEFAULT_INVAL_VALUE);
    return;
}

int AbilityManagerService::GenerateAbilityRequest(
    const Want &want, int requestCode, AbilityRequest &request, const sptr<IRemoteObject> &callerToken)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    request.want = want;
    request.requestCode = requestCode;
    request.callerToken = callerToken;
    request.startSetting = nullptr;

    auto bms = GetBundleManager();
    CHECK_POINTER_AND_RETURN(bms, GET_ABILITY_SERVICE_FAILED);

    int userId = GetUserId();
    if (IsSystemUI(want.GetBundle())) {
        userId = MAIN_USER_ID;
    }
    if (want.GetAction().compare(ACTION_CHOOSE) == 0) {
        return ShowPickerDialog(want, userId);
    }
    bms->QueryAbilityInfo(want, AppExecFwk::AbilityInfoFlag::GET_ABILITY_INFO_WITH_APPLICATION,
        userId, request.abilityInfo);
    if (request.abilityInfo.name.empty() || request.abilityInfo.bundleName.empty()) {
        // try to find extension
        int ret = GetAbilityInfoFromExtension(want, request.abilityInfo);
        if (!ret) {
            HILOG_ERROR("Get ability info failed.");
            return RESOLVE_ABILITY_ERR;
        }
    }
    HILOG_DEBUG("Query ability name: %{public}s,", request.abilityInfo.name.c_str());
    if (request.abilityInfo.type == AppExecFwk::AbilityType::SERVICE) {
        AppExecFwk::BundleInfo bundleInfo;
        bool ret = bms->GetBundleInfo(request.abilityInfo.bundleName,
            AppExecFwk::BundleFlag::GET_BUNDLE_WITH_ABILITIES, bundleInfo, userId);
        if (!ret) {
            HILOG_ERROR("Failed to get bundle info when GenerateAbilityRequest.");
            return RESOLVE_ABILITY_ERR;
        }
        HILOG_INFO("bundleInfo.compatibleVersion:%{public}d", bundleInfo.compatibleVersion);
        if (bundleInfo.compatibleVersion >= EXTENSION_SUPPORT_API_VERSION) {
            HILOG_INFO("abilityInfo reset EXTENSION.");
            request.abilityInfo.type = AppExecFwk::AbilityType::EXTENSION;
        }
    }

    request.appInfo = request.abilityInfo.applicationInfo;
    if (request.appInfo.name.empty() || request.appInfo.bundleName.empty()) {
        HILOG_ERROR("Get app info failed.");
        return RESOLVE_APP_ERR;
    }
    HILOG_DEBUG("Query app name: %{public}s,", request.appInfo.name.c_str());

    AppExecFwk::BundleInfo bundleInfo;
    if (!bms->GetBundleInfo(request.appInfo.bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT,
        bundleInfo, userId)) {
        HILOG_ERROR("Failed to get bundle info when generate ability request.");
        return RESOLVE_APP_ERR;
    }
    request.compatibleVersion = bundleInfo.compatibleVersion;
    request.uid = bundleInfo.uid;

    return ERR_OK;
}

int AbilityManagerService::GetAllStackInfo(StackInfo &stackInfo)
{
    HILOG_DEBUG("Get all stack info.");
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_NO_INIT);
    currentStackManager_->GetAllStackInfo(stackInfo);
    return ERR_OK;
}

int AbilityManagerService::TerminateAbilityResult(const sptr<IRemoteObject> &token, int startId)
{
    HILOG_INFO("Terminate ability result, startId: %{public}d", startId);
    if (!VerificationToken(token)) {
        return ERR_INVALID_VALUE;
    }

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER_AND_RETURN(abilityRecord, ERR_INVALID_VALUE);
    int result = AbilityUtil::JudgeAbilityVisibleControl(abilityRecord->GetAbilityInfo());
    if (result != ERR_OK) {
        HILOG_ERROR("%{public}s JudgeAbilityVisibleControl error.", __func__);
        return result;
    }

    auto type = abilityRecord->GetAbilityInfo().type;
    if (type != AppExecFwk::AbilityType::SERVICE && type != AppExecFwk::AbilityType::EXTENSION) {
        HILOG_ERROR("target ability is not service.");
        return TARGET_ABILITY_NOT_SERVICE;
    }

    return connectManager_->TerminateAbilityResult(token, startId);
}

int AbilityManagerService::StopServiceAbility(const Want &want)
{
    BYTRACE_NAME(BYTRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_DEBUG("Stop service ability.");
    AbilityRequest abilityRequest;
    int result = GenerateAbilityRequest(want, DEFAULT_INVAL_VALUE, abilityRequest, nullptr);
    if (result != ERR_OK) {
        HILOG_ERROR("Generate ability request error.");
        return result;
    }
    auto abilityInfo = abilityRequest.abilityInfo;
    auto type = abilityInfo.type;
    if (type != AppExecFwk::AbilityType::SERVICE && type != AppExecFwk::AbilityType::EXTENSION) {
        HILOG_ERROR("Target ability is not service type.");
        return TARGET_ABILITY_NOT_SERVICE;
    }
    return connectManager_->StopServiceAbility(abilityRequest);
}

void AbilityManagerService::OnAbilityDied(std::shared_ptr<AbilityRecord> abilityRecord)
{
    CHECK_POINTER(abilityRecord);

    if (useNewMission_) {
        if (kernalAbilityManager_ && abilityRecord->IsKernalSystemAbility()) {
            kernalAbilityManager_->OnAbilityDied(abilityRecord);
            return;
        }

        auto manager = GetListManagerByToken(abilityRecord->GetToken());
        if (manager) {
            manager->OnAbilityDied(abilityRecord, GetUserId());
            return;
        }
    } else {
        if (systemAppManager_ && abilityRecord->IsKernalSystemAbility()) {
            systemAppManager_->OnAbilityDied(abilityRecord);
            return;
        }

        auto manager = GetStackManagerByToken(abilityRecord->GetToken());
        if (manager) {
            manager->OnAbilityDied(abilityRecord);
            return;
        }
    }

    auto manager = GetConnectManagerByToken(abilityRecord->GetToken());
    if (manager) {
        manager->OnAbilityDied(abilityRecord);
        return;
    }

    auto dataAbilityManager = GetDataAbilityManagerByToken(abilityRecord->GetToken());
    if (dataAbilityManager) {
        dataAbilityManager->OnAbilityDied(abilityRecord);
    }
}

void AbilityManagerService::GetMaxRestartNum(int &max)
{
    if (amsConfigResolver_) {
        max = amsConfigResolver_->GetMaxRestartNum();
    }
}

int AbilityManagerService::KillProcess(const std::string &bundleName)
{
    HILOG_DEBUG("Kill process, bundleName: %{public}s", bundleName.c_str());
    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not systemApp");
        return CALLER_ISNOT_SYSTEMAPP;
    }

    auto bms = GetBundleManager();
    CHECK_POINTER_AND_RETURN(bms, KILL_PROCESS_FAILED);

    AppExecFwk::BundleInfo bundleInfo;
    if (!bms->GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo)) {
        HILOG_ERROR("Failed to get bundle info when kill process.");
        return GET_BUNDLE_INFO_FAILED;
    }

    if (bundleInfo.isKeepAlive) {
        HILOG_ERROR("Can not kill keep alive process.");
        return KILL_PROCESS_KEEP_ALIVE;
    }

    int ret = DelayedSingleton<AppScheduler>::GetInstance()->KillApplication(bundleName);
    if (ret != ERR_OK) {
        return KILL_PROCESS_FAILED;
    }
    return ERR_OK;
}

int AbilityManagerService::ClearUpApplicationData(const std::string &bundleName)
{
    HILOG_DEBUG("ClearUpApplicationData, bundleName: %{public}s", bundleName.c_str());
    if (!CheckCallerIsSystemAppByIpc()) {
        HILOG_ERROR("caller is not systemApp");
        return CALLER_ISNOT_SYSTEMAPP;
    }
    int ret = DelayedSingleton<AppScheduler>::GetInstance()->ClearUpApplicationData(bundleName);
    if (ret != ERR_OK) {
        return CLEAR_APPLICATION_DATA_FAIL;
    }
    return ERR_OK;
}

int AbilityManagerService::UninstallApp(const std::string &bundleName)
{
    HILOG_DEBUG("Uninstall app, bundleName: %{public}s", bundleName.c_str());
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_NO_INIT);
    currentStackManager_->UninstallApp(bundleName);
    CHECK_POINTER_AND_RETURN(pendingWantManager_, ERR_NO_INIT);
    pendingWantManager_->ClearPendingWantRecord(bundleName);
    int ret = DelayedSingleton<AppScheduler>::GetInstance()->KillApplication(bundleName);
    if (ret != ERR_OK) {
        return UNINSTALL_APP_FAILED;
    }
    return ERR_OK;
}

sptr<AppExecFwk::IBundleMgr> AbilityManagerService::GetBundleManager()
{
    if (iBundleManager_ == nullptr) {
        auto bundleObj =
            OHOS::DelayedSingleton<SaMgrClient>::GetInstance()->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
        if (bundleObj == nullptr) {
            HILOG_ERROR("Failed to get bundle manager service.");
            return nullptr;
        }
        iBundleManager_ = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
    }

    return iBundleManager_;
}

int AbilityManagerService::PreLoadAppDataAbilities(const std::string &bundleName)
{
    if (bundleName.empty()) {
        HILOG_ERROR("Invalid bundle name when app data abilities preloading.");
        return ERR_INVALID_VALUE;
    }

    auto dataAbilityManager = dataAbilityManager_;
    if (dataAbilityManager == nullptr) {
        HILOG_ERROR("Invalid data ability manager when app data abilities preloading.");
        return ERR_INVALID_STATE;
    }

    auto bms = GetBundleManager();
    CHECK_POINTER_AND_RETURN(bms, GET_ABILITY_SERVICE_FAILED);

    AppExecFwk::BundleInfo bundleInfo;
    int32_t userId = GetUserId();
    if (IsSystemUI(bundleName)) {
        userId = MAIN_USER_ID;
    }
    bool ret = bms->GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_WITH_ABILITIES, bundleInfo, userId);
    if (!ret) {
        HILOG_ERROR("Failed to get bundle info when app data abilities preloading.");
        return RESOLVE_APP_ERR;
    }

    HILOG_INFO("App data abilities preloading for bundle '%{public}s'...", bundleName.data());

    auto begin = system_clock::now();
    AbilityRequest dataAbilityRequest;
    dataAbilityRequest.appInfo = bundleInfo.applicationInfo;
    for (auto it = bundleInfo.abilityInfos.begin(); it != bundleInfo.abilityInfos.end(); ++it) {
        if (it->type != AppExecFwk::AbilityType::DATA) {
            continue;
        }
        if ((system_clock::now() - begin) >= DATA_ABILITY_START_TIMEOUT) {
            HILOG_ERROR("App data ability preloading for '%{public}s' timeout.", bundleName.c_str());
            return ERR_TIMED_OUT;
        }
        dataAbilityRequest.abilityInfo = *it;
        dataAbilityRequest.uid = bundleInfo.uid;
        HILOG_INFO("App data ability preloading: '%{public}s.%{public}s'...", it->bundleName.c_str(), it->name.c_str());

        auto dataAbility = dataAbilityManager->Acquire(dataAbilityRequest, false, nullptr, false);
        if (dataAbility == nullptr) {
            HILOG_ERROR(
                "Failed to preload data ability '%{public}s.%{public}s'.", it->bundleName.c_str(), it->name.c_str());
            return ERR_NULL_OBJECT;
        }
    }

    HILOG_INFO("App data abilities preloading done.");

    return ERR_OK;
}

bool AbilityManagerService::IsSystemUiApp(const AppExecFwk::AbilityInfo &info) const
{
    if (info.bundleName != AbilityConfig::SYSTEM_UI_BUNDLE_NAME) {
        return false;
    }
    return (info.name == AbilityConfig::SYSTEM_UI_NAVIGATION_BAR || info.name == AbilityConfig::SYSTEM_UI_STATUS_BAR);
}

bool AbilityManagerService::IsSystemUI(const std::string &bundleName) const
{
    return bundleName == AbilityConfig::SYSTEM_UI_BUNDLE_NAME;
}

void AbilityManagerService::HandleLoadTimeOut(int64_t eventId)
{
    HILOG_DEBUG("Handle load timeout.");
    if (useNewMission_) {
        if (kernalAbilityManager_) {
            kernalAbilityManager_->OnTimeOut(AbilityManagerService::LOAD_TIMEOUT_MSG, eventId);
        }
        for (auto& item : missionListManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::LOAD_TIMEOUT_MSG, eventId);
            }
        }
    } else {
        if (systemAppManager_) {
            systemAppManager_->OnTimeOut(AbilityManagerService::LOAD_TIMEOUT_MSG, eventId);
        }
        for (auto& item : stackManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::LOAD_TIMEOUT_MSG, eventId);
            }
        }
     }
}

void AbilityManagerService::HandleActiveTimeOut(int64_t eventId)
{
    HILOG_DEBUG("Handle active timeout.");

    if (useNewMission_) {
        if (kernalAbilityManager_) {
            kernalAbilityManager_->OnTimeOut(AbilityManagerService::ACTIVE_TIMEOUT_MSG, eventId);
        }
        for (auto& item : missionListManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::ACTIVE_TIMEOUT_MSG, eventId);
            }
        }
    } else {
        if (systemAppManager_) {
            systemAppManager_->OnTimeOut(AbilityManagerService::ACTIVE_TIMEOUT_MSG, eventId);
        }
        for (auto& item : stackManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::ACTIVE_TIMEOUT_MSG, eventId);
            }
        }
    }
}

void AbilityManagerService::HandleInactiveTimeOut(int64_t eventId)
{
    HILOG_DEBUG("Handle inactive timeout.");
    if (useNewMission_) {
        for (auto& item : missionListManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::INACTIVE_TIMEOUT_MSG, eventId);
            }
        }
    } else {
        for (auto& item : stackManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::INACTIVE_TIMEOUT_MSG, eventId);
            }
        }
    }
}

void AbilityManagerService::HandleForegroundNewTimeOut(int64_t eventId)
{
    HILOG_DEBUG("Handle ForegroundNew timeout.");
    if (useNewMission_) {
        if (kernalAbilityManager_) {
            kernalAbilityManager_->OnTimeOut(AbilityManagerService::FOREGROUNDNEW_TIMEOUT_MSG, eventId);
        }
        for (auto& item : missionListManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::FOREGROUNDNEW_TIMEOUT_MSG, eventId);
            }
        }
    } else {
        if (systemAppManager_) {
            systemAppManager_->OnTimeOut(AbilityManagerService::FOREGROUNDNEW_TIMEOUT_MSG, eventId);
        }
        for (auto& item : stackManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::FOREGROUNDNEW_TIMEOUT_MSG, eventId);
            }
        }
    }
}

void AbilityManagerService::HandleBackgroundNewTimeOut(int64_t eventId)
{
    HILOG_DEBUG("Handle BackgroundNew timeout.");
    if (useNewMission_) {
        if (kernalAbilityManager_) {
            kernalAbilityManager_->OnTimeOut(AbilityManagerService::BACKGROUNDNEW_TIMEOUT_MSG, eventId);
        }
        for (auto& item : missionListManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::BACKGROUNDNEW_TIMEOUT_MSG, eventId);
            }
        }
    } else {
        if (systemAppManager_) {
            systemAppManager_->OnTimeOut(AbilityManagerService::BACKGROUNDNEW_TIMEOUT_MSG, eventId);
        }
        for (auto& item : stackManagers_) {
            if (item.second) {
                item.second->OnTimeOut(AbilityManagerService::BACKGROUNDNEW_TIMEOUT_MSG, eventId);
            }
        }
    }
}

bool AbilityManagerService::VerificationToken(const sptr<IRemoteObject> &token)
{
    HILOG_INFO("Verification token.");
    CHECK_POINTER_RETURN_BOOL(dataAbilityManager_);
    CHECK_POINTER_RETURN_BOOL(connectManager_);
    CHECK_POINTER_RETURN_BOOL(currentStackManager_);
    CHECK_POINTER_RETURN_BOOL(systemAppManager_);
    CHECK_POINTER_RETURN_BOOL(currentMissionListManager_);
    CHECK_POINTER_RETURN_BOOL(kernalAbilityManager_);

    if (useNewMission_) {
        if (currentMissionListManager_->GetAbilityRecordByToken(token)) {
            return true;
        }
        if (currentMissionListManager_->GetAbilityFromTerminateList(token)) {
            return true;
        }
    } else {
        if (currentStackManager_->GetAbilityRecordByToken(token)) {
            return true;
        }

        if (currentStackManager_->GetAbilityFromTerminateList(token)) {
            return true;
        }
    }

    if (dataAbilityManager_->GetAbilityRecordByToken(token)) {
        return true;
    }

    if (connectManager_->GetServiceRecordByToken(token)) {
        return true;
    }

    if (useNewMission_) {
        if (kernalAbilityManager_->GetAbilityRecordByToken(token)) {
            return true;
        }
    } else {
        if (systemAppManager_->GetAbilityRecordByToken(token)) {
            return true;
        }
    }

    HILOG_ERROR("Failed to verify token.");
    return false;
}

bool AbilityManagerService::VerificationAllToken(const sptr<IRemoteObject> &token)
{
    HILOG_INFO("VerificationAllToken.");

    if (useNewMission_) {
        for (auto item: missionListManagers_) {
            if (item.second && item.second->GetAbilityRecordByToken(token)) {
                return true;
            }

            if (item.second && item.second->GetAbilityFromTerminateList(token)) {
                return true;
            }
        }
    } else {
        for (auto item: stackManagers_) {
            if (item.second && item.second->GetAbilityRecordByToken(token)) {
                return true;
            }

            if (item.second && item.second->GetAbilityFromTerminateList(token)) {
                return true;
            }
        }
    }

    for (auto item: dataAbilityManagers_) {
        if (item.second && item.second->GetAbilityRecordByToken(token)) {
            return true;
        }
    }

    for (auto item: connectManagers_) {
        if (item.second && item.second->GetServiceRecordByToken(token)) {
            return true;
        }
    }

    if (useNewMission_) {
        if (kernalAbilityManager_->GetAbilityRecordByToken(token)) {
            return true;
        }
    } else {
        if (systemAppManager_->GetAbilityRecordByToken(token)) {
            return true;
        }
    }

    HILOG_ERROR("Failed to verify all token.");
    return false;
}

std::shared_ptr<AbilityStackManager> AbilityManagerService::GetStackManagerByUserId(int32_t userId)
{
    auto it = stackManagers_.find(userId);
    if (it != stackManagers_.end()) {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<MissionListManager> AbilityManagerService::GetListManagerByUserId(int32_t userId)
{
    auto it = missionListManagers_.find(userId);
    if (it != missionListManagers_.end()) {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<AbilityConnectManager> AbilityManagerService::GetConnectManagerByUserId(int32_t userId)
{
    auto it = connectManagers_.find(userId);
    if (it != connectManagers_.end()) {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<DataAbilityManager> AbilityManagerService::GetDataAbilityManagerByUserId(int32_t userId)
{
    auto it = dataAbilityManagers_.find(userId);
    if (it != dataAbilityManagers_.end()) {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<AbilityStackManager> AbilityManagerService::GetStackManagerByToken(const sptr<IRemoteObject> &token)
{
    for (auto item: stackManagers_) {
        if (item.second && item.second->GetAbilityRecordByToken(token)) {
            return item.second;
        }

        if (item.second && item.second->GetAbilityFromTerminateList(token)) {
            return item.second;
        }
    }

    return nullptr;
}

std::shared_ptr<MissionListManager> AbilityManagerService::GetListManagerByToken(const sptr<IRemoteObject> &token)
{
    for (auto item: missionListManagers_) {
        if (item.second && item.second->GetAbilityRecordByToken(token)) {
            return item.second;
        }

        if (item.second && item.second->GetAbilityFromTerminateList(token)) {
            return item.second;
        }
    }

    return nullptr;
}

std::shared_ptr<AbilityConnectManager> AbilityManagerService::GetConnectManagerByToken(
    const sptr<IRemoteObject> &token)
{
    for (auto item: connectManagers_) {
        if (item.second && item.second->GetServiceRecordByToken(token)) {
            return item.second;
        }
    }

    return nullptr;
}

std::shared_ptr<DataAbilityManager> AbilityManagerService::GetDataAbilityManagerByToken(
    const sptr<IRemoteObject> &token)
{
    for (auto item: dataAbilityManagers_) {
        if (item.second && item.second->GetAbilityRecordByToken(token)) {
            return item.second;
        }
    }

    return nullptr;
}


int AbilityManagerService::MoveMissionToFloatingStack(const MissionOption &missionOption)
{
    HILOG_INFO("Move mission to floating stack.");
    return currentStackManager_->MoveMissionToFloatingStack(missionOption);
}

int AbilityManagerService::MoveMissionToSplitScreenStack(const MissionOption &primary, const MissionOption &secondary)
{
    HILOG_INFO("Move mission to split screen stack.");
    return currentStackManager_->MoveMissionToSplitScreenStack(primary, secondary);
}

int AbilityManagerService::ChangeFocusAbility(
    const sptr<IRemoteObject> &lostFocusToken, const sptr<IRemoteObject> &getFocusToken)
{
    HILOG_INFO("Change focus ability.");
    CHECK_POINTER_AND_RETURN(lostFocusToken, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(getFocusToken, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_INVALID_VALUE);

    if (!VerificationToken(lostFocusToken)) {
        return ERR_INVALID_VALUE;
    }

    if (!VerificationToken(getFocusToken)) {
        return ERR_INVALID_VALUE;
    }

    return currentStackManager_->ChangeFocusAbility(lostFocusToken, getFocusToken);
}

int AbilityManagerService::MinimizeMultiWindow(int missionId)
{
    HILOG_INFO("Minimize multi window.");
    return currentStackManager_->MinimizeMultiWindow(missionId);
}

int AbilityManagerService::MaximizeMultiWindow(int missionId)
{
    HILOG_INFO("Maximize multi window.");
    return currentStackManager_->MaximizeMultiWindow(missionId);
}

int AbilityManagerService::GetFloatingMissions(std::vector<AbilityMissionInfo> &list)
{
    HILOG_INFO("Get floating missions.");
    return currentStackManager_->GetFloatingMissions(list);
}

int AbilityManagerService::CloseMultiWindow(int missionId)
{
    HILOG_INFO("Close multi window.");
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_INVALID_VALUE);
    return currentStackManager_->CloseMultiWindow(missionId);
}

int AbilityManagerService::SetMissionStackSetting(const StackSetting &stackSetting)
{
    HILOG_INFO("Set mission stack setting.");
    currentStackManager_->SetMissionStackSetting(stackSetting);
    return ERR_OK;
}

bool AbilityManagerService::IsFirstInMission(const sptr<IRemoteObject> &token)
{
    HILOG_INFO("Is first in mission.");
    CHECK_POINTER_RETURN_BOOL(token);
    CHECK_POINTER_RETURN_BOOL(currentStackManager_);

    if (!VerificationToken(token)) {
        return false;
    }

    return currentStackManager_->IsFirstInMission(token);
}

int AbilityManagerService::CompelVerifyPermission(const std::string &permission, int pid, int uid, std::string &message)
{
    HILOG_INFO("Compel verify permission.");
    return DelayedSingleton<AppScheduler>::GetInstance()->CompelVerifyPermission(permission, pid, uid, message);
}

int AbilityManagerService::PowerOff()
{
    HILOG_INFO("Power off.");
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_NO_INIT);
    return currentStackManager_->PowerOff();
}

int AbilityManagerService::PowerOn()
{
    HILOG_INFO("Power on.");
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_NO_INIT);
    return currentStackManager_->PowerOn();
}

int AbilityManagerService::LockMission(int missionId)
{
    HILOG_INFO("request lock mission id :%{public}d", missionId);
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_NO_INIT);
    CHECK_POINTER_AND_RETURN(iBundleManager_, ERR_NO_INIT);

    int callerUid = IPCSkeleton::GetCallingUid();
    int callerPid = IPCSkeleton::GetCallingPid();
    bool isSystemApp = iBundleManager_->CheckIsSystemAppByUid(callerUid);
    HILOG_DEBUG("locker uid :%{public}d, pid :%{public}d. isSystemApp: %{public}d", callerUid, callerPid, isSystemApp);
    return currentStackManager_->StartLockMission(callerUid, missionId, isSystemApp, true);
}

int AbilityManagerService::UnlockMission(int missionId)
{
    HILOG_INFO("request unlock mission id :%{public}d", missionId);
    CHECK_POINTER_AND_RETURN(currentStackManager_, ERR_NO_INIT);
    CHECK_POINTER_AND_RETURN(iBundleManager_, ERR_NO_INIT);

    int callerUid = IPCSkeleton::GetCallingUid();
    int callerPid = IPCSkeleton::GetCallingPid();
    bool isSystemApp = iBundleManager_->CheckIsSystemAppByUid(callerUid);
    HILOG_DEBUG("locker uid :%{public}d, pid :%{public}d. isSystemApp: %{public}d", callerUid, callerPid, isSystemApp);
    return currentStackManager_->StartLockMission(callerUid, missionId, isSystemApp, false);
}

int AbilityManagerService::GetUidByBundleName(std::string bundleName)
{
    CHECK_POINTER_AND_RETURN(iBundleManager_, -1);
    return iBundleManager_->GetUidByBundleName(bundleName, GetUserId());
}

void AbilityManagerService::RestartAbility(const sptr<IRemoteObject> &token)
{
    HILOG_INFO("%{public}s called", __func__);
    CHECK_POINTER(currentStackManager_);
    CHECK_POINTER(kernalAbilityManager_);
    CHECK_POINTER(systemAppManager_);
    if (!VerificationToken(token)) {
        return;
    }

    auto abilityRecord = Token::GetAbilityRecordByToken(token);
    CHECK_POINTER(abilityRecord);

    if (IsSystemUiApp(abilityRecord->GetAbilityInfo())) {
        if (useNewMission_) {
            kernalAbilityManager_->RestartAbility(abilityRecord);
        } else {
            systemAppManager_->RestartAbility(abilityRecord);
        }
        return;
    }

    currentStackManager_->RestartAbility(abilityRecord);
}

void AbilityManagerService::NotifyBmsAbilityLifeStatus(
    const std::string &bundleName, const std::string &abilityName, const int64_t launchTime)
{
    auto bundleManager = GetBundleManager();
    CHECK_POINTER(bundleManager);
    bundleManager->NotifyAbilityLifeStatus(bundleName, abilityName, launchTime);
}

void AbilityManagerService::StartSystemApplication()
{
    HILOG_DEBUG("%{public}s", __func__);

    ConnectBmsService();

    if (!amsConfigResolver_ || amsConfigResolver_->NonConfigFile()) {
        HILOG_INFO("start all");
        StartingSystemUiAbility(SatrtUiMode::STARTUIBOTH);
        return;
    }

    if (amsConfigResolver_->GetStatusBarState()) {
        HILOG_INFO("start status bar");
        StartingSystemUiAbility(SatrtUiMode::STATUSBAR);
    }

    if (amsConfigResolver_->GetNavigationBarState()) {
        HILOG_INFO("start navigation bar");
        StartingSystemUiAbility(SatrtUiMode::NAVIGATIONBAR);
    }

    // Location may change
    DelayedSingleton<AppScheduler>::GetInstance()->StartupResidentProcess();
}

void AbilityManagerService::ConnectBmsService()
{
    HILOG_DEBUG("%{public}s", __func__);
    HILOG_INFO("Waiting AppMgr Service run completed.");
    while (!appScheduler_->Init(shared_from_this())) {
        HILOG_ERROR("failed to init appScheduler_");
        usleep(REPOLL_TIME_MICRO_SECONDS);
    }

    HILOG_INFO("Waiting BundleMgr Service run completed.");
    /* wait until connected to bundle manager service */
    while (iBundleManager_ == nullptr) {
        sptr<IRemoteObject> bundle_obj =
            OHOS::DelayedSingleton<SaMgrClient>::GetInstance()->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
        if (bundle_obj == nullptr) {
            HILOG_ERROR("failed to get bundle manager service");
            usleep(REPOLL_TIME_MICRO_SECONDS);
            continue;
        }
        iBundleManager_ = iface_cast<AppExecFwk::IBundleMgr>(bundle_obj);
    }

    HILOG_INFO("Connect bms success!");
}

void AbilityManagerService::StartingSystemUiAbility(const SatrtUiMode &mode)
{
    HILOG_DEBUG("%{public}s", __func__);
    if (!iBundleManager_) {
        HILOG_INFO("bms service is null");
        return;
    }

    AppExecFwk::AbilityInfo statusBarInfo;
    AppExecFwk::AbilityInfo navigationBarInfo;
    Want statusBarWant;
    Want navigationBarWant;
    statusBarWant.SetElementName(AbilityConfig::SYSTEM_UI_BUNDLE_NAME, AbilityConfig::SYSTEM_UI_STATUS_BAR);
    navigationBarWant.SetElementName(AbilityConfig::SYSTEM_UI_BUNDLE_NAME, AbilityConfig::SYSTEM_UI_NAVIGATION_BAR);
    uint32_t waitCnt = 0;
    // Wait 10 minutes for the installation to complete.
    auto userId = MAIN_USER_ID;
    while ((!(iBundleManager_->QueryAbilityInfo(statusBarWant,
            OHOS::AppExecFwk::AbilityInfoFlag::GET_ABILITY_INFO_DEFAULT, userId, statusBarInfo)) ||
        !(iBundleManager_->QueryAbilityInfo(navigationBarWant,
            OHOS::AppExecFwk::AbilityInfoFlag::GET_ABILITY_INFO_DEFAULT, userId, navigationBarInfo))) &&
            waitCnt < MAX_WAIT_SYSTEM_UI_NUM) {
        HILOG_INFO("Waiting query system ui info completed.");
        usleep(REPOLL_TIME_MICRO_SECONDS);
        waitCnt++;
    }

    HILOG_INFO("start ui mode : %{public}d", mode);
    switch (mode) {
        case SatrtUiMode::STATUSBAR:
            StartSystemUi(AbilityConfig::SYSTEM_UI_STATUS_BAR);
            break;
        case SatrtUiMode::NAVIGATIONBAR:
            StartSystemUi(AbilityConfig::SYSTEM_UI_NAVIGATION_BAR);
            break;
        case SatrtUiMode::STARTUIBOTH:
            StartSystemUi(AbilityConfig::SYSTEM_UI_STATUS_BAR);
            StartSystemUi(AbilityConfig::SYSTEM_UI_NAVIGATION_BAR);
            break;
        default:
            HILOG_INFO("Input mode error ...");
            break;
    }
}

bool AbilityManagerService::CheckCallerIsSystemAppByIpc()
{
    HILOG_DEBUG("%{public}s begin", __func__);
    auto bms = GetBundleManager();
    CHECK_POINTER_RETURN_BOOL(bms);
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    HILOG_ERROR("callerUid %{public}d", callerUid);
    return bms->CheckIsSystemAppByUid(callerUid);
}

int AbilityManagerService::GetWantSenderInfo(const sptr<IWantSender> &target, std::shared_ptr<WantSenderInfo> &info)
{
    HILOG_INFO("Get pending request info.");
    CHECK_POINTER_AND_RETURN(pendingWantManager_, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(target, ERR_INVALID_VALUE);
    CHECK_POINTER_AND_RETURN(info, ERR_INVALID_VALUE);
    return pendingWantManager_->GetWantSenderInfo(target, info);
}

void AbilityManagerService::UpdateLockScreenState(bool isLockScreen)
{
    HILOG_DEBUG("%{public}s begin", __func__);
    CHECK_POINTER(currentStackManager_);
    currentStackManager_->UpdateLockScreenState(isLockScreen);
}

/**
 * Get system memory information.
 * @param SystemMemoryAttr, memory information.
 */
void AbilityManagerService::GetSystemMemoryAttr(AppExecFwk::SystemMemoryAttr &memoryInfo)
{
    auto appScheduler = DelayedSingleton<AppScheduler>::GetInstance();
    if (appScheduler == nullptr) {
        HILOG_ERROR("%{public}s, appScheduler is nullptr", __func__);
        return;
    }

    int memoryThreshold = 0;
    if (amsConfigResolver_ == nullptr) {
        HILOG_ERROR("%{public}s, amsConfigResolver_ is nullptr", __func__);
        memoryThreshold = EXPERIENCE_MEM_THRESHOLD;
    } else {
        memoryThreshold = amsConfigResolver_->GetMemThreshold(AmsConfig::MemThreshold::HOME_APP);
    }

    nlohmann::json memJson = { "memoryThreshold", memoryThreshold };
    std::string memConfig = memJson.dump();

    appScheduler->GetSystemMemoryAttr(memoryInfo, memConfig);
}

int AbilityManagerService::GetMissionSaveTime() const
{
    if (!amsConfigResolver_) {
        return 0;
    }

    return amsConfigResolver_->GetMissionSaveTime();
}

int32_t AbilityManagerService::GetMissionIdByAbilityToken(const sptr<IRemoteObject> &token)
{
    if (!currentMissionListManager_) {
        return -1;
    }
    return currentMissionListManager_->GetMissionIdByAbilityToken(token);
}

sptr<IRemoteObject> AbilityManagerService::GetAbilityTokenByMissionId(int32_t missionId)
{
    if (!currentMissionListManager_) {
        return nullptr;
    }
    return currentMissionListManager_->GetAbilityTokenByMissionId(missionId);
}

void AbilityManagerService::StartingSettingsDataAbility()
{
    HILOG_DEBUG("%{public}s", __func__);
    if (!iBundleManager_) {
        HILOG_INFO("bms service is null");
        return;
    }

    AppExecFwk::AbilityInfo abilityInfo;
    Want want;
    want.SetElementName(AbilityConfig::SETTINGS_DATA_BUNDLE_NAME, AbilityConfig::SETTINGS_DATA_ABILITY_NAME);
    uint32_t waitCnt = 0;
    // Wait 5 minutes for the installation to complete.
    auto userId = GetUserId();
    while (!iBundleManager_->QueryAbilityInfo(want, OHOS::AppExecFwk::AbilityInfoFlag::GET_ABILITY_INFO_DEFAULT,
        userId, abilityInfo) && waitCnt < MAX_WAIT_SETTINGS_DATA_NUM) {
        HILOG_INFO("Waiting query settings data info completed.");
        usleep(REPOLL_TIME_MICRO_SECONDS);
        waitCnt++;
    }

    // node: do not use abilityInfo.uri directly, need check uri first.
    auto GetValidUri = [&]() -> std::string {
        int32_t firstSeparator = abilityInfo.uri.find_first_of('/');
        int32_t lastSeparator = abilityInfo.uri.find_last_of('/');
        if (lastSeparator - firstSeparator != 1) {
            HILOG_ERROR("ability info uri error, uri: %{public}s", abilityInfo.uri.c_str());
            return "";
        }

        std::string uriStr = abilityInfo.uri;
        uriStr.insert(lastSeparator, "/");
        return uriStr;
    };

    std::string abilityUri = GetValidUri();
    if (abilityUri.empty()) {
        return;
    }

    HILOG_INFO("abilityInfo uri: %{public}s.", abilityUri.c_str());

    // start settings data ability
    Uri uri(abilityUri);
    (void)AcquireDataAbility(uri, true, nullptr);
}

int AbilityManagerService::SetMissionLabel(const sptr<IRemoteObject> &token, const std::string &label)
{
    HILOG_DEBUG("%{public}s", __func__);
    auto missionListManager = currentMissionListManager_;
    if (missionListManager) {
        missionListManager->SetMissionLabel(token, label);
    }
    return 0;
}

int AbilityManagerService::StartUser(int userId)
{
    HILOG_DEBUG("%{public}s, userId:%{public}d", __func__, userId);
    if (userController_) {
        return userController_->StartUser(userId, true);
    }
    return 0;
}

int AbilityManagerService::StopUser(int userId, const sptr<IStopUserCallback> &callback)
{
    HILOG_DEBUG("%{public}s", __func__);
    if (callback) {
        callback->OnStopUserDone(userId, ERR_OK);
    }
    return 0;
}

void AbilityManagerService::OnAcceptWantResponse(
    const AAFwk::Want &want, const std::string &flag)
{
    HILOG_DEBUG("On accept want response");
    if (!currentMissionListManager_) {
        return;
    }
    currentMissionListManager_->OnAcceptWantResponse(want, flag);
}

void AbilityManagerService::OnStartSpecifiedAbilityTimeoutResponse(const AAFwk::Want &want)
{
    return;
}
int AbilityManagerService::GetAbilityRunningInfos(std::vector<AbilityRunningInfo> &info)
{
    HILOG_DEBUG("Get running ability infos.");
    auto bundleMgr = GetBundleManager();
    if (!bundleMgr) {
        HILOG_ERROR("bundleMgr is nullptr.");
        return INNER_ERR;
    }

    auto callerUid = IPCSkeleton::GetCallingUid();
    auto isSystem = bundleMgr->CheckIsSystemAppByUid(callerUid);
    HILOG_DEBUG("callerUid : %{public}d, isSystem : %{public}d", callerUid, static_cast<int>(isSystem));

    if (!isSystem) {
        HILOG_ERROR("callar is not system app.");
        return INNER_ERR;
    }

    currentMissionListManager_->GetAbilityRunningInfos(info);
    kernalAbilityManager_->GetAbilityRunningInfos(info);
    connectManager_->GetAbilityRunningInfos(info);
    dataAbilityManager_->GetAbilityRunningInfos(info);

    return ERR_OK;
}

int AbilityManagerService::GetExtensionRunningInfos(int upperLimit, std::vector<ExtensionRunningInfo> &info)
{
    HILOG_DEBUG("Get extension infos, upperLimit : %{public}d", upperLimit);
    auto bundleMgr = GetBundleManager();
    if (!bundleMgr) {
        HILOG_ERROR("bundleMgr is nullptr.");
        return INNER_ERR;
    }

    auto callerUid = IPCSkeleton::GetCallingUid();
    auto isSystem = bundleMgr->CheckIsSystemAppByUid(callerUid);
    HILOG_DEBUG("callerUid : %{public}d, isSystem : %{public}d", callerUid, static_cast<int>(isSystem));

    if (!isSystem) {
        HILOG_ERROR("callar is not system app.");
        return INNER_ERR;
    }

    connectManager_->GetExtensionRunningInfos(upperLimit, info);
    return ERR_OK;
}

int AbilityManagerService::GetProcessRunningInfos(std::vector<AppExecFwk::RunningProcessInfo> &info)
{
    return DelayedSingleton<AppScheduler>::GetInstance()->GetProcessRunningInfos(info);
}

void AbilityManagerService::ClearUserData(int32_t userId)
{
    HILOG_DEBUG("%{public}s", __func__);
    missionListManagers_.erase(userId);
    connectManagers_.erase(userId);
    dataAbilityManagers_.erase(userId);
    pendingWantManagers_.erase(userId);
}

int AbilityManagerService::RegisterSnapshotHandler(const sptr<ISnapshotHandler>& handler)
{
    if (!currentMissionListManager_) {
        HILOG_ERROR("snapshot: currentMissionListManager_ is nullptr.");
        return 0;
    }
    currentMissionListManager_->RegisterSnapshotHandler(handler);
    HILOG_INFO("snapshot: AbilityManagerService register snapshot handler success.");
    return 0;
}

int32_t AbilityManagerService::GetMissionSnapshot(const std::string& deviceId, int32_t missionId,
    MissionSnapshot& missionSnapshot)
{
    if (CheckIsRemote(deviceId)) {
        HILOG_INFO("get remote mission snapshot.");
        return GetRemoteMissionSnapshotInfo(deviceId, missionId, missionSnapshot);
    }
    HILOG_INFO("get local mission snapshot.");
    if (!currentMissionListManager_) {
        HILOG_ERROR("snapshot: currentMissionListManager_ is nullptr.");
        return -1;
    }
    auto token = GetAbilityTokenByMissionId(missionId);
    currentMissionListManager_->GetMissionSnapshot(missionId, token, missionSnapshot);
    return 0;
}

int32_t AbilityManagerService::GetRemoteMissionSnapshotInfo(const std::string& deviceId, int32_t missionId,
    MissionSnapshot& missionSnapshot)
{
    HILOG_INFO("GetRemoteMissionSnapshotInfo begin");
    std::unique_ptr<MissionSnapshot> missionSnapshotPtr = std::make_unique<MissionSnapshot>();
    DistributedClient dmsClient;
    int result = dmsClient.GetRemoteMissionSnapshotInfo(deviceId, missionId, missionSnapshotPtr);
    if (result != ERR_OK) {
        HILOG_ERROR("GetRemoteMissionSnapshotInfo failed, result = %{public}d", result);
        return result;
    }
    missionSnapshot = *missionSnapshotPtr;
    return ERR_OK;
}

void AbilityManagerService::StartFreezingScreen()
{
    HILOG_INFO("%{public}s", __func__);
}

void AbilityManagerService::StopFreezingScreen()
{
    HILOG_INFO("%{public}s", __func__);
}

void AbilityManagerService::UserStarted(int32_t userId)
{
    HILOG_INFO("%{public}s", __func__);
    InitConnectManager(userId, false);
    SetStackManager(userId, false);
    InitMissionListManager(userId, false);
    InitDataAbilityManager(userId, false);
    InitPendWantManager(userId, false);
}

void AbilityManagerService::SwitchToUser(int32_t oldUserId, int32_t userId)
{
    HILOG_INFO("%{public}s, oldUserId:%{public}d, newUserId:%{public}d", __func__, oldUserId, userId);
    SwitchManagers(userId);
    PauseOldUser(oldUserId);
    StartUserApps(userId);
}

void AbilityManagerService::SwitchManagers(int32_t userId)
{
    HILOG_INFO("%{public}s, SwitchManagers:%{public}d-----begin", __func__, userId);
    InitConnectManager(userId, true);
    SetStackManager(userId, true);
    InitMissionListManager(userId, true);
    InitDataAbilityManager(userId, true);
    InitPendWantManager(userId, true);
    HILOG_INFO("%{public}s, SwitchManagers:%{public}d-----end", __func__, userId);
}

void AbilityManagerService::PauseOldUser(int32_t userId)
{
    HILOG_INFO("%{public}s, PauseOldUser:%{public}d-----begin", __func__, userId);
    if (useNewMission_) {
        PauseOldMissionListManager(userId);
    } else {
        PauseOldStackManager(userId);
    }
    HILOG_INFO("%{public}s, PauseOldUser:%{public}d-----end", __func__, userId);
}

void AbilityManagerService::PauseOldMissionListManager(int32_t userId)
{
    HILOG_INFO("%{public}s, PauseOldMissionListManager:%{public}d-----begin", __func__, userId);
    auto it = missionListManagers_.find(userId);
    if (it == missionListManagers_.end()) {
        HILOG_INFO("%{public}s, PauseOldMissionListManager:%{public}d-----end1", __func__, userId);
        return;
    }
    auto manager = it->second;
    if (!manager) {
        HILOG_INFO("%{public}s, PauseOldMissionListManager:%{public}d-----end2", __func__, userId);
        return;
    }
    manager->PauseManager();
    HILOG_INFO("%{public}s, PauseOldMissionListManager:%{public}d-----end", __func__, userId);
}

void AbilityManagerService::PauseOldStackManager(int32_t userId)
{
    auto it = stackManagers_.find(userId);
    if (it == stackManagers_.end()) {
        return;
    }
    auto manager = it->second;
    if (!manager) {
        return;
    }
    manager->PauseManager();
}

void AbilityManagerService::StartUserApps(int32_t userId)
{
    HILOG_INFO("StartUserApps, userId:%{public}d, currentUserId:%{public}d", userId, GetUserId());
    if (useNewMission_) {
        if (currentMissionListManager_ && currentMissionListManager_->IsStarted()) {
            HILOG_INFO("missionListManager ResumeManager");
            currentMissionListManager_->ResumeManager();
            return;
        }
    } else {
        if (currentStackManager_ && currentStackManager_->IsStarted()) {
            HILOG_INFO("stack ResumeManager");
            currentStackManager_->ResumeManager();
            return;
        }
    }
    StartSystemAbilityByUser(userId);
}

void AbilityManagerService::StartSystemAbilityByUser(int32_t userId)
{
    HILOG_INFO("StartSystemAbilityByUser, userId:%{public}d, currentUserId:%{public}d", userId, GetUserId());
    ConnectBmsService();

    if (!amsConfigResolver_ || amsConfigResolver_->NonConfigFile()) {
        HILOG_INFO("start all");
        StartingLauncherAbility();
        StartingSettingsDataAbility();
        return;
    }

    if (amsConfigResolver_->GetStartLauncherState()) {
        HILOG_INFO("start launcher");
        StartingLauncherAbility();
    }

    if (amsConfigResolver_->GetStartSettingsDataState()) {
        HILOG_INFO("start settingsdata");
        StartingSettingsDataAbility();
    }

    if (amsConfigResolver_->GetPhoneServiceState()) {
        HILOG_INFO("start phone service");
        StartingPhoneServiceAbility();
    }

    if (amsConfigResolver_->GetStartContactsState()) {
        HILOG_INFO("start contacts");
        StartingContactsAbility();
    }

    if (amsConfigResolver_->GetStartMmsState()) {
        HILOG_INFO("start mms");
        StartingMmsAbility();
    }
}

void AbilityManagerService::InitConnectManager(int32_t userId, bool switchUser)
{
    auto it = connectManagers_.find(userId);
    if (it == connectManagers_.end()) {
        auto manager = std::make_shared<AbilityConnectManager>();
        manager->SetEventHandler(handler_);
        connectManagers_.emplace(userId, manager);
        if (switchUser) {
            connectManager_ = manager;
        }
    } else {
        if (switchUser) {
            connectManager_ = it->second;
        }
    }
}

void AbilityManagerService::InitDataAbilityManager(int32_t userId, bool switchUser)
{
    auto it = dataAbilityManagers_.find(userId);
    if (it == dataAbilityManagers_.end()) {
        auto manager = std::make_shared<DataAbilityManager>();
        dataAbilityManagers_.emplace(userId, manager);
        if (switchUser) {
            dataAbilityManager_ = manager;
        }
    } else {
        if (switchUser) {
            dataAbilityManager_ = it->second;
        }
    }
}

void AbilityManagerService::InitPendWantManager(int32_t userId, bool switchUser)
{
    auto it = pendingWantManagers_.find(userId);
    if (it == pendingWantManagers_.end()) {
        auto manager = std::make_shared<PendingWantManager>();
        pendingWantManagers_.emplace(userId, manager);
        if (switchUser) {
            pendingWantManager_ = manager;
        }
    } else {
        if (switchUser) {
            pendingWantManager_ = it->second;
        }
    }
}

int AbilityManagerService::SetAbilityController(const sptr<IAbilityController> &abilityController,
    bool imAStabilityTest)
{
    HILOG_DEBUG("%{public}s, imAStabilityTest: %{public}d", __func__, imAStabilityTest);
    std::lock_guard<std::recursive_mutex> guard(globalLock_);
    abilityController_ = abilityController;
    controllerIsAStabilityTest_ = imAStabilityTest;
    return ERR_OK;
}

bool AbilityManagerService::SendANRProcessID(int pid)
{
    int anrTimeOut = amsConfigResolver_->GetANRTimeOutTime();
    auto timeoutTask = [pid]() {
        if (kill(pid, SIGKILL) != ERR_OK) {
            HILOG_ERROR("Kill app not response process failed");
        }
    };
    if (kill(pid, SIGUSR1) != ERR_OK) {
        HILOG_ERROR("Send sig to app not response process failed");
    }
    handler_->PostTask(timeoutTask, "TIME_OUT_TASK", anrTimeOut);
    return true;
}

bool AbilityManagerService::IsRunningInStabilityTest()
{
    std::lock_guard<std::recursive_mutex> guard(globalLock_);
    bool ret = abilityController_ != nullptr && controllerIsAStabilityTest_;
    HILOG_DEBUG("%{public}s, IsRunningInStabilityTest: %{public}d", __func__, ret);
    return ret;
}

bool AbilityManagerService::IsAbilityControllerStart(const Want &want, const std::string &bundleName)
{
    if (abilityController_ != nullptr) {
        HILOG_DEBUG("%{public}s, controllerIsAStabilityTest_: %{public}d", __func__, controllerIsAStabilityTest_);
        bool isStart = abilityController_->AllowAbilityStart(want, bundleName);
        if (!isStart) {
            HILOG_INFO("Not finishing start ability because controller starting: %{public}s", bundleName.c_str());
            return false;
        }
    }
    return true;
}

bool AbilityManagerService::IsAbilityControllerForeground(const std::string &bundleName)
{
    if (abilityController_ != nullptr) {
        HILOG_DEBUG("%{public}s, controllerIsAStabilityTest_: %{public}d", __func__, controllerIsAStabilityTest_);
        bool isResume = abilityController_->AllowAbilityForeground(bundleName);
        if (!isResume) {
            HILOG_INFO("Not finishing terminate ability because controller resuming: %{public}s", bundleName.c_str());
            return false;
        }
    }
    return true;
}

int32_t AbilityManagerService::InitAbilityInfoFromExtension(AppExecFwk::ExtensionAbilityInfo &extensionInfo,
    AppExecFwk::AbilityInfo &abilityInfo)
{
    abilityInfo.bundleName = extensionInfo.bundleName;
    abilityInfo.package = extensionInfo.moduleName;
    abilityInfo.moduleName = extensionInfo.moduleName;
    abilityInfo.name = extensionInfo.name;
    abilityInfo.srcEntrance = extensionInfo.srcEntrance;
    abilityInfo.srcPath = extensionInfo.srcEntrance;
    abilityInfo.iconPath = extensionInfo.icon;
    abilityInfo.iconId = extensionInfo.iconId;
    abilityInfo.label = extensionInfo.label;
    abilityInfo.labelId = extensionInfo.labelId;
    abilityInfo.description = extensionInfo.description;
    abilityInfo.descriptionId = extensionInfo.descriptionId;
    abilityInfo.permissions = extensionInfo.permissions;
    abilityInfo.readPermission = extensionInfo.readPermission;
    abilityInfo.writePermission = extensionInfo.writePermission;
    abilityInfo.extensionAbilityType = extensionInfo.type;
    abilityInfo.visible = extensionInfo.visible;
    abilityInfo.applicationInfo = extensionInfo.applicationInfo;
    abilityInfo.resourcePath = extensionInfo.resourcePath;
    abilityInfo.enabled = extensionInfo.enabled;
    abilityInfo.isModuleJson = true;
    abilityInfo.isStageBasedModel = true;
    abilityInfo.process = extensionInfo.process;
    abilityInfo.type = AppExecFwk::AbilityType::EXTENSION;
    return 0;
}

int32_t AbilityManagerService::GetAbilityInfoFromExtension(const Want &want, AppExecFwk::AbilityInfo &abilityInfo)
{
    ElementName elementName = want.GetElement();
    std::string bundleName = elementName.GetBundleName();
    std::string abilityName = elementName.GetAbilityName();
    AppExecFwk::BundleMgrClient bundleClient;
    AppExecFwk::BundleInfo bundleInfo;
    if (!bundleClient.GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO, bundleInfo)) {
        HILOG_ERROR("Failed to get bundle info when generate ability request.");
        return RESOLVE_APP_ERR;
    }
    bool found = false;

    for (auto &extensionInfo: bundleInfo.extensionInfos) {
        if (extensionInfo.name != abilityName) {
            continue;
        }
        found = true;
        HILOG_DEBUG("GetExtensionAbilityInfo, extension ability info found, name=%{public}s", abilityName.c_str());
        abilityInfo.applicationName = bundleInfo.applicationInfo.name;
        InitAbilityInfoFromExtension(extensionInfo, abilityInfo);
        break;
    }

    return found;
}

int32_t AbilityManagerService::ShowPickerDialog(const Want& want, int32_t userId)
{
    auto bms = GetBundleManager();
    CHECK_POINTER_AND_RETURN(bms, GET_ABILITY_SERVICE_FAILED);
    HILOG_INFO("share content: ShowPickerDialog");
    std::vector<AppExecFwk::AbilityInfo> abilityInfos;
    bms->QueryAbilityInfos(want, AppExecFwk::AbilityInfoFlag::GET_ABILITY_INFO_WITH_APPLICATION, userId, abilityInfos);
    return Ace::UIServiceMgrClient::GetInstance()->ShowAppPickerDialog(want, abilityInfos);
}

void AbilityManagerService::UpdateCallerInfo(Want& want)
{
    int32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t callerUid = IPCSkeleton::GetCallingUid();
    int32_t callerPid = IPCSkeleton::GetCallingPid();
    want.SetParam(Want::PARAM_RESV_CALLER_TOKEN, tokenId);
    want.SetParam(Want::PARAM_RESV_CALLER_UID, callerUid);
    want.SetParam(Want::PARAM_RESV_CALLER_PID, callerPid);
}
}  // namespace AAFwk
}  // namespace OHOS
