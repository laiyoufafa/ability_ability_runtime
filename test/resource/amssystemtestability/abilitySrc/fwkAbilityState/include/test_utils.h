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
#ifndef RESOURCE_OHOS_ABILITY_RUNTIME_TEST_UTILS_H_
#define RESOURCE_OHOS_ABILITY_RUNTIME_TEST_UTILS_H_
#include "want.h"

namespace OHOS {
namespace AppExecFwk {
class TestUtils {
public:
    TestUtils() = default;
    virtual ~TestUtils() = default;
    static bool PublishEvent(const std::string &eventName, const int &code, const std::string &data);
    static std::vector<std::string> split(const std::string &in, const std::string &delim);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // RESOURCE_OHOS_ABILITY_RUNTIME_TEST_UTILS_H_
