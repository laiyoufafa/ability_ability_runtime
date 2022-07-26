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

#ifndef RESOURCE_OHOS_ABILITY_RUNTIME_ABILITY_STATE_MAIN_H
#define RESOURCE_OHOS_ABILITY_RUNTIME_ABILITY_STATE_MAIN_H
#include <string>
#include "ability.h"
#include "ability_loader.h"
#include "hilog_wrapper.h"
#include "common_event.h"
#include "common_event_manager.h"
#include "pac_map.h"
#include "stpageabilityevent.h"

namespace OHOS {
namespace AppExecFwk {
class FwkAbilityStateMainSubscriber;
class FwkAbilityStateMain : public Ability {
public:
    FwkAbilityStateMain()
    {
        mapAction_["StartNextAbility"] = [this](int code) { StartNextAbility(code); };
    }
    ~FwkAbilityStateMain();
    virtual void OnStart(const Want &want) override;
    virtual void OnStop() override;
    virtual void OnActive() override;
    virtual void OnInactive() override;
    virtual void OnBackground() override;
    virtual void OnForeground(const Want &want) override;
    virtual void OnNewWant(const Want &want) override;
    virtual void OnSaveAbilityState(PacMap &outState) override;
    virtual void OnRestoreAbilityState(const PacMap &inState) override;

    void Action(std::string action, int code);
    void StartNextAbility(int code);
    void DoCrash(std::string action, int code);
    void BlockAndStart(std::string action, int code);
    void OnBlockProcess(bool &bIsBlockFlag);

private:
    std::shared_ptr<FwkAbilityStateMainSubscriber> subscriber_;
    std::string callback_seq;
    std::string action;
    std::unordered_map<std::string, std::function<void(int)>> mapAction_;
    bool bIsBlockSave;
    bool bIsBlockRestore;
    void SubscribeEvent();
};

class FwkAbilityStateMainSubscriber : public EventFwk::CommonEventSubscriber {
public:
    explicit FwkAbilityStateMainSubscriber(const EventFwk::CommonEventSubscribeInfo &sp) : CommonEventSubscriber(sp)
    {
        mapAction_ = {
            {"StartNextAbility", [this](std::string action, int code) { Action(action, code); }},
            {"DoCrash", [this](std::string action, int code) { DoCrash(action, code); }},
            {"BlockAndStart", [this](std::string action, int code) { BlockAndStart(action, code); }},
        };
        mainAbility = nullptr;
    }

    void Action(std::string action, int code)
    {
        mainAbility->Action(action, code);
    }

    void DoCrash(std::string action, int code)
    {
        mainAbility->DoCrash(action, code);
    }

    void BlockAndStart(std::string action, int code)
    {
        mainAbility->BlockAndStart(action, code);
    }

    virtual void OnReceiveEvent(const EventFwk::CommonEventData &data) override;

    FwkAbilityStateMain *mainAbility;
    std::unordered_map<std::string, std::function<void(std::string, int)>> mapAction_;
    ~FwkAbilityStateMainSubscriber() = default;
};
class CrashMaker {
public:
    int CrashTry()
    {
        return this->crashData;
    };

private:
    int crashData = 0;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // RESOURCE_OHOS_ABILITY_RUNTIME_ABILITY_STATE_MAIN_H
