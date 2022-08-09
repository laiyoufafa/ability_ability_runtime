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

#include "runtime_extractor.h"

#include "hilog_wrapper.h"

namespace OHOS {
namespace AbilityRuntime {
RuntimeExtractor::RuntimeExtractor(const std::string &source) : AppExecFwk::BaseExtractor(source)
{
    HILOG_DEBUG("RuntimeExtractor is created");
}

RuntimeExtractor::~RuntimeExtractor()
{
    HILOG_DEBUG("RuntimeExtractor destroyed");
}

bool RuntimeExtractor::ExtractABCFile(const std::string& srcPath, std::ostream &dest) const
{
    HILOG_DEBUG("%{public}s: hapPath is %{public}s", __func__, srcPath.c_str());
    return AppExecFwk::BaseExtractor::ExtractByName(srcPath, dest);
}
}  // namespace AbilityRuntime
}  // namespace OHOS
