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

#ifndef ABILITY_RUNTIME_JS_APPLICATION_CONTEXT_ENVIRONMENT_CALLBACK_H
#define ABILITY_RUNTIME_JS_APPLICATION_CONTEXT_ENVIRONMENT_CALLBACK_H

#include <map>
#include <memory>

#include "configuration.h"

class NativeEngine;
class NativeValue;
class NativeReference;
struct NativeCallbackInfo;

namespace OHOS {
namespace AbilityRuntime {
class EnvironmentCallback {
public:
    virtual ~EnvironmentCallback() {}
    /**
     * Called when the system configuration is updated.
     *
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     * @param config: Indicates the updated configuration.
     * @StageModelOnly
     */
    virtual void OnConfigurationUpdated(const AppExecFwk::Configuration &config) = 0;
};

class JsEnvironmentCallback : public EnvironmentCallback,
                                   public std::enable_shared_from_this<JsEnvironmentCallback> {
public:
    explicit JsEnvironmentCallback(NativeEngine* engine);
    void OnConfigurationUpdated(const AppExecFwk::Configuration &config) override;
    int32_t Register(NativeValue *jsCallback);
    bool UnRegister(int32_t callbackId);
    bool IsEmpty();
    static int32_t serialNumber_;

private:
    NativeEngine* engine_;
    std::shared_ptr<NativeReference> jsCallback_;
    std::map<int32_t, std::shared_ptr<NativeReference>> callbacks_;
    void CallJsMethod(const std::string &methodName, const AppExecFwk::Configuration &config);
    void CallJsMethodInner(const std::string &methodName, const AppExecFwk::Configuration &config);
};
}  // namespace AbilityRuntime
}  // namespace OHOS
#endif  // ABILITY_RUNTIME_JS_APPLICATION_CONTEXT_ENVIRONMENT_CALLBACK_H
