/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "ability_context_impl.h"

#include <native_engine/native_engine.h>

#include "ability_manager_client.h"
#include "hitrace_meter.h"
#include "connection_manager.h"
#include "dialog_request_callback_impl.h"
#include "hilog_wrapper.h"
#include "remote_object_wrapper.h"
#include "request_constants.h"
#include "string_wrapper.h"
#include "want_params_wrapper.h"

namespace OHOS {
namespace AbilityRuntime {
const size_t AbilityContext::CONTEXT_TYPE_ID(std::hash<const char*> {} ("AbilityContext"));

struct RequestResult {
    int32_t resultCode {0};
    AAFwk::Want resultWant;
    RequestDialogResultTask task;
};

Global::Resource::DeviceType AbilityContextImpl::GetDeviceType() const
{
    return (stageContext_ != nullptr) ? stageContext_->GetDeviceType() : Global::Resource::DeviceType::DEVICE_PHONE;
}

std::string AbilityContextImpl::GetBaseDir() const
{
    return stageContext_ ? stageContext_->GetBaseDir() : "";
}

std::string AbilityContextImpl::GetBundleCodeDir()
{
    return stageContext_ ? stageContext_->GetBundleCodeDir() : "";
}

std::string AbilityContextImpl::GetCacheDir()
{
    return stageContext_ ? stageContext_->GetCacheDir() : "";
}

std::string AbilityContextImpl::GetDatabaseDir()
{
    return stageContext_ ? stageContext_->GetDatabaseDir() : "";
}

std::string AbilityContextImpl::GetPreferencesDir()
{
    return stageContext_ ? stageContext_->GetPreferencesDir() : "";
}

std::string AbilityContextImpl::GetTempDir()
{
    return stageContext_ ? stageContext_->GetTempDir() : "";
}

std::string AbilityContextImpl::GetFilesDir()
{
    return stageContext_ ? stageContext_->GetFilesDir() : "";
}

std::string AbilityContextImpl::GetDistributedFilesDir()
{
    return stageContext_ ? stageContext_->GetDistributedFilesDir() : "";
}

bool AbilityContextImpl::IsUpdatingConfigurations()
{
    return stageContext_ ? stageContext_->IsUpdatingConfigurations() : false;
}

bool AbilityContextImpl::PrintDrawnCompleted()
{
    return stageContext_ ? stageContext_->PrintDrawnCompleted() : false;
}

void AbilityContextImpl::SwitchArea(int mode)
{
    HILOG_INFO("mode:%{public}d.", mode);
    if (stageContext_ != nullptr) {
        stageContext_->SwitchArea(mode);
    }
}

int AbilityContextImpl::GetArea()
{
    HILOG_DEBUG("AbilityContextImpl::GetArea.");
    if (stageContext_ == nullptr) {
        HILOG_ERROR("AbilityContextImpl::stageContext is nullptr.");
        return ContextImpl::EL_DEFAULT;
    }
    return stageContext_->GetArea();
}

ErrCode AbilityContextImpl::StartAbility(const AAFwk::Want& want, int requestCode)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_DEBUG("Start calling StartAbility.");
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, token_, requestCode);
    HILOG_INFO("StartAbility. ret=%{public}d", err);
    return err;
}

ErrCode AbilityContextImpl::StartAbilityAsCaller(const AAFwk::Want &want, int requestCode)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_DEBUG("Start calling StartAbilityAsCaller.");
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbilityAsCaller(want, token_, requestCode);
    HILOG_INFO("StartAbilityAsCaller. ret=%{public}d", err);
    return err;
}

ErrCode AbilityContextImpl::StartAbilityWithAccount(const AAFwk::Want& want, int accountId, int requestCode)
{
    HILOG_DEBUG("AbilityContextImpl::StartAbilityWithAccount. Start calling StartAbility.");
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, token_, requestCode, accountId);
    HILOG_INFO("StartAbility. ret=%{public}d", err);
    return err;
}

ErrCode AbilityContextImpl::StartAbility(const AAFwk::Want& want, const AAFwk::StartOptions& startOptions,
    int requestCode)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_DEBUG("AbilityContextImpl::StartAbility. Start calling StartAbility.");
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, startOptions, token_, requestCode);
    HILOG_INFO("StartAbility. ret=%{public}d", err);
    return err;
}

ErrCode AbilityContextImpl::StartAbilityAsCaller(const AAFwk::Want &want, const AAFwk::StartOptions &startOptions,
    int requestCode)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_DEBUG("AbilityContextImpl::StartAbilityAsCaller. Start calling StartAbilityAsCaller.");
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbilityAsCaller(want,
        startOptions, token_, requestCode);
    HILOG_INFO("StartAbilityAsCaller. ret=%{public}d", err);
    return err;
}

ErrCode AbilityContextImpl::StartAbilityWithAccount(
    const AAFwk::Want& want, int accountId, const AAFwk::StartOptions& startOptions, int requestCode)
{
    HILOG_INFO("name:%{public}s %{public}s, accountId=%{public}d",
        want.GetElement().GetBundleName().c_str(), want.GetElement().GetAbilityName().c_str(), accountId);
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(
        want, startOptions, token_, requestCode, accountId);
    HILOG_INFO("StartAbility. ret=%{public}d", err);
    return err;
}

ErrCode AbilityContextImpl::StartAbilityForResult(const AAFwk::Want& want, int requestCode, RuntimeTask&& task)
{
    HILOG_DEBUG("Start calling StartAbilityForResult.");
    resultCallbacks_.insert(make_pair(requestCode, std::move(task)));
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, token_, requestCode);
    HILOG_INFO("StartAbilityForResult. ret=%{public}d", err);
    if (err != ERR_OK) {
        OnAbilityResultInner(requestCode, err, want);
    }
    return err;
}

ErrCode AbilityContextImpl::StartAbilityForResultWithAccount(
    const AAFwk::Want& want, const int accountId, int requestCode, RuntimeTask&& task)
{
    HILOG_DEBUG("accountId:%{private}d", accountId);
    resultCallbacks_.insert(make_pair(requestCode, std::move(task)));
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, token_, requestCode, accountId);
    HILOG_INFO("ok. ret=%{public}d", err);
    if (err != ERR_OK) {
        OnAbilityResultInner(requestCode, err, want);
    }
    return err;
}

ErrCode AbilityContextImpl::StartAbilityForResult(const AAFwk::Want& want, const AAFwk::StartOptions& startOptions,
    int requestCode, RuntimeTask&& task)
{
    HILOG_DEBUG("Start calling StartAbilityForResult.");
    resultCallbacks_.insert(make_pair(requestCode, std::move(task)));
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, startOptions, token_, requestCode);
    HILOG_INFO("StartAbilityForResult. ret=%{public}d", err);
    if (err != ERR_OK) {
        OnAbilityResultInner(requestCode, err, want);
    }
    return err;
}

ErrCode AbilityContextImpl::StartAbilityForResultWithAccount(
    const AAFwk::Want& want, int accountId, const AAFwk::StartOptions& startOptions,
    int requestCode, RuntimeTask&& task)
{
    HILOG_DEBUG("Start calling StartAbilityForResultWithAccount.");
    resultCallbacks_.insert(make_pair(requestCode, std::move(task)));
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(
        want, startOptions, token_, requestCode, accountId);
    HILOG_INFO("StartAbilityForResultWithAccount. ret=%{public}d", err);
    if (err != ERR_OK) {
        OnAbilityResultInner(requestCode, err, want);
    }
    return err;
}

ErrCode AbilityContextImpl::StartServiceExtensionAbility(const AAFwk::Want& want, int32_t accountId)
{
    HILOG_INFO("name:%{public}s %{public}s, accountId=%{public}d",
        want.GetElement().GetBundleName().c_str(), want.GetElement().GetAbilityName().c_str(), accountId);
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StartExtensionAbility(
        want, token_, accountId, AppExecFwk::ExtensionAbilityType::SERVICE);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContextImpl::StartServiceExtensionAbility is failed %{public}d", err);
    }
    return err;
}

ErrCode AbilityContextImpl::StopServiceExtensionAbility(const AAFwk::Want& want, int32_t accountId)
{
    HILOG_INFO("name:%{public}s %{public}s, accountId=%{public}d",
        want.GetElement().GetBundleName().c_str(), want.GetElement().GetAbilityName().c_str(), accountId);
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->StopExtensionAbility(
        want, token_, accountId, AppExecFwk::ExtensionAbilityType::SERVICE);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContextImpl::StopServiceExtensionAbility is failed %{public}d", err);
    }
    return err;
}

ErrCode AbilityContextImpl::TerminateAbilityWithResult(const AAFwk::Want& want, int resultCode)
{
    HILOG_DEBUG("Start calling TerminateAbilityWithResult.");
    isTerminating_ = true;
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->TerminateAbility(token_, resultCode, &want);
    HILOG_INFO("TerminateAbilityWithResult. ret=%{public}d", err);
    return err;
}

void AbilityContextImpl::OnAbilityResult(int requestCode, int resultCode, const AAFwk::Want& resultData)
{
    HILOG_DEBUG("Start calling OnAbilityResult.");
    auto callback = resultCallbacks_.find(requestCode);
    if (callback != resultCallbacks_.end()) {
        if (callback->second) {
            callback->second(resultCode, resultData, false);
        }
        resultCallbacks_.erase(requestCode);
    }
    HILOG_INFO("OnAbilityResult");
}

void AbilityContextImpl::OnAbilityResultInner(int requestCode, int resultCode, const AAFwk::Want& resultData)
{
    HILOG_DEBUG("Start calling OnAbilityResult.");
    auto callback = resultCallbacks_.find(requestCode);
    if (callback != resultCallbacks_.end()) {
        if (callback->second) {
            callback->second(resultCode, resultData, true);
        }
        resultCallbacks_.erase(requestCode);
    }
    HILOG_INFO("OnAbilityResult");
}

ErrCode AbilityContextImpl::ConnectAbility(const AAFwk::Want& want, const sptr<AbilityConnectCallback>& connectCallback)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_DEBUG("ConnectAbility begin, name:%{public}s.", abilityInfo_ == nullptr ? "" : abilityInfo_->name.c_str());
    ErrCode ret = ConnectionManager::GetInstance().ConnectAbility(token_, want, connectCallback);
    HILOG_INFO("ConnectAbility ret:%{public}d", ret);
    return ret;
}

ErrCode AbilityContextImpl::ConnectAbilityWithAccount(const AAFwk::Want& want, int accountId,
    const sptr<AbilityConnectCallback>& connectCallback)
{
    HILOG_DEBUG("%{public}s begin.", __func__);
    ErrCode ret =
        ConnectionManager::GetInstance().ConnectAbilityWithAccount(token_, want, accountId, connectCallback);
    HILOG_INFO("ConnectAbility ret:%{public}d", ret);
    return ret;
}

void AbilityContextImpl::DisconnectAbility(const AAFwk::Want& want,
    const sptr<AbilityConnectCallback>& connectCallback)
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_DEBUG("DisconnectAbility begin, caller:%{public}s.",
        abilityInfo_ == nullptr ? "" : abilityInfo_->name.c_str());
    ErrCode ret =
        ConnectionManager::GetInstance().DisconnectAbility(token_, want.GetElement(), connectCallback);
    if (ret != ERR_OK) {
        HILOG_ERROR("%{public}s end DisconnectAbility error, ret=%{public}d", __func__, ret);
    }
}

std::string AbilityContextImpl::GetBundleName() const
{
    return stageContext_ ? stageContext_->GetBundleName() : "";
}

std::shared_ptr<AppExecFwk::ApplicationInfo> AbilityContextImpl::GetApplicationInfo() const
{
    return stageContext_ ? stageContext_->GetApplicationInfo() : nullptr;
}

std::string AbilityContextImpl::GetBundleCodePath() const
{
    return stageContext_ ? stageContext_->GetBundleCodePath() : "";
}

std::shared_ptr<AppExecFwk::HapModuleInfo> AbilityContextImpl::GetHapModuleInfo() const
{
    return stageContext_ ? stageContext_->GetHapModuleInfo() : nullptr;
}

std::shared_ptr<Global::Resource::ResourceManager> AbilityContextImpl::GetResourceManager() const
{
    return stageContext_ ? stageContext_->GetResourceManager() : nullptr;
}

std::shared_ptr<Context> AbilityContextImpl::CreateBundleContext(const std::string& bundleName)
{
    return stageContext_ ? stageContext_->CreateBundleContext(bundleName) : nullptr;
}

std::shared_ptr<Context> AbilityContextImpl::CreateModuleContext(const std::string& moduleName)
{
    return stageContext_ ? stageContext_->CreateModuleContext(moduleName) : nullptr;
}

std::shared_ptr<Context> AbilityContextImpl::CreateModuleContext(const std::string& bundleName,
    const std::string& moduleName)
{
    return stageContext_ ? stageContext_->CreateModuleContext(bundleName, moduleName) : nullptr;
}

void AbilityContextImpl::SetAbilityInfo(const std::shared_ptr<AppExecFwk::AbilityInfo>& abilityInfo)
{
    abilityInfo_ = abilityInfo;
}

std::shared_ptr<AppExecFwk::AbilityInfo> AbilityContextImpl::GetAbilityInfo() const
{
    return abilityInfo_;
}

void AbilityContextImpl::SetStageContext(const std::shared_ptr<AbilityRuntime::Context>& stageContext)
{
    stageContext_ = stageContext;
}

void AbilityContextImpl::SetConfiguration(const std::shared_ptr<AppExecFwk::Configuration>& config)
{
    config_ = config;
}

std::shared_ptr<AppExecFwk::Configuration> AbilityContextImpl::GetConfiguration() const
{
    return config_;
}

void AbilityContextImpl::MinimizeAbility(bool fromUser)
{
    HILOG_DEBUG("call");
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->MinimizeAbility(token_, fromUser);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContext::MinimizeAbility is failed %{public}d", err);
    }
}

ErrCode AbilityContextImpl::TerminateSelf()
{
    HILOG_DEBUG("%{public}s begin.", __func__);
    isTerminating_ = true;
    AAFwk::Want resultWant;
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->TerminateAbility(token_, -1, &resultWant);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContextImpl::TerminateSelf is failed %{public}d", err);
    }
    return err;
}

ErrCode AbilityContextImpl::CloseAbility()
{
    HITRACE_METER_NAME(HITRACE_TAG_ABILITY_MANAGER, __PRETTY_FUNCTION__);
    HILOG_DEBUG("%{public}s begin.", __func__);
    isTerminating_ = true;
    AAFwk::Want resultWant;
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->CloseAbility(token_, -1, &resultWant);
    if (err != ERR_OK) {
        HILOG_ERROR("CloseAbility failed: %{public}d", err);
    }
    return err;
}

sptr<IRemoteObject> AbilityContextImpl::GetToken()
{
    return token_;
}

ErrCode AbilityContextImpl::RestoreWindowStage(NativeEngine& engine, NativeValue* contentStorage)
{
    HILOG_INFO("call");
    contentStorage_ = std::unique_ptr<NativeReference>(engine.CreateReference(contentStorage, 1));
    return ERR_OK;
}

ErrCode AbilityContextImpl::StartAbilityByCall(
    const AAFwk::Want& want, const std::shared_ptr<CallerCallBack>& callback, int32_t accountId)
{
    if (localCallContainer_ == nullptr) {
        localCallContainer_ = std::make_shared<LocalCallContainer>();
        if (localCallContainer_ == nullptr) {
            HILOG_ERROR("%{public}s failed, localCallContainer_ is nullptr.", __func__);
            return ERR_INVALID_VALUE;
        }
    }
    return localCallContainer_->StartAbilityByCallInner(want, callback, token_, accountId);
}

ErrCode AbilityContextImpl::ReleaseCall(const std::shared_ptr<CallerCallBack>& callback)
{
    HILOG_DEBUG("AbilityContextImpl::Release begin.");
    if (localCallContainer_ == nullptr) {
        HILOG_ERROR("%{public}s failed, localCallContainer_ is nullptr.", __func__);
        return ERR_INVALID_VALUE;
    }
    HILOG_DEBUG("AbilityContextImpl::Release end.");
    return localCallContainer_->ReleaseCall(callback);
}

void AbilityContextImpl::ClearFailedCallConnection(const std::shared_ptr<CallerCallBack>& callback)
{
    HILOG_DEBUG("AbilityContextImpl::Clear begin.");
    if (localCallContainer_ == nullptr) {
        HILOG_ERROR("%{public}s failed, localCallContainer_ is nullptr.", __func__);
        return;
    }
    localCallContainer_->ClearFailedCallConnection(callback);
    HILOG_DEBUG("AbilityContextImpl::Clear end.");
}

void AbilityContextImpl::RegisterAbilityCallback(std::weak_ptr<AppExecFwk::IAbilityCallback> abilityCallback)
{
    HILOG_INFO("call");
    abilityCallback_ = abilityCallback;
}

ErrCode AbilityContextImpl::RequestDialogService(NativeEngine &engine,
    AAFwk::Want &want, RequestDialogResultTask &&task)
{
    want.SetParam(RequestConstants::REQUEST_TOKEN_KEY, token_);

    auto resultTask =
        [&engine, outTask = std::move(task)](int32_t resultCode, const AAFwk::Want &resultWant) {
        auto retData = new RequestResult();
        retData->resultCode = resultCode;
        retData->resultWant = resultWant;
        retData->task = std::move(outTask);

        auto loop = engine.GetUVLoop();
        if (loop == nullptr) {
            HILOG_ERROR("RequestDialogService, fail to get uv loop.");
            return;
        }
        auto work = new uv_work_t;
        work->data = static_cast<void*>(retData);
        int rev = uv_queue_work(
            loop,
            work,
            [](uv_work_t* work) {},
            RequestDialogResultJSThreadWorker);
        if (rev != 0) {
            delete retData;
            retData = nullptr;
            if (work != nullptr) {
                delete work;
                work = nullptr;
            }
        }
    };

    sptr<IRemoteObject> remoteObject = new DialogRequestCallbackImpl(std::move(resultTask));
    want.SetParam(RequestConstants::REQUEST_CALLBACK_KEY, remoteObject);

    auto err = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want, token_, -1);
    HILOG_DEBUG("RequestDialogService ret=%{public}d", static_cast<int32_t>(err));
    return err;
}

void AbilityContextImpl::RequestDialogResultJSThreadWorker(uv_work_t* work, int status)
{
    HILOG_DEBUG("RequestDialogResultJSThreadWorker is called.");
    if (work == nullptr) {
        HILOG_ERROR("RequestDialogResultJSThreadWorker, uv_queue_work input work is nullptr");
        return;
    }
    RequestResult* retCB = static_cast<RequestResult*>(work->data);
    if (retCB == nullptr) {
        HILOG_ERROR("RequestDialogResultJSThreadWorker, retCB is nullptr");
        delete work;
        work = nullptr;
        return;
    }

    if (retCB->task) {
        retCB->task(retCB->resultCode, retCB->resultWant);
    }

    delete retCB;
    retCB = nullptr;
    delete work;
    work = nullptr;
}

ErrCode AbilityContextImpl::GetMissionId(int32_t &missionId)
{
    HILOG_DEBUG("%{public}s begin.", __func__);
    if (missionId_ != -1) {
        missionId = missionId_;
        return ERR_OK;
    }

    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->GetMissionIdByToken(token_, missionId);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContextImpl::GetMissionId is failed %{public}d", err);
    } else {
        missionId_ = missionId;
        HILOG_DEBUG("%{public}s success, missionId is %{public}d.", __func__, missionId_);
    }
    return err;
}

#ifdef SUPPORT_GRAPHICS
ErrCode AbilityContextImpl::SetMissionLabel(const std::string& label)
{
    HILOG_INFO("call label:%{public}s", label.c_str());
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->SetMissionLabel(token_, label);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContextImpl::SetMissionLabel is failed %{public}d", err);
    } else {
        HILOG_INFO("ok");
        auto abilityCallback = abilityCallback_.lock();
        if (abilityCallback) {
            abilityCallback->SetMissionLabel(label);
        }
    }
    return err;
}

ErrCode AbilityContextImpl::SetMissionIcon(const std::shared_ptr<OHOS::Media::PixelMap>& icon)
{
    HILOG_INFO("call");
    ErrCode err = AAFwk::AbilityManagerClient::GetInstance()->SetMissionIcon(token_, icon);
    if (err != ERR_OK) {
        HILOG_ERROR("AbilityContextImpl::SetMissionIcon is failed %{public}d", err);
    } else {
        HILOG_INFO("ok");
        auto abilityCallback = abilityCallback_.lock();
        if (abilityCallback) {
            abilityCallback->SetMissionIcon(icon);
        }
    }
    return err;
}

int AbilityContextImpl::GetCurrentWindowMode()
{
    HILOG_INFO("call");
    auto abilityCallback = abilityCallback_.lock();
    if (abilityCallback == nullptr) {
        return AAFwk::AbilityWindowConfiguration::MULTI_WINDOW_DISPLAY_UNDEFINED;
    }
    return abilityCallback->GetCurrentWindowMode();
}
#endif
} // namespace AbilityRuntime
} // namespace OHOS
