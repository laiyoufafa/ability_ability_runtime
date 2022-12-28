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

#include "idle_time.h"

#include "hilog_wrapper.h"
#include "transaction/rs_interfaces.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
constexpr int64_t PERIOD = 16666666; // ns
constexpr int64_t MS_PER_NS = 1000000;
constexpr int32_t TRY_COUNT_MAX = 6;
std::shared_ptr<Rosen::VSyncReceiver> receiver_ = nullptr;
}

IdleTime::IdleTime(const std::shared_ptr<EventHandler> &eventHandler, const std::shared_ptr<NativeEngine> &nativeEngine)
{
    eventHandler_ = eventHandler;
    nativeEngine_ = nativeEngine;
}

int64_t IdleTime::GetSysTimeNs()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

void IdleTime::OnVSync(int64_t timestamp, void* client)
{
    // only use for 60HZ
    if (vSyncCount_ > TRY_COUNT_MAX) {
        HILOG_ERROR("Only support 60HZ.");
        return;
    }
    int64_t period = timestamp - firstVSyncTime_;
    int64_t deviation = period - PERIOD;
    deviation = deviation < 0 ? -deviation : deviation;
    int64_t timeOut = MS_PER_NS / deviation  * PERIOD;
    if (timeOut == 0) { // deviation greater than 1ms
        RequestVSync();
        if (period_ == 0) {
            vSyncCount_++;
        }
    } else {
        if (eventHandler_ == nullptr) {
            HILOG_ERROR("eventHandler_ is nullptr.");
            return;
        }
        std::weak_ptr<IdleTime> weak(shared_from_this());
        auto task = [weak]() {
            auto idleTime = weak.lock();
            if (idleTime == nullptr) {
                HILOG_ERROR("idleTime is nullptr.");
                return;
            }
            idleTime->RequestVSync();
        };
        eventHandler_->PostTask(task, timeOut / MS_PER_NS);
        period_ = period;
    }

    firstVSyncTime_ = timestamp;
    HILOG_DEBUG(
        "%{public}s called. vSync count is %{public}d, timestamp is %{public}" PRId64 ", period is %{public}" PRId64,
        __func__, vSyncCount_, timestamp, period_);
}

void IdleTime::RequestVSync()
{
    if (receiver_ == nullptr) {
        auto& rsClient = Rosen::RSInterfaces::GetInstance();
        receiver_ = rsClient.CreateVSyncReceiver("ABILITY");
        if (receiver_ == nullptr) {
            HILOG_ERROR("Create VSync receiver failed.");
            return;
        }
        receiver_->Init();
    }
    Rosen::VSyncReceiver::FrameCallback frameCallback = {
        .userData_ = this,
        .callback_ = [this](int64_t timestamp, void* data) { OnVSync(timestamp, data); },
    };
    receiver_->RequestNextVSync(frameCallback);
}

void IdleTime::EventTask()
{
    if (firstVSyncTime_ == 0 || period_ == 0) {
        PostTask();
        HILOG_ERROR("no VSync occur.");
        return;
    }
    int64_t period = PERIOD;

    int64_t occurTimestamp = GetSysTimeNs();
    int64_t numPeriod = (occurTimestamp - firstVSyncTime_) / period;
    int64_t lastVSyncTime = numPeriod * period + firstVSyncTime_;
    int64_t elapsedTime = occurTimestamp - lastVSyncTime;
    int64_t idleTime = period - elapsedTime;
    // set runtime
    if (nativeEngine_ == nullptr) {
        HILOG_ERROR("nativeEngine_ is nullptr.");
        return;
    }
    nativeEngine_->NotifyIdleTime(idleTime/MS_PER_NS);

    HILOG_DEBUG("%{public}s called. idleTime is %{public}" PRId64, __func__, idleTime);

    PostTask();
}

void IdleTime::PostTask()
{
    if (vSyncCount_ > TRY_COUNT_MAX) {
        HILOG_ERROR("Only support 60HZ.");
        return;
    }

    if (eventHandler_ == nullptr) {
        HILOG_ERROR("eventHandler_ is nullptr.");
        return;
    }
    std::weak_ptr<IdleTime> weak(shared_from_this());
    auto task = [weak]() {
        auto idleTime = weak.lock();
        if (idleTime == nullptr) {
            HILOG_ERROR("idleTime is nullptr.");
            return;
        }
        idleTime->EventTask();
    };
    eventHandler_->PostTask(task, EventQueue::Priority::IDLE);
}

void IdleTime::Start()
{
    RequestVSync();
    PostTask();
}
} // AppExecFwk
} // namespace OHOS