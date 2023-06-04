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

#ifndef OHOS_ABILITY_RUNTIME_FAULT_DATA_H
#define OHOS_ABILITY_RUNTIME_FAULT_DATA_H

#include <string>

#include "ierror_observer.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {

/**
 * @enum FaultDataType
 * FaultDataType defines the type of FaultData.
 */
enum class FaultDataType {
    UNKNOWN = -1,
    CPP_CRASH,
    JS_ERROR,
    APP_FREEZE,
    PERFORMANCE_CONTROL,
    RESOURCE_CONTROL
};

/**
 * @struct FaultData
 * FaultData is used to save information about faultdata.
 */
struct FaultData : public Parcelable {
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static FaultData *Unmarshalling(Parcel &parcel);
    // error object
    ErrorObject errorObject;
    FaultDataType faultType = FaultDataType::UNKNOWN;
};

/**
 * @struct AppFaultDataBySA
 * AppFaultDataBySA is used to save information about faultdata notified by SA.
 */
struct AppFaultDataBySA : public Parcelable {
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static AppFaultDataBySA *Unmarshalling(Parcel &parcel);
    // error object
    ErrorObject errorObject;
    FaultDataType faultType = FaultDataType::UNKNOWN;
    int32_t pid = -1;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_FAULT_DATA_H