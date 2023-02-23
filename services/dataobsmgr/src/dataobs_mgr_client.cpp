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

#include "dataobs_mgr_client.h"

#include "hilog_wrapper.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace AAFwk {
std::shared_ptr<DataObsMgrClient> DataObsMgrClient::instance_ = nullptr;
std::mutex DataObsMgrClient::mutex_;

std::shared_ptr<DataObsMgrClient> DataObsMgrClient::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock_l(mutex_);
        if (instance_ == nullptr) {
            instance_ = std::make_shared<DataObsMgrClient>();
        }
    }
    return instance_;
}

DataObsMgrClient::DataObsMgrClient()
{}

DataObsMgrClient::~DataObsMgrClient()
{}

/**
 * Registers an observer to DataObsMgr specified by the given Uri.
 *
 * @param uri, Indicates the path of the data to operate.
 * @param dataObserver, Indicates the IDataAbilityObserver object.
 *
 * @return Returns ERR_OK on success, others on failure.
 */
ErrCode DataObsMgrClient::RegisterObserver(const Uri &uri, const sptr<IDataAbilityObserver> &dataObserver)
{
    if (Connect() != SUCCESS) {
        return DATAOBS_SERVICE_NOT_CONNECTED;
    }
    sptr<IDataObsMgr> dataObsManger = iface_cast<IDataObsMgr>(remoteObject_);
    return dataObsManger->RegisterObserver(uri, dataObserver);
}

/**
 * Deregisters an observer used for DataObsMgr specified by the given Uri.
 *
 * @param uri, Indicates the path of the data to operate.
 * @param dataObserver, Indicates the IDataAbilityObserver object.
 *
 * @return Returns ERR_OK on success, others on failure.
 */
ErrCode DataObsMgrClient::UnregisterObserver(const Uri &uri, const sptr<IDataAbilityObserver> &dataObserver)
{
    if (Connect() != SUCCESS) {
        return DATAOBS_SERVICE_NOT_CONNECTED;
    }
    sptr<IDataObsMgr> dataObsManger = iface_cast<IDataObsMgr>(remoteObject_);
    return dataObsManger->UnregisterObserver(uri, dataObserver);
}

/**
 * Notifies the registered observers of a change to the data resource specified by Uri.
 *
 * @param uri, Indicates the path of the data to operate.
 *
 * @return Returns ERR_OK on success, others on failure.
 */
ErrCode DataObsMgrClient::NotifyChange(const Uri &uri)
{
    if (Connect() != SUCCESS) {
        return DATAOBS_SERVICE_NOT_CONNECTED;
    }
    sptr<IDataObsMgr> dataObsManger = iface_cast<IDataObsMgr>(remoteObject_);
    return dataObsManger->NotifyChange(uri);
}

/**
 * Connect dataobs manager service.
 *
 * @return Returns SUCCESS on success, others on failure.
 */
Status DataObsMgrClient::Connect()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (remoteObject_ != nullptr) {
        return SUCCESS;
    }
    sptr<ISystemAbilityManager> systemManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemManager == nullptr) {
        HILOG_ERROR("%{private}s:fail to get Registry", __func__);
        return GET_DATAOBS_SERVICE_FAILED;
    }
    remoteObject_ = systemManager->GetSystemAbility(DATAOBS_MGR_SERVICE_SA_ID);
    if (remoteObject_ == nullptr) {
        HILOG_ERROR("%{private}s:fail to connect DataObsMgrService", __func__);
        return GET_DATAOBS_SERVICE_FAILED;
    }
    return SUCCESS;
}

Status DataObsMgrClient::RegisterObserverExt(const Uri &uri, const sptr<IDataAbilityObserver> &dataObserver,
    bool isDescendants)
{
    if (Connect() != SUCCESS) {
        return DATAOBS_SERVICE_NOT_CONNECTED;
    }
    sptr<IDataObsMgr> dataObsManger = iface_cast<IDataObsMgr>(remoteObject_);
    return dataObsManger->RegisterObserverExt(uri, dataObserver, isDescendants);
}

Status DataObsMgrClient::UnregisterObserverExt(const sptr<IDataAbilityObserver> &dataObserver)
{
    if (Connect() != SUCCESS) {
        return DATAOBS_SERVICE_NOT_CONNECTED;
    }
    sptr<IDataObsMgr> dataObsManger = iface_cast<IDataObsMgr>(remoteObject_);
    return dataObsManger->UnregisterObserverExt(dataObserver);
}

Status DataObsMgrClient::NotifyChangeExt(const std::list<Uri> &uris)
{
    if (Connect() != SUCCESS) {
        return DATAOBS_SERVICE_NOT_CONNECTED;
    }
    sptr<IDataObsMgr> dataObsManger = iface_cast<IDataObsMgr>(remoteObject_);
    return dataObsManger->NotifyChangeExt(uris);
}
}  // namespace AAFwk
}  // namespace OHOS
