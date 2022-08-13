/*
    Copyright (c) 2022 Huawei Device Co., Ltd.
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

import resourceManager from '@ohos.resourceManager';

var EVENT_WAITING = "EVENT_WAITING";
var EVENT_CLOSE = "EVENT_CLOSE";
var EVENT_WAITING_CODE = "0";
var EVENT_CLOSE_CODE = "1";
export default {
    data: {
        labelAppName: "",
    },
    onInit() {
        console.info('onInit');
        this.labelAppName = this.appName;
    },
    onShow() {
        console.info('onshow');
    },
    onWaiting() {
        console.info('click waiting for a response');
        callNativeHandler(EVENT_WAITING, EVENT_WAITING_CODE);
    },
    onCloseApp() {
        console.info('click close app');
        callNativeHandler(EVENT_CLOSE, EVENT_CLOSE_CODE);
    }
}