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
#ifndef OHOS_ABILITY_RUNTIME_APP_NO_RESPONE_DISPOSER_H
#define OHOS_ABILITY_RUNTIME_APP_NO_RESPONE_DISPOSER_H

#include <string>
#include <vector>

#include "iremote_object.h"

namespace OHOS {
namespace AAFwk {
/**
 * @class AppNoResponseDisposer
 * AppNoResponseDisposer.
 */
class AppNoResponseDisposer : public std::enable_shared_from_this<AppNoResponseDisposer> {
public:
    using Closure = std::function<void()>;
    using SetMissionClosure = std::function<void(const std::vector<sptr<IRemoteObject>> &tokens)>;
    using ShowDialogClosure = std::function<void(int32_t labelId, const std::string &bundle, const Closure &callBack)>;

    explicit AppNoResponseDisposer(const int timeout);
    virtual ~AppNoResponseDisposer() = default;

#ifdef SUPPORT_GRAPHICS
    int DisposeAppNoResponse(int pid,
        const SetMissionClosure &task, const ShowDialogClosure &showDialogTask) const;
#else
    int DisposeAppNoResponse(int pid, const SetMissionClosure &task) const;
#endif

private:
    int ExcuteANRSaveStackInfoTask(int pid, const SetMissionClosure &task) const;
    int PostTimeoutTask(int pid, std::string bundleName = "") const;

private:
    int timeout_ = 0;
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_APP_NO_RESPONE_DISPOSER_H