/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <chrono>

#include "ability_manager_service.h"
#include "ability_manager_errors.h"
#include "ability_event_handler.h"
#include "free_install_observer_manager.h"
#include "free_install_observer_interface.h"
#include "hilog_wrapper.h"

namespace OHOS {
namespace AAFwk {
FreeInstallObserverManager::FreeInstallObserverManager()
{}

FreeInstallObserverManager::~FreeInstallObserverManager()
{}

int32_t FreeInstallObserverManager::AddObserver(const sptr<IFreeInstallObserver> &observer)
{
    HILOG_DEBUG("AddObserver begin.");
    if (observer == nullptr) {
        HILOG_ERROR("the observer is nullptr.");
        return ERR_INVALID_VALUE;
    }
    if (ObserverExist(observer)) {
        HILOG_ERROR("Observer exist.");
        return ERR_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> lock(observerLock_);
    observerList_.emplace_back(observer);
    HILOG_DEBUG("observerList_ size:%{public}zu", observerList_.size());
    AddObserverDeathRecipient(observer);
    return ERR_OK;
}

int32_t FreeInstallObserverManager::RemoveObserver(const sptr<IFreeInstallObserver> &observer)
{
    HILOG_DEBUG("RemoveObserver begin.");
    if (observer == nullptr) {
        HILOG_ERROR("the observer is nullptr.");
        return ERR_INVALID_VALUE;
    }
    std::lock_guard<std::mutex> lock(observerLock_);
    auto it = std::find_if(observerList_.begin(), observerList_.end(),
        [&observer](const sptr<IFreeInstallObserver> &item) {
        return (item && item->AsObject() == observer->AsObject());
    });
    if (it != observerList_.end()) {
        observerList_.erase(it);
        HILOG_INFO("observerList_ size:%{public}zu", observerList_.size());
        RemoveObserverDeathRecipient(observer);
        return ERR_OK;
    }
    HILOG_ERROR("Observer not exist or has been removed.");
    return ERR_INVALID_VALUE;
}

void FreeInstallObserverManager::OnInstallFinished(const std::string bundleName, const std::string abilityName,
    const std::string startTime, int resultCode)
{
    auto task = [weak = weak_from_this(), bundleName, abilityName, startTime, resultCode]() {
        auto self = weak.lock();
        if (self == nullptr) {
            HILOG_ERROR("self is nullptr, OnInstallFinished failed.");
            return;
        }
        HILOG_INFO("OnInstallFinished come.");
        self->HandleOnInstallFinished(bundleName, abilityName, startTime, resultCode);
    };

    std::shared_ptr<AbilityEventHandler> handler =
        DelayedSingleton<AbilityManagerService>::GetInstance()->GetEventHandler();
    CHECK_POINTER_LOG(handler, "Fail to get AbilityEventHandler.");
    handler->PostTask(task);
}

void FreeInstallObserverManager::HandleOnInstallFinished(const std::string bundleName, const std::string abilityName,
    const std::string startTime, int resultCode)
{
    HILOG_DEBUG("HandleOnInstallFinished begin.");
    for (auto it = observerList_.begin(); it != observerList_.end(); ++it) {
        (*it)->OnInstallFinished(bundleName, abilityName, startTime, resultCode);
    }
}

bool FreeInstallObserverManager::ObserverExist(const sptr<IFreeInstallObserver> &observer)
{
    HILOG_DEBUG("ObserExist begin.");
    if (observer == nullptr) {
        HILOG_ERROR("The param observer is nullptr.");
        return false;
    }
    std::lock_guard<std::mutex> lock(observerLock_);
    auto it = std::find_if(observerList_.begin(), observerList_.end(),
        [&observer](const sptr<IFreeInstallObserver> &item) {
        return (item && item->AsObject() == observer->AsObject());
    });
    if (it != observerList_.end()) {
        return true;
    }
    return false;
}

void FreeInstallObserverManager::AddObserverDeathRecipient(const sptr<IFreeInstallObserver> &observer)
{
    HILOG_INFO("Add observer death recipient come.");
    if (observer == nullptr || observer->AsObject() == nullptr) {
        HILOG_ERROR("The param observer is nullptr.");
        return;
    }
    auto it = recipientMap_.find(observer->AsObject());
    if (it != recipientMap_.end()) {
        HILOG_ERROR("This death recipient has been added.");
        return;
    } else {
        std::weak_ptr<FreeInstallObserverManager> thisWeakPtr(shared_from_this());
        // add death recipient
        sptr<IRemoteObject::DeathRecipient> deathRecipient =
            new FreeInstallObserverRecipient([thisWeakPtr](const wptr<IRemoteObject> &remote) {
                auto freeInstallObserverManager = thisWeakPtr.lock();
                if (freeInstallObserverManager) {
                    freeInstallObserverManager->OnObserverDied(remote);
                }
            });
        observer->AsObject()->AddDeathRecipient(deathRecipient);
        recipientMap_.emplace(observer->AsObject(), deathRecipient);
    }
}

void FreeInstallObserverManager::RemoveObserverDeathRecipient(const sptr<IFreeInstallObserver> &observer)
{
    HILOG_INFO("Remove observer death recipient begin.");
    if (observer == nullptr || observer->AsObject() == nullptr) {
        HILOG_ERROR("The param observer is nullptr.");
        return;
    }
    auto it = recipientMap_.find(observer->AsObject());
    if (it != recipientMap_.end()) {
        it->first->RemoveDeathRecipient(it->second);
        recipientMap_.erase(it);
        return;
    }
}

void FreeInstallObserverManager::OnObserverDied(const wptr<IRemoteObject> &remote)
{
    HILOG_INFO("OnObserverDied begin.");
    auto object = remote.promote();
    if (object == nullptr) {
        HILOG_ERROR("observer is nullptr.");
        return;
    }
    sptr<IFreeInstallObserver> observer = iface_cast<IFreeInstallObserver>(object);
    RemoveObserver(observer);
}

FreeInstallObserverRecipient::FreeInstallObserverRecipient(RemoteDiedHandler handler) : handler_(handler)
{}

FreeInstallObserverRecipient::~FreeInstallObserverRecipient()
{}

void FreeInstallObserverRecipient::OnRemoteDied(const wptr<IRemoteObject> &__attribute__((unused)) remote)
{
    HILOG_ERROR("FreeInstallObserverRecipient On remote died.");
    if (handler_) {
        handler_(remote);
    }
}
} // namespace AAFwk
} // namespace OHOS