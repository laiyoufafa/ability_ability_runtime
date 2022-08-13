/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "application_context.h"

#include <algorithm>

#include "hilog_wrapper.h"

namespace OHOS {
namespace AbilityRuntime {
std::vector<std::shared_ptr<AbilityLifecycleCallback>> ApplicationContext::callbacks_;
std::vector<std::shared_ptr<EnvironmentCallback>> ApplicationContext::envCallbacks_;

void ApplicationContext::InitApplicationContext()
{
    std::lock_guard<std::mutex> lock(Context::contextMutex_);
    applicationContext_ = shared_from_this();
}

void ApplicationContext::AttachContextImpl(const std::shared_ptr<ContextImpl> &contextImpl)
{
    contextImpl_ = contextImpl;
}

void ApplicationContext::RegisterAbilityLifecycleCallback(
    const std::shared_ptr<AbilityLifecycleCallback> &abilityLifecycleCallback)
{
    HILOG_DEBUG("ApplicationContext RegisterAbilityLifecycleCallback");
    callbacks_.push_back(abilityLifecycleCallback);
}

void ApplicationContext::UnregisterAbilityLifecycleCallback(
    const std::shared_ptr<AbilityLifecycleCallback> &abilityLifecycleCallback)
{
    HILOG_DEBUG("ApplicationContext UnregisterAbilityLifecycleCallback");
    auto it = std::find(callbacks_.begin(), callbacks_.end(), abilityLifecycleCallback);
    if (it != callbacks_.end()) {
        callbacks_.erase(it);
    }
}

bool ApplicationContext::isAbilityLifecycleCallbackEmpty()
{
    return (callbacks_.size() == 0);
}

void ApplicationContext::RegisterEnvironmentCallback(
    const std::shared_ptr<EnvironmentCallback> &environmentCallback)
{
    HILOG_DEBUG("ApplicationContext RegisterEnvironmentCallback");
    envCallbacks_.push_back(environmentCallback);
}

void ApplicationContext::UnregisterEnvironmentCallback(
    const std::shared_ptr<EnvironmentCallback> &environmentCallback)
{
    HILOG_DEBUG("ApplicationContext UnregisterEnvironmentCallback");
    auto it = std::find(envCallbacks_.begin(), envCallbacks_.end(), environmentCallback);
    if (it != envCallbacks_.end()) {
        envCallbacks_.erase(it);
    }
}

void ApplicationContext::DispatchOnAbilityCreate(const std::shared_ptr<NativeReference> &ability)
{
    if (!ability) {
        HILOG_ERROR("ability is nullptr");
        return;
    }
    for (auto callback : callbacks_) {
        callback->OnAbilityCreate(ability);
    }
}

void ApplicationContext::DispatchOnWindowStageCreate(const std::shared_ptr<NativeReference> &ability,
    const std::shared_ptr<NativeReference> &windowStage)
{
    if (!ability || !windowStage) {
        HILOG_ERROR("ability or windowStage is nullptr");
        return;
    }
    for (auto callback : callbacks_) {
        callback->OnWindowStageCreate(ability, windowStage);
    }
}

void ApplicationContext::DispatchOnWindowStageDestroy(const std::shared_ptr<NativeReference> &ability,
    const std::shared_ptr<NativeReference> &windowStage)
{
    if (!ability || !windowStage) {
        HILOG_ERROR("ability or windowStage is nullptr");
        return;
    }
    for (auto callback : callbacks_) {
        callback->OnWindowStageDestroy(ability, windowStage);
    }
}

void ApplicationContext::DispatchWindowStageFocus(const std::shared_ptr<NativeReference> &ability,
    const std::shared_ptr<NativeReference> &windowStage)
{
    HILOG_DEBUG("%{public}s begin.", __func__);
    if (!ability || !windowStage) {
        HILOG_ERROR("ability or windowStage is nullptr");
        return;
    }
    for (auto callback : callbacks_) {
        callback->OnWindowStageActive(ability, windowStage);
    }
}

void ApplicationContext::DispatchWindowStageUnfocus(const std::shared_ptr<NativeReference> &ability,
    const std::shared_ptr<NativeReference> &windowStage)
{
    HILOG_DEBUG("%{public}s begin.", __func__);
    if (!ability || !windowStage) {
        HILOG_ERROR("ability or windowStage is nullptr");
        return;
    }
    for (auto callback : callbacks_) {
        callback->OnWindowStageInactive(ability, windowStage);
    }
}

void ApplicationContext::DispatchOnAbilityDestroy(const std::shared_ptr<NativeReference> &ability)
{
    if (!ability) {
        HILOG_ERROR("ability is nullptr");
        return;
    }
    for (auto callback : callbacks_) {
        callback->OnAbilityDestroy(ability);
    }
}

void ApplicationContext::DispatchOnAbilityForeground(const std::shared_ptr<NativeReference> &ability)
{
    if (!ability) {
        HILOG_ERROR("ability is nullptr");
        return;
    }
    for (auto callback : callbacks_) {
        callback->OnAbilityForeground(ability);
    }
}

void ApplicationContext::DispatchOnAbilityBackground(const std::shared_ptr<NativeReference> &ability)
{
    if (!ability) {
        HILOG_ERROR("ability is nullptr");
        return;
    }
    for (auto callback : callbacks_) {
        callback->OnAbilityBackground(ability);
    }
}

void ApplicationContext::DispatchOnAbilityContinue(const std::shared_ptr<NativeReference> &ability)
{
    if (!ability) {
        HILOG_ERROR("ability is nullptr");
        return;
    }
    for (auto callback : callbacks_) {
        callback->OnAbilityContinue(ability);
    }
}

void ApplicationContext::DispatchConfigurationUpdated(const AppExecFwk::Configuration &config)
{
    for (auto envCallback : envCallbacks_) {
        envCallback->OnConfigurationUpdated(config);
    }
}

std::string ApplicationContext::GetBundleName() const
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetBundleName() : "";
}

std::shared_ptr<Context> ApplicationContext::CreateBundleContext(const std::string &bundleName)
{
    return (contextImpl_ != nullptr) ? contextImpl_->CreateBundleContext(bundleName) : nullptr;
}

std::shared_ptr<Context> ApplicationContext::CreateModuleContext(const std::string &moduleName)
{
    return contextImpl_ ? contextImpl_->CreateModuleContext(moduleName) : nullptr;
}

std::shared_ptr<Context> ApplicationContext::CreateModuleContext(const std::string &bundleName,
                                                                 const std::string &moduleName)
{
    return contextImpl_ ? contextImpl_->CreateModuleContext(bundleName, moduleName) : nullptr;
}

std::shared_ptr<AppExecFwk::ApplicationInfo> ApplicationContext::GetApplicationInfo() const
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetApplicationInfo() : nullptr;
}

std::shared_ptr<Global::Resource::ResourceManager> ApplicationContext::GetResourceManager() const
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetResourceManager() : nullptr;
}

std::string ApplicationContext::GetBundleCodePath() const
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetBundleCodePath() : "";
}

std::shared_ptr<AppExecFwk::HapModuleInfo> ApplicationContext::GetHapModuleInfo() const
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetHapModuleInfo() : nullptr;
}

std::string ApplicationContext::GetBundleCodeDir()
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetBundleCodeDir() : "";
}

std::string ApplicationContext::GetCacheDir()
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetCacheDir() : "";
}

std::string ApplicationContext::GetTempDir()
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetTempDir() : "";
}

std::string ApplicationContext::GetFilesDir()
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetFilesDir() : "";
}

bool ApplicationContext::IsUpdatingConfigurations()
{
    return (contextImpl_ != nullptr) ? contextImpl_->IsUpdatingConfigurations() : false;
}

bool ApplicationContext::PrintDrawnCompleted()
{
    return (contextImpl_ != nullptr) ? contextImpl_->PrintDrawnCompleted() : false;
}

std::string ApplicationContext::GetDatabaseDir()
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetDatabaseDir() : "";
}

std::string ApplicationContext::GetPreferencesDir()
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetPreferencesDir() : "";
}

std::string ApplicationContext::GetDistributedFilesDir()
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetDistributedFilesDir() : "";
}

sptr<IRemoteObject> ApplicationContext::GetToken()
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetToken() : nullptr;
}

void ApplicationContext::SetToken(const sptr<IRemoteObject> &token)
{
    if (contextImpl_ != nullptr) {
        contextImpl_->SetToken(token);
    }
}

void ApplicationContext::SwitchArea(int mode)
{
    if (contextImpl_ != nullptr) {
        contextImpl_->SwitchArea(mode);
    }
}

int ApplicationContext::GetArea()
{
    if (contextImpl_ == nullptr) {
        HILOG_ERROR("AbilityContext::contextImpl is nullptr.");
        return ContextImpl::EL_DEFAULT;
    }
    return contextImpl_->GetArea();
}

std::shared_ptr<AppExecFwk::Configuration> ApplicationContext::GetConfiguration() const
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetConfiguration() : nullptr;
}

std::string ApplicationContext::GetBaseDir() const
{
    return (contextImpl_ != nullptr) ? contextImpl_->GetBaseDir() : nullptr;
}
}  // namespace AbilityRuntime
}  // namespace OHOS