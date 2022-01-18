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

#include "native_engine/native_engine.h"
#include "event_handler.h"

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_MANAGER_H
#define OHOS_ABILITY_RUNTIME_ABILITY_MANAGER_H

namespace OHOS {
namespace AbilityRuntime {
NativeValue* JsAbilityManagerInit(NativeEngine* engine, NativeValue* exportObj);
}  // namespace AbilityRuntime
}  // namespace OHOS
#endif // OHOS_ABILITY_RUNTIME_ABILITY_MANAGER_H