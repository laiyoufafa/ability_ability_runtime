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
var Context = requireNapi("application.Context")

class ExtensionContext extends Context {
    constructor(obj) {
        super(obj);
        this.currentHapModuleInfo = obj.currentHapModuleInfo
        this.config = obj.config
        this.extensionAbilityInfo = obj.extensionAbilityInfo
    }

    onUpdateConfiguration(config) {
        this.config = config
    }
}

export default ExtensionContext