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

#ifndef FOUNDATION_APPEXECFWK_OHOS_FORM_ERRORS_H
#define FOUNDATION_APPEXECFWK_OHOS_FORM_ERRORS_H

#include <map>
#include <singleton.h>

#include "errors.h"

namespace OHOS {
namespace AppExecFwk {
/**
 * @class FormErrors
 * FormErrors is used to access form error message.
 */
class FormErrors final : public DelayedRefSingleton<FormErrors> {
    DECLARE_DELAYED_REF_SINGLETON(FormErrors)
public:
    DISALLOW_COPY_AND_MOVE(FormErrors);

    /**
     * @brief Get the error message content.
     *
     * @param errCode Error code.
     * @return Message content.
     */
    std::string GetErrorMessage(int errCode);

private:
    /**
    * @brief Init error message map object.
    *
    */
    void InitErrorMessageMap();

private:
    std::map<ErrCode, std::string> errorMessageMap_;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_OHOS_FORM_ERRORS_H