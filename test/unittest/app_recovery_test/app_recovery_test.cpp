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

#include <gtest/gtest.h>

#define private public
#include "app_recovery.h"
#undef private
#include "ability.h"
#include "ability_info.h"
#include "event_handler.h"
#include "mock_ability_token.h"
#include "recovery_param.h"

using namespace testing::ext;
namespace OHOS {
namespace AppExecFwk {
class AppRecoveryUnitTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    std::shared_ptr<AppExecFwk::Ability> ability_ = std::make_shared<Ability>();
    std::shared_ptr<AppExecFwk::AbilityInfo> abilityInfo_ = std::make_shared<AbilityInfo>();
    sptr<IRemoteObject> token_ = new MockAbilityToken();
};

void AppRecoveryUnitTest::SetUpTestCase()
{
}

void AppRecoveryUnitTest::TearDownTestCase()
{}

void AppRecoveryUnitTest::SetUp()
{
    AppRecovery::GetInstance().isEnable_ = false;
    AppRecovery::GetInstance().restartFlag_ = 0;
    AppRecovery::GetInstance().saveOccasion_ = 0;
    AppRecovery::GetInstance().saveMode_ = 0;
    AppRecovery::GetInstance().abilityRecoverys_.clear();
}

void AppRecoveryUnitTest::TearDown()
{
}

/**
 * @tc.name: GetRestartFlag_001
 * @tc.desc: Test GetRestartFlag
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, GetRestartFlag_001, TestSize.Level1)
{
    EXPECT_EQ(AppRecovery::GetInstance().GetRestartFlag(), 0);
    AppRecovery::GetInstance().restartFlag_ = RestartFlag::ALWAYS_RESTART;
    EXPECT_EQ(AppRecovery::GetInstance().GetRestartFlag(), RestartFlag::ALWAYS_RESTART);
}

/**
 * @tc.name: GetSaveOccasionFlag_001
 * @tc.desc: Test GetSaveOccasionFlag
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, GetSaveOccasionFlag_001, TestSize.Level1)
{
    EXPECT_EQ(AppRecovery::GetInstance().GetSaveOccasionFlag(), 0);
    AppRecovery::GetInstance().saveOccasion_ = SaveOccasionFlag::SAVE_WHEN_ERROR;
    EXPECT_EQ(AppRecovery::GetInstance().GetSaveOccasionFlag(), SaveOccasionFlag::SAVE_WHEN_ERROR);
}
/**
 * @tc.name: GetSaveModeFlag_001
 * @tc.desc: Test GetSaveModeFlag
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, GetSaveModeFlag_001, TestSize.Level1)
{
    EXPECT_EQ(AppRecovery::GetInstance().GetSaveModeFlag(), 0);
    AppRecovery::GetInstance().saveMode_ = SaveModeFlag::SAVE_WITH_FILE;
    EXPECT_EQ(AppRecovery::GetInstance().GetSaveModeFlag(), SaveModeFlag::SAVE_WITH_FILE);
}

/**
 * @tc.name: InitApplicationInfo_001
 * @tc.desc: Test InitApplicationInfo
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, InitApplicationInfo_001, TestSize.Level1)
{
    auto applicationInfo = std::make_shared<ApplicationInfo>();
    auto testHandler = std::make_shared<EventHandler>();
    EXPECT_TRUE(AppRecovery::GetInstance().InitApplicationInfo(testHandler, applicationInfo));
}

/**
 * @tc.name: EnableAppRecovery_001
 * @tc.desc: EnableAppRecovery with config, check the enable flag is set as expected.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, EnableAppRecovery_001, TestSize.Level1)
{
    EXPECT_FALSE(AppRecovery::GetInstance().IsEnabled());
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::ALWAYS_RESTART, SaveOccasionFlag::SAVE_WHEN_ERROR,
                                  SaveModeFlag::SAVE_WITH_FILE);
    EXPECT_TRUE(AppRecovery::GetInstance().IsEnabled());
}

/**
 * @tc.name: EnableAppRecovery_002
 * @tc.desc: EnableAppRecovery with config, check the config is set as expected.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, EnableAppRecovery_002, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::ALWAYS_RESTART, SaveOccasionFlag::SAVE_WHEN_ERROR,
                                  SaveModeFlag::SAVE_WITH_FILE);
    EXPECT_EQ(RestartFlag::ALWAYS_RESTART, AppRecovery::GetInstance().GetRestartFlag());
    EXPECT_EQ(SaveOccasionFlag::SAVE_WHEN_ERROR, AppRecovery::GetInstance().GetSaveOccasionFlag());
    EXPECT_EQ(SaveModeFlag::SAVE_WITH_FILE, AppRecovery::GetInstance().GetSaveModeFlag());
}

/**
 * @tc.name: EnableAppRecovery_003
 * @tc.desc: EnableAppRecovery with config, check the config is set as expected.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, EnableAppRecovery_003, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::NO_RESTART, SaveOccasionFlag::SAVE_ALL,
                                  SaveModeFlag::SAVE_WITH_SHARED_MEMORY);
    EXPECT_EQ(RestartFlag::NO_RESTART, AppRecovery::GetInstance().GetRestartFlag());
    EXPECT_EQ(SaveOccasionFlag::SAVE_ALL, AppRecovery::GetInstance().GetSaveOccasionFlag());
    EXPECT_EQ(SaveModeFlag::SAVE_WITH_SHARED_MEMORY, AppRecovery::GetInstance().GetSaveModeFlag());
}

/**
 * @tc.name:  AddAbility_001
 * @tc.desc: AddAbility when enable flag is false.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, AddAbility_001, TestSize.Level1)
{
    AppRecovery::GetInstance().isEnable_ = false;
    bool ret = AppRecovery::GetInstance().AddAbility(ability_, abilityInfo_, token_);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  AddAbility_002
 * @tc.desc: AddAbility when abilityRecoverys_ is not empty.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, AddAbility_002, TestSize.Level1)
{
    AppRecovery::GetInstance().isEnable_ = true;
    auto abilityRecovery = std::make_shared<AbilityRecovery>();
    AppRecovery::GetInstance().abilityRecoverys_.push_back(abilityRecovery);
    bool ret = AppRecovery::GetInstance().AddAbility(ability_, abilityInfo_, token_);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  AddAbility_003
 * @tc.desc: AddAbility check the ret as expected.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, AddAbility_003, TestSize.Level1)
{
    AppRecovery::GetInstance().isEnable_ = true;
    bool ret = AppRecovery::GetInstance().AddAbility(ability_, abilityInfo_, token_);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name:  ScheduleSaveAppState_001
 * @tc.desc:  ScheduleSaveAppState when enable flag is false.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, ScheduleSaveAppState_001, TestSize.Level1)
{
    bool ret = AppRecovery::GetInstance().ScheduleSaveAppState(StateReason::DEVELOPER_REQUEST);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  ScheduleSaveAppState_002
 * @tc.desc:  ScheduleSaveAppState when state is not support save.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, ScheduleSaveAppState_002, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::ALWAYS_RESTART, SaveOccasionFlag::SAVE_WHEN_ERROR,
                                  SaveModeFlag::SAVE_WITH_FILE);
    bool ret = AppRecovery::GetInstance().ScheduleSaveAppState(StateReason::LIFECYCLE);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  ScheduleSaveAppState_003
 * @tc.desc:  ScheduleSaveAppState when state is not support save.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, ScheduleSaveAppState_003, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::ALWAYS_RESTART, SaveOccasionFlag::SAVE_WHEN_BACKGROUND,
                                  SaveModeFlag::SAVE_WITH_FILE);
    bool ret = AppRecovery::GetInstance().ScheduleSaveAppState(StateReason::CPP_CRASH);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  ScheduleSaveAppState_004
 * @tc.desc:  ScheduleSaveAppState should be retuen true.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, ScheduleSaveAppState_004, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::ALWAYS_RESTART, SaveOccasionFlag::SAVE_WHEN_ERROR,
                                  SaveModeFlag::SAVE_WITH_FILE);
    bool ret = AppRecovery::GetInstance().ScheduleSaveAppState(StateReason::DEVELOPER_REQUEST);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name:  ScheduleRecoverApp_001
 * @tc.desc:  ScheduleRecoverApp when enable flag is false.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, ScheduleRecoverApp_001, TestSize.Level1)
{
    bool ret = AppRecovery::GetInstance().ScheduleRecoverApp(StateReason::DEVELOPER_REQUEST);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  ScheduleRecoverApp_002
 * @tc.desc:  ScheduleRecoverApp when state is not support save.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, ScheduleRecoverApp_002, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::NO_RESTART, SaveOccasionFlag::SAVE_WHEN_ERROR,
                                  SaveModeFlag::SAVE_WITH_FILE);
    bool ret = AppRecovery::GetInstance().ScheduleRecoverApp(StateReason::DEVELOPER_REQUEST);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  ScheduleRecoverApp_003
 * @tc.desc:  ScheduleRecoverApp when state is not support save.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, ScheduleRecoverApp_003, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::ALWAYS_RESTART, SaveOccasionFlag::SAVE_WHEN_BACKGROUND,
                                  SaveModeFlag::SAVE_WITH_FILE);
    bool ret = AppRecovery::GetInstance().ScheduleRecoverApp(StateReason::LIFECYCLE);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  ScheduleRecoverApp_004
 * @tc.desc:  ScheduleRecoverApp when abilityRecoverys is empty.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, ScheduleRecoverApp_004, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::ALWAYS_RESTART, SaveOccasionFlag::SAVE_WHEN_ERROR,
                                  SaveModeFlag::SAVE_WITH_FILE);
    bool ret = AppRecovery::GetInstance().ScheduleRecoverApp(StateReason::DEVELOPER_REQUEST);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  ScheduleRecoverApp_005
 * @tc.desc:  ScheduleRecoverApp should be retuen true.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, ScheduleRecoverApp_005, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::ALWAYS_RESTART, SaveOccasionFlag::SAVE_WHEN_ERROR,
                                  SaveModeFlag::SAVE_WITH_FILE);
    auto caseAbilityInfo = std::make_shared<AbilityInfo>();
    auto ability = std::make_shared<Ability>();
    sptr<IRemoteObject> token = new MockAbilityToken();
    EXPECT_TRUE(AppRecovery::GetInstance().AddAbility(ability_, abilityInfo_, token_));
    bool ret = AppRecovery::GetInstance().ScheduleRecoverApp(StateReason::DEVELOPER_REQUEST);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name:  TryRecoverApp_001
 * @tc.desc:  TryRecoverApp when enable flag is false.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, TryRecoverApp_001, TestSize.Level1)
{
    bool ret = AppRecovery::GetInstance().TryRecoverApp(StateReason::DEVELOPER_REQUEST);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name:  TryRecoverApp_002
 * @tc.desc:  TryRecoverApp should be retuen true.
 * @tc.type: FUNC
 * @tc.require: I5UL6H
 */
HWTEST_F(AppRecoveryUnitTest, TryRecoverApp_002, TestSize.Level1)
{
    AppRecovery::GetInstance().EnableAppRecovery(RestartFlag::ALWAYS_RESTART, SaveOccasionFlag::SAVE_WHEN_ERROR,
                                  SaveModeFlag::SAVE_WITH_FILE);
    auto caseAbilityInfo = std::make_shared<AbilityInfo>();
    auto ability = std::make_shared<Ability>();
    sptr<IRemoteObject> token = new MockAbilityToken();
    EXPECT_TRUE(AppRecovery::GetInstance().AddAbility(ability_, abilityInfo_, token_));
    bool ret = AppRecovery::GetInstance().ScheduleRecoverApp(StateReason::DEVELOPER_REQUEST);
    EXPECT_TRUE(ret);
}
}  // namespace AppExecFwk
}  // namespace OHOS
