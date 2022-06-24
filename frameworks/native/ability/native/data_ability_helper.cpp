/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "data_ability_helper.h"
#include "hilog_wrapper.h"
#include "hitrace_meter.h"

namespace OHOS {
namespace AppExecFwk {
DataAbilityHelper::DataAbilityHelper(const std::shared_ptr<DataAbilityHelperImpl> &helperImpl)
{
    dataAbilityHelperImpl_ = helperImpl;
}

/**
 * @brief Creates a DataAbilityHelper instance without specifying the Uri based on the given Context.
 *
 * @param context Indicates the Context object on OHOS.
 *
 * @return Returns the created DataAbilityHelper instance where Uri is not specified.
 */
std::shared_ptr<DataAbilityHelper> DataAbilityHelper::Creator(const std::shared_ptr<Context> &context)
{
    HILOG_INFO("Creator with context called.");
    DataAbilityHelper *ptrDataAbilityHelper = nullptr;
    std::shared_ptr<DataAbilityHelperImpl> dataAbilityHelperImpl = DataAbilityHelperImpl::Creator(context);
    if (dataAbilityHelperImpl) {
        ptrDataAbilityHelper = new (std::nothrow) DataAbilityHelper(dataAbilityHelperImpl);
    }
    if (ptrDataAbilityHelper == nullptr) {
        HILOG_ERROR("Create DataAbilityHelper failed.");
        return nullptr;
    }
    return std::shared_ptr<DataAbilityHelper>(ptrDataAbilityHelper);
}

/**
 * @brief Creates a DataAbilityHelper instance with the Uri specified based on the given Context.
 *
 * @param context Indicates the Context object on OHOS.
 * @param uri Indicates the database table or disk file to operate.
 *
 * @return Returns the created DataAbilityHelper instance with a specified Uri.
 */
std::shared_ptr<DataAbilityHelper> DataAbilityHelper::Creator(
    const std::shared_ptr<Context> &context, const std::shared_ptr<Uri> &uri)
{
    HILOG_INFO("Creator with context & uri called.");
    return DataAbilityHelper::Creator(context, uri, false);
}

/**
 * @brief Creates a DataAbilityHelper instance with the Uri specified based on the given Context.
 *
 * @param context Indicates the Context object on OHOS.
 * @param uri Indicates the database table or disk file to operate.
 *
 * @return Returns the created DataAbilityHelper instance with a specified Uri.
 */
std::shared_ptr<DataAbilityHelper> DataAbilityHelper::Creator(
    const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, const std::shared_ptr<Uri> &uri)
{
    HILOG_INFO("Creator with ability runtime context & uri called.");
    return DataAbilityHelper::Creator(context, uri, false);
}

/**
 * @brief You can use this method to specify the Uri of the data to operate and set the binding relationship
 * between the ability using the Data template (Data ability for short) and the associated client process in
 * a DataAbilityHelper instance.
 *
 * @param context Indicates the Context object on OHOS.
 * @param uri Indicates the database table or disk file to operate.
 * @param tryBind Specifies whether the exit of the corresponding Data ability process causes the exit of the
 * client process.
 *
 * @return Returns the created DataAbilityHelper instance.
 */
std::shared_ptr<DataAbilityHelper> DataAbilityHelper::Creator(
    const std::shared_ptr<Context> &context, const std::shared_ptr<Uri> &uri, const bool tryBind)
{
    HILOG_INFO("Creator with context & uri & tryBind called.");
    DataAbilityHelper *ptrDataAbilityHelper = nullptr;
    auto dataAbilityHelperImpl = DataAbilityHelperImpl::Creator(context, uri, tryBind);
    if (dataAbilityHelperImpl) {
        ptrDataAbilityHelper = new (std::nothrow) DataAbilityHelper(dataAbilityHelperImpl);
    }
    if (ptrDataAbilityHelper == nullptr) {
        HILOG_ERROR("Create DataAbilityHelper failed.");
        return nullptr;
    }
    return std::shared_ptr<DataAbilityHelper>(ptrDataAbilityHelper);
}

/**
 * @brief You can use this method to specify the Uri of the data to operate and set the binding relationship
 * between the ability using the Data template (Data ability for short) and the associated client process in
 * a DataAbilityHelper instance.
 *
 * @param context Indicates the Context object on OHOS.
 * @param uri Indicates the database table or disk file to operate.
 * @param tryBind Specifies whether the exit of the corresponding Data ability process causes the exit of the
 * client process.
 *
 * @return Returns the created DataAbilityHelper instance.
 */
std::shared_ptr<DataAbilityHelper> DataAbilityHelper::Creator(
    const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, const std::shared_ptr<Uri> &uri, const bool tryBind)
{
    HILOG_INFO("Creator with ability runtime context & uri & tryBind called.");
    DataAbilityHelper *ptrDataAbilityHelper = nullptr;
    auto dataAbilityHelperImpl = DataAbilityHelperImpl::Creator(context, uri, tryBind);
    if (dataAbilityHelperImpl) {
        ptrDataAbilityHelper = new (std::nothrow) DataAbilityHelper(dataAbilityHelperImpl);
    }
    if (ptrDataAbilityHelper == nullptr) {
        HILOG_ERROR("Create DataAbilityHelper failed.");
        return nullptr;
    }
    return std::shared_ptr<DataAbilityHelper>(ptrDataAbilityHelper);
}

/**
 * @brief Creates a DataAbilityHelper instance without specifying the Uri based.
 *
 * @param token Indicates the System token.
 *
 * @return Returns the created DataAbilityHelper instance where Uri is not specified.
 */
std::shared_ptr<DataAbilityHelper> DataAbilityHelper::Creator(const sptr<IRemoteObject> &token)
{
    HILOG_INFO("Creator with token called.");
    DataAbilityHelper *ptrDataAbilityHelper = nullptr;
    auto dataAbilityHelperImpl = DataAbilityHelperImpl::Creator(token);
    if (dataAbilityHelperImpl) {
        ptrDataAbilityHelper = new (std::nothrow) DataAbilityHelper(dataAbilityHelperImpl);
    }
    if (ptrDataAbilityHelper == nullptr) {
        HILOG_ERROR("Create DataAbilityHelper failed.");
        return nullptr;
    }
    return std::shared_ptr<DataAbilityHelper>(ptrDataAbilityHelper);
}

/**
 * @brief You can use this method to specify the Uri of the data to operate and set the binding relationship
 * between the ability using the Data template (Data ability for short) and the associated client process in
 * a DataAbilityHelper instance.
 *
 * @param token Indicates the System token.
 * @param uri Indicates the database table or disk file to operate.
 *
 * @return Returns the created DataAbilityHelper instance.
 */
std::shared_ptr<DataAbilityHelper> DataAbilityHelper::Creator(
    const sptr<IRemoteObject> &token, const std::shared_ptr<Uri> &uri)
{
    HILOG_INFO("Creator with token & uri called.");
    DataAbilityHelper *ptrDataAbilityHelper = nullptr;
    auto dataAbilityHelperImpl = DataAbilityHelperImpl::Creator(token, uri);
    if (dataAbilityHelperImpl) {
        ptrDataAbilityHelper = new (std::nothrow) DataAbilityHelper(dataAbilityHelperImpl);
    }
    if (ptrDataAbilityHelper == nullptr) {
        HILOG_ERROR("Create DataAbilityHelper failed.");
        return nullptr;
    }
    return std::shared_ptr<DataAbilityHelper>(ptrDataAbilityHelper);
}

/**
 * @brief Releases the client resource of the Data ability.
 * You should call this method to releases client resource after the data operations are complete.
 *
 * @return Returns true if the resource is successfully released; returns false otherwise.
 */
bool DataAbilityHelper::Release()
{
    HILOG_INFO("Release called.");
    bool ret = false;
    if (dataAbilityHelperImpl_) {
        ret = dataAbilityHelperImpl_->Release();
        dataAbilityHelperImpl_.reset();
    }
    return ret;
}

/**
 * @brief Obtains the MIME types of files supported.
 *
 * @param uri Indicates the path of the files to obtain.
 * @param mimeTypeFilter Indicates the MIME types of the files to obtain. This parameter cannot be null.
 *
 * @return Returns the matched MIME types. If there is no match, null is returned.
 */
std::vector<std::string> DataAbilityHelper::GetFileTypes(Uri &uri, const std::string &mimeTypeFilter)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("GetFileTypes called.");
    std::vector<std::string> matchedMIMEs;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->GetFileTypes(uri, mimeTypeFilter);
    }
    return matchedMIMEs;
}

/**
 * @brief Opens a file in a specified remote path.
 *
 * @param uri Indicates the path of the file to open.
 * @param mode Indicates the file open mode, which can be "r" for read-only access, "w" for write-only access
 * (erasing whatever data is currently in the file), "wt" for write access that truncates any existing file,
 * "wa" for write-only access to append to any existing data, "rw" for read and write access on any existing data,
 *  or "rwt" for read and write access that truncates any existing file.
 *
 * @return Returns the file descriptor.
 */
int DataAbilityHelper::OpenFile(Uri &uri, const std::string &mode)
{
    HILOG_INFO("OpenFile Called.");
    int fd = -1;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->OpenFile(uri, mode);
    }
    return fd;
}

/**
 * @brief This is like openFile, open a file that need to be able to return sub-sections of files，often assets
 * inside of their .hap.
 *
 * @param uri Indicates the path of the file to open.
 * @param mode Indicates the file open mode, which can be "r" for read-only access, "w" for write-only access
 * (erasing whatever data is currently in the file), "wt" for write access that truncates any existing file,
 * "wa" for write-only access to append to any existing data, "rw" for read and write access on any existing
 * data, or "rwt" for read and write access that truncates any existing file.
 *
 * @return Returns the RawFileDescriptor object containing file descriptor.
 */
int DataAbilityHelper::OpenRawFile(Uri &uri, const std::string &mode)
{
    HILOG_INFO("OpenRawFile called.");
    int fd = -1;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->OpenRawFile(uri, mode);
    }
    return fd;
}

/**
 * @brief Inserts a single data record into the database.
 *
 * @param uri Indicates the path of the data to operate.
 * @param value Indicates the data record to insert. If this parameter is null, a blank row will be inserted.
 *
 * @return Returns the index of the inserted data record.
 */
int DataAbilityHelper::Insert(Uri &uri, const NativeRdb::ValuesBucket &value)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("Insert called.");
    int index = -1;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->Insert(uri, value);
    }
    return index;
}

std::shared_ptr<AppExecFwk::PacMap> DataAbilityHelper::Call(
    const Uri &uri, const std::string &method, const std::string &arg, const AppExecFwk::PacMap &pacMap)
{
    std::shared_ptr<AppExecFwk::PacMap> result = nullptr;
    HILOG_INFO("Call called.");
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->Call(uri, method, arg, pacMap);
    }
    return result;
}

/**
 * @brief Updates data records in the database.
 *
 * @param uri Indicates the path of data to update.
 * @param value Indicates the data to update. This parameter can be null.
 * @param predicates Indicates filter criteria. You should define the processing logic when this parameter is null.
 *
 * @return Returns the number of data records updated.
 */
int DataAbilityHelper::Update(
    Uri &uri, const NativeRdb::ValuesBucket &value, const NativeRdb::DataAbilityPredicates &predicates)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("Update called.");
    int index = -1;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->Update(uri, value, predicates);
    }
    return index;
}

/**
 * @brief Deletes one or more data records from the database.
 *
 * @param uri Indicates the path of the data to operate.
 * @param predicates Indicates filter criteria. You should define the processing logic when this parameter is null.
 *
 * @return Returns the number of data records deleted.
 */
int DataAbilityHelper::Delete(Uri &uri, const NativeRdb::DataAbilityPredicates &predicates)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("Delete called.");
    int index = -1;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->Delete(uri, predicates);
    }
    return index;
}

/**
 * @brief Deletes one or more data records from the database.
 *
 * @param uri Indicates the path of data to query.
 * @param columns Indicates the columns to query. If this parameter is null, all columns are queried.
 * @param predicates Indicates filter criteria. You should define the processing logic when this parameter is null.
 *
 * @return Returns the query result.
 */
std::shared_ptr<NativeRdb::AbsSharedResultSet> DataAbilityHelper::Query(
    Uri &uri, std::vector<std::string> &columns, const NativeRdb::DataAbilityPredicates &predicates)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("Query called.");
    std::shared_ptr<NativeRdb::AbsSharedResultSet> resultset = nullptr;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->Query(uri, columns, predicates);
    }
    return resultset;
}

/**
 * @brief Obtains the MIME type matching the data specified by the URI of the Data ability. This method should be
 * implemented by a Data ability. Data abilities supports general data types, including text, HTML, and JPEG.
 *
 * @param uri Indicates the URI of the data.
 *
 * @return Returns the MIME type that matches the data specified by uri.
 */
std::string DataAbilityHelper::GetType(Uri &uri)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("GetType called.");
    std::string type;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->GetType(uri);
    }
    return type;
}

/**
 * @brief Reloads data in the database.
 *
 * @param uri Indicates the position where the data is to reload. This parameter is mandatory.
 * @param extras Indicates the PacMap object containing the additional parameters to be passed in this call. This
 * parameter can be null. If a custom Sequenceable object is put in the PacMap object and will be transferred across
 * processes, you must call BasePacMap.setClassLoader(ClassLoader) to set a class loader for the custom object.
 *
 * @return Returns true if the data is successfully reloaded; returns false otherwise.
 */
bool DataAbilityHelper::Reload(Uri &uri, const PacMap &extras)
{
    HILOG_INFO("Reload called.");
    bool ret = false;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->Reload(uri, extras);
    }
    return ret;
}

/**
 * @brief Inserts multiple data records into the database.
 *
 * @param uri Indicates the path of the data to operate.
 * @param values Indicates the data records to insert.
 *
 * @return Returns the number of data records inserted.
 */
int DataAbilityHelper::BatchInsert(Uri &uri, const std::vector<NativeRdb::ValuesBucket> &values)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("BatchInsert called.");
    int ret = -1;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->BatchInsert(uri, values);
    }
    return ret;
}

/**
 * @brief Registers an observer to DataObsMgr specified by the given Uri.
 *
 * @param uri, Indicates the path of the data to operate.
 * @param dataObserver, Indicates the IDataAbilityObserver object.
 */
void DataAbilityHelper::RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    HILOG_INFO("RegisterObserver called.");
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->RegisterObserver(uri, dataObserver);
    }
}

/**
 * @brief Deregisters an observer used for DataObsMgr specified by the given Uri.
 *
 * @param uri, Indicates the path of the data to operate.
 * @param dataObserver, Indicates the IDataAbilityObserver object.
 */
void DataAbilityHelper::UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    HILOG_INFO("UnregisterObserver called.");
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->UnregisterObserver(uri, dataObserver);
    }
}

/**
 * @brief Notifies the registered observers of a change to the data resource specified by Uri.
 *
 * @param uri, Indicates the path of the data to operate.
 */
void DataAbilityHelper::NotifyChange(const Uri &uri)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("NotifyChange called.");
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->NotifyChange(uri);
    }
}

/**
 * @brief Converts the given uri that refer to the Data ability into a normalized URI. A normalized URI can be used
 * across devices, persisted, backed up, and restored. It can refer to the same item in the Data ability even if the
 * context has changed. If you implement URI normalization for a Data ability, you must also implement
 * denormalizeUri(ohos.utils.net.Uri) to enable URI denormalization. After this feature is enabled, URIs passed to any
 * method that is called on the Data ability must require normalization verification and denormalization. The default
 * implementation of this method returns null, indicating that this Data ability does not support URI normalization.
 *
 * @param uri Indicates the Uri object to normalize.
 *
 * @return Returns the normalized Uri object if the Data ability supports URI normalization; returns null otherwise.
 */
Uri DataAbilityHelper::NormalizeUri(Uri &uri)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("NormalizeUri called.");
    Uri urivalue("");
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->NormalizeUri(uri);
    }
    return urivalue;
}

/**
 * @brief Converts the given normalized uri generated by normalizeUri(ohos.utils.net.Uri) into a denormalized one.
 * The default implementation of this method returns the original URI passed to it.
 *
 * @param uri uri Indicates the Uri object to denormalize.
 *
 * @return Returns the denormalized Uri object if the denormalization is successful; returns the original Uri passed to
 * this method if there is nothing to do; returns null if the data identified by the original Uri cannot be found in the
 * current environment.
 */
Uri DataAbilityHelper::DenormalizeUri(Uri &uri)
{
    HITRACE_METER_NAME(HITRACE_TAG_DISTRIBUTEDDATA, __PRETTY_FUNCTION__);
    HILOG_INFO("DenormalizeUri called.");
    Uri urivalue("");
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->DenormalizeUri(uri);
    }
    return urivalue;
}

std::vector<std::shared_ptr<DataAbilityResult>> DataAbilityHelper::ExecuteBatch(
    const Uri &uri, const std::vector<std::shared_ptr<DataAbilityOperation>> &operations)
{
    HILOG_INFO("ExecuteBatch called.");
    std::vector<std::shared_ptr<DataAbilityResult>> results;
    if (dataAbilityHelperImpl_) {
        return dataAbilityHelperImpl_->ExecuteBatch(uri, operations);
    }
    return results;
}
}  // namespace AppExecFwk
}  // namespace OHOS

