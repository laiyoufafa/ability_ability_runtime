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

#ifndef OHOS_ABILITY_RUNTIME_JS_UTILS_H
#define OHOS_ABILITY_RUNTIME_JS_UTILS_H

#include "hilog_wrapper.h"

namespace OHOS {
namespace AbilityRuntime {
#define CHECK_POINTER(object)                   \
    do {                                        \
        if ((object) == nullptr) {              \
            HILOG_ERROR("pointer is nullptr."); \
            return;                             \
        }                                       \
    } while (0)

#define CHECK_POINTER_AND_RETURN(object, value) \
    do {                                        \
        if ((object) == nullptr) {              \
            HILOG_ERROR("pointer is nullptr."); \
            return value;                       \
        }                                       \
    } while (0)
}  // namespace AbilityRuntime
}  // namespace OHOS

#endif  // OHOS_ABILITY_RUNTIME_JS_UTILS_H
