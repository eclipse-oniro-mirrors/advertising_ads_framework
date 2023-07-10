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
#include "ad_init_ability_connect.h"
#include "ad_inner_error_code.h"
#include "ad_constant.h"
#include "ad_hilog_wreapper.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {

void AdsInitAbilityConnection::OnAbilityConnectDone(
    const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int32_t resultCode)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AdsInitAbilityConnection OnAbilityConnectDone");
    proxy_ = (new (std::nothrow) AdsInitSendRequestProxy(remoteObject));
    if (proxy_ == nullptr) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ads init connect SEA get sendRequest proxy failed.");
        return;
    }

    proxy_->SendAdInitOption(adConfigStrJson_, callback_);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AdsInitAbilityConnection OnAbilityConnectDone over");
}

void AdsInitAbilityConnection::OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int32_t resultCode)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "on ability disconnected.");
    proxy_ = nullptr;
}

}  // namespace AdsNapi
}  // namespace CloudNapi
}  // namespace OHOS