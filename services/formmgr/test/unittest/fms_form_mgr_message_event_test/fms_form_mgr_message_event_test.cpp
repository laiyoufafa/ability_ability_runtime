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
#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "form_ams_helper.h"
#include "form_bms_helper.h"
#include "form_data_mgr.h"
#include "form_db_cache.h"
#include "form_host_interface.h"
#define private public
#include "form_mgr.h"
#undef private
#include "form_mgr_errors.h"
#include "form_mgr_service.h"
#include "if_system_ability_manager.h"
#include "inner_bundle_info.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"

#include "mock_ability_manager.h"
#include "mock_bundle_manager.h"
#include "mock_form_host_client.h"
#include "power_mgr_client.h"
#include "running_process_info.h"
#include "system_ability_definition.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::AppExecFwk;
using namespace OHOS::Security;

namespace {
const std::string PERMISSION_NAME_REQUIRE_FORM = "ohos.permission.REQUIRE_FORM";
const std::string PARAM_PROVIDER_PACKAGE_NAME = "com.form.provider.app.test.abiliy";
const std::string FORM_PROVIDER_BUNDLE_NAME = "com.form.provider.service";
const std::string PARAM_PROVIDER_MODULE_NAME = "com.form.provider.app.test.abiliy";
const std::string FORM_PROVIDER_ABILITY_NAME = "com.form.provider.app.test.abiliy";
const std::string PARAM_FORM_NAME = "com.form.name.test";

const std::string FORM_JS_COMPOMENT_NAME = "jsComponentName";
const std::string FORM_PROVIDER_MODULE_SOURCE_DIR = "";

const std::string FORM_HOST_BUNDLE_NAME = "com.form.host.app";

const std::string DEVICE_ID = "ohos-phone1";

const std::string FORM_MESSAGE_EVENT_VALUE_1 = "event message1";

class FmsFormMgrMessageEventTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

protected:
    sptr<MockFormHostClient> token_;
    std::shared_ptr<FormMgrService> formyMgrServ_ = DelayedSingleton<FormMgrService>::GetInstance();
};

void FmsFormMgrMessageEventTest::SetUpTestCase()
{
    FormBmsHelper::GetInstance().SetBundleManager(new BundleMgrService());
    FormAmsHelper::GetInstance().SetAbilityManager(new MockAbilityMgrService());
}

void FmsFormMgrMessageEventTest::TearDownTestCase()
{}

void FmsFormMgrMessageEventTest::SetUp()
{
    formyMgrServ_->OnStart();
    token_ = new (std::nothrow) MockFormHostClient();

    // Permission install
    int userId = 0;
    auto tokenId = AccessToken::AccessTokenKit::GetHapTokenID(userId, FORM_PROVIDER_BUNDLE_NAME, 0);
    auto flag = OHOS::Security::AccessToken::PERMISSION_USER_FIXED;
    AccessToken::AccessTokenKit::GrantPermission(tokenId, PERMISSION_NAME_REQUIRE_FORM, flag);
}

void FmsFormMgrMessageEventTest::TearDown()
{}

/*
 * Feature: FormMgrService
 * Function: FormMgr
 * SubFunction: MessageEvent Function
 * FunctionPoints: FormMgr MessageEvent interface
 * EnvConditions: Mobile that can run ohos test framework
 * CaseDescription: Verify if FormMgr invoke MessageEvent works.
 */
HWTEST_F(FmsFormMgrMessageEventTest, MessageEvent_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_001 start";

    int64_t formId {10000001};
    int callingUid {20000007};
    // Create cache
    FormItemInfo record1;
    record1.SetFormId(formId);
    record1.SetProviderBundleName(FORM_PROVIDER_BUNDLE_NAME);
    record1.SetAbilityName(FORM_PROVIDER_ABILITY_NAME);
    record1.SetTemporaryFlag(false);
    FormRecord retFormRec = FormDataMgr::GetInstance().AllotFormRecord(record1, callingUid);
    // User Uid Add some Useruids into a form in cache
    int formUserUid {1};
    FormDataMgr::GetInstance().AddFormUserUid(formId, formUserUid);
    // Set form host record
    retFormRec.formUserUids.emplace_back(formUserUid);
    FormDBInfo formDBInfo(formId, retFormRec);
    FormDbCache::GetInstance().SaveFormInfo(formDBInfo);
    // Set host record
    FormItemInfo info;
    FormDataMgr::GetInstance().AllotFormHostRecord(info, token_, formId, callingUid);

    Want want;
    want.SetParam(Constants::PARAM_FORM_HOST_BUNDLENAME_KEY, FORM_HOST_BUNDLE_NAME);
    want.SetParam(Constants::PARAM_MODULE_NAME_KEY, PARAM_PROVIDER_MODULE_NAME);
    want.SetParam(Constants::PARAM_FORM_NAME_KEY, PARAM_FORM_NAME);
    want.SetElementName(DEVICE_ID, FORM_PROVIDER_BUNDLE_NAME, FORM_PROVIDER_ABILITY_NAME);
    want.SetParam(Constants::PARAM_MESSAGE_KEY, FORM_MESSAGE_EVENT_VALUE_1);

    bool screenOnFlag = PowerMgr::PowerMgrClient::GetInstance().IsScreenOn();
    if (!screenOnFlag) {
        EXPECT_EQ(ERR_APPEXECFWK_FORM_COMMON_CODE, FormMgr::GetInstance().MessageEvent(formId, want, token_));
    } else {
        EXPECT_EQ(ERR_OK, FormMgr::GetInstance().MessageEvent(formId, want, token_));
    }

    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_001 end";
}

/*
 * Feature: FormMgrService
 * Function: FormMgr
 * SubFunction: MessageEvent Function
 * FunctionPoints: FormMgr MessageEvent interface
 * EnvConditions: Mobile that can run ohos test framework
 * CaseDescription: An exception tests if formId == 0.
 */
HWTEST_F(FmsFormMgrMessageEventTest, MessageEvent_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_003 start";
    int64_t formId = 0;

    Want want;
    want.SetParam(Constants::PARAM_FORM_HOST_BUNDLENAME_KEY, FORM_HOST_BUNDLE_NAME);
    want.SetParam(Constants::PARAM_MODULE_NAME_KEY, PARAM_PROVIDER_MODULE_NAME);
    want.SetParam(Constants::PARAM_FORM_NAME_KEY, PARAM_FORM_NAME);
    want.SetElementName(DEVICE_ID, FORM_PROVIDER_BUNDLE_NAME, FORM_PROVIDER_ABILITY_NAME);
    want.SetParam(Constants::PARAM_MESSAGE_KEY, FORM_MESSAGE_EVENT_VALUE_1);
    EXPECT_EQ(ERR_APPEXECFWK_FORM_INVALID_PARAM, FormMgr::GetInstance().MessageEvent(formId, want, token_));
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_003 end";
}
/*
 * Feature: FormMgrService
 * Function: FormMgr
 * SubFunction: MessageEvent Function
 * FunctionPoints: FormMgr MessageEvent interface
 * EnvConditions: Mobile that can run ohos test framework
 * CaseDescription: An exception tests if callerToken == nullptr.
 */
HWTEST_F(FmsFormMgrMessageEventTest, MessageEvent_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_004 start";
    int64_t formId {10000001};

    Want want;
    want.SetParam(Constants::PARAM_FORM_HOST_BUNDLENAME_KEY, FORM_HOST_BUNDLE_NAME);
    want.SetParam(Constants::PARAM_MODULE_NAME_KEY, PARAM_PROVIDER_MODULE_NAME);
    want.SetParam(Constants::PARAM_FORM_NAME_KEY, PARAM_FORM_NAME);
    want.SetElementName(DEVICE_ID, FORM_PROVIDER_BUNDLE_NAME, FORM_PROVIDER_ABILITY_NAME);
    want.SetParam(Constants::PARAM_MESSAGE_KEY, FORM_MESSAGE_EVENT_VALUE_1);
    EXPECT_EQ(ERR_APPEXECFWK_FORM_INVALID_PARAM, FormMgr::GetInstance().MessageEvent(formId, want, nullptr));
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_004 end";
}
/*
 * Feature: FormMgrService
 * Function: FormMgr
 * SubFunction: MessageEvent Function
 * FunctionPoints: FormMgr MessageEvent interface
 * EnvConditions: Mobile that can run ohos test framework
 * CaseDescription: An exception tests if message info is not exist.
 */
HWTEST_F(FmsFormMgrMessageEventTest, MessageEvent_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_005 start";
    int64_t formId {10000001};

    Want want;
    want.SetParam(Constants::PARAM_FORM_HOST_BUNDLENAME_KEY, FORM_HOST_BUNDLE_NAME);
    want.SetParam(Constants::PARAM_MODULE_NAME_KEY, PARAM_PROVIDER_MODULE_NAME);
    want.SetParam(Constants::PARAM_FORM_NAME_KEY, PARAM_FORM_NAME);
    want.SetElementName(DEVICE_ID, FORM_PROVIDER_BUNDLE_NAME, FORM_PROVIDER_ABILITY_NAME);

    EXPECT_EQ(ERR_APPEXECFWK_FORM_INVALID_PARAM, FormMgr::GetInstance().MessageEvent(formId, want, token_));
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_005 end";
}
/*
 * Feature: FormMgrService
 * Function: FormMgr
 * SubFunction: MessageEvent Function
 * FunctionPoints: FormMgr MessageEvent interface
 * EnvConditions: Mobile that can run ohos test framework
 * CaseDescription: An exception tests if not exist such form.
 */
HWTEST_F(FmsFormMgrMessageEventTest, MessageEvent_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_006 start";
    int64_t formId {11100002};

    Want want;
    want.SetParam(Constants::PARAM_FORM_HOST_BUNDLENAME_KEY, FORM_HOST_BUNDLE_NAME);
    want.SetParam(Constants::PARAM_MODULE_NAME_KEY, PARAM_PROVIDER_MODULE_NAME);
    want.SetParam(Constants::PARAM_FORM_NAME_KEY, PARAM_FORM_NAME);
    want.SetElementName(DEVICE_ID, FORM_PROVIDER_BUNDLE_NAME, FORM_PROVIDER_ABILITY_NAME);
    want.SetParam(Constants::PARAM_MESSAGE_KEY, FORM_MESSAGE_EVENT_VALUE_1);
    EXPECT_EQ(ERR_APPEXECFWK_FORM_NOT_EXIST_ID, FormMgr::GetInstance().MessageEvent(formId, want, token_));
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_006 end";
}
/*
 * Feature: FormMgrService
 * Function: FormMgr
 * SubFunction: MessageEvent Function
 * FunctionPoints: FormMgr MessageEvent interface
 * EnvConditions: Mobile that can run ohos test framework
 * CaseDescription: An exception tests if cannot find target client.
 */
HWTEST_F(FmsFormMgrMessageEventTest, MessageEvent_007, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_007 start";
    int64_t formId {10000001};

    Want want;
    want.SetParam(Constants::PARAM_FORM_HOST_BUNDLENAME_KEY, FORM_HOST_BUNDLE_NAME);
    want.SetParam(Constants::PARAM_MODULE_NAME_KEY, PARAM_PROVIDER_MODULE_NAME);
    want.SetParam(Constants::PARAM_FORM_NAME_KEY, PARAM_FORM_NAME);
    want.SetElementName(DEVICE_ID, FORM_PROVIDER_BUNDLE_NAME, FORM_PROVIDER_ABILITY_NAME);
    want.SetParam(Constants::PARAM_MESSAGE_KEY, FORM_MESSAGE_EVENT_VALUE_1);
    EXPECT_EQ(ERR_APPEXECFWK_FORM_COMMON_CODE, FormMgr::GetInstance().MessageEvent(formId, want,
    new (std::nothrow) MockFormHostClient()));
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_007 end";
}
/*
 * Feature: FormMgrService
 * Function: FormMgr
 * SubFunction: MessageEvent Function
 * FunctionPoints: FormMgr MessageEvent interface
 * EnvConditions: Mobile that can run ohos test framework
 * CaseDescription: An exception tests if form is not self-owned.
 */
HWTEST_F(FmsFormMgrMessageEventTest, MessageEvent_008, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_008 start";
    int64_t formId2 {10000001};
    int64_t formId {10000002};
    int callingUid {0};
    // Create cache
    FormItemInfo record1;
    record1.SetFormId(formId);
    record1.SetProviderBundleName(FORM_HOST_BUNDLE_NAME);
    record1.SetAbilityName(FORM_PROVIDER_ABILITY_NAME);
    record1.SetTemporaryFlag(false);
    FormRecord retFormRec = FormDataMgr::GetInstance().AllotFormRecord(record1, callingUid);
    // User Uid Add some Useruids into a form in cache
    int formUserUid {1};
    FormDataMgr::GetInstance().AddFormUserUid(formId, formUserUid);
    // Set form host record
    retFormRec.formUserUids.emplace_back(formUserUid);
    FormDBInfo formDBInfo(formId, retFormRec);
    FormDbCache::GetInstance().SaveFormInfo(formDBInfo);
    // Set host record
    FormItemInfo info;
    FormDataMgr::GetInstance().AllotFormHostRecord(info, token_, formId, callingUid);

    FormDataMgr::GetInstance().DeleteHostRecord(token_, formId2);

    Want want;
    want.SetParam(Constants::PARAM_FORM_HOST_BUNDLENAME_KEY, FORM_HOST_BUNDLE_NAME);
    want.SetParam(Constants::PARAM_MODULE_NAME_KEY, PARAM_PROVIDER_MODULE_NAME);
    want.SetParam(Constants::PARAM_FORM_NAME_KEY, PARAM_FORM_NAME);
    want.SetElementName(DEVICE_ID, FORM_PROVIDER_BUNDLE_NAME, FORM_PROVIDER_ABILITY_NAME);
    want.SetParam(Constants::PARAM_MESSAGE_KEY, FORM_MESSAGE_EVENT_VALUE_1);
    EXPECT_EQ(ERR_APPEXECFWK_FORM_OPERATION_NOT_SELF, FormMgr::GetInstance().MessageEvent(formId2, want, token_));
    GTEST_LOG_(INFO) << "fms_form_mgr_message_event_test_008 end";
}
}