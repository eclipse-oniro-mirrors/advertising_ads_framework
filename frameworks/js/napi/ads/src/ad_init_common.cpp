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

#include "ad_init_common.h"
#include "ad_hilog_wreapper.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {

AdsInitCallback::AdsInitCallback(napi_threadsafe_function *tsfn) : tsfn_(tsfn)
{}

AdsInitCallback::~AdsInitCallback()
{}

struct ThreadSafeResultCode resultCode = {0};

void AdsInitCallback::SetAdsInitResult(int32_t resultCode)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AdsInitCallback SetAdsInitResult ret is : %{public}d", resultCode);
    ThreadSafeResultCode *data = new ThreadSafeResultCode;
    data->resultCode = resultCode;
    napi_status status;
    status = napi_acquire_threadsafe_function(*tsfn_);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "napi acquire threadsafe function is %{public}d", status == napi_ok);
    status = napi_call_threadsafe_function(*tsfn_, data, napi_tsfn_blocking);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "napi call_threadsafe_function is %{public}d", status == napi_ok);
}

}  // namespace AdsNapi
}  // namespace CloudNapi
}  // namespace OHOS