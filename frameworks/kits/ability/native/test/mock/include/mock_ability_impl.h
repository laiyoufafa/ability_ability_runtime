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

#ifndef FOUNDATION_APPEXECFWK_OHOS_MOCK_INHERITANCE_IMPL_H
#define FOUNDATION_APPEXECFWK_OHOS_MOCK_INHERITANCE_IMPL_H

#include "ability.h"
#include "ability_impl.h"
#include <gtest/gtest.h>

namespace OHOS {
namespace AppExecFwk {
using Want = OHOS::AAFwk::Want;

class MockAbilityimpl : public AbilityImpl {
public:
    MockAbilityimpl() = default;
    virtual ~MockAbilityimpl() = default;

    void ImplStart(const Want &want)
    {
        this->Start(want);
    }

    void ImplStop()
    {
        this->Stop();
    }

    void ImplActive()
    {
        this->Active();
    }

    void ImplInactive()
    {
        this->Inactive();
    }

#ifdef SUPPORT_GRAPHICS
    void ImplForeground(const Want &want)
    {
        this->Foreground(want);
    }

    void ImplBackground()
    {
        this->Background();
    }
#endif

    void SetlifecycleState(int state)
    {
        this->lifecycleState_ = state;
    }

    int MockGetCurrentState()
    {
        int value;
        value = GetCurrentState();
        return value;
    }

    sptr<IRemoteObject> GetToken()
    {
        return token_;
    }

    std::shared_ptr<Ability> GetAbility()
    {
        return ability_;
    }

    bool CheckAndSave()
    {
        return AbilityImpl::CheckAndSave();
    }

    bool CheckAndRestore()
    {
        return AbilityImpl::CheckAndRestore();
    }

private:
    AbilityImpl AbilityImpl_;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_OHOS_MOCK_PAGE_ABILITY_H