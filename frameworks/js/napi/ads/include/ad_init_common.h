/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_CLOUD_ADS_INIT_COMMON_H
#define OHOS_CLOUD_ADS_INIT_COMMON_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "ad_hilog_wreapper.h"
#include "ad_init_callback_stub.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {

struct AsyncCallbackInfoAdsInit {
    napi_env env = nullptr;
    napi_async_work asyncWork = nullptr;
    napi_ref callback = nullptr;
    napi_deferred deferred = nullptr;
    int32_t resultCode;
    std::string adConfigStrJson;
    sptr<IAdsInitCallback> adsCb = nullptr;
    bool isCallback = false;
    int32_t errorCode = NO_ERROR;
};

struct ThreadSafeResultCode {
    int32_t resultCode;
};

class AdsInitCallback : public AdsInitCallbackStub {
public:
    explicit AdsInitCallback(napi_threadsafe_function *tsfn);
    ~AdsInitCallback();
    void SetAdsInitResult(int32_t resultCode) override;

private:
    napi_threadsafe_function *tsfn_;
};

}  // namespace AdsNapi
}  // namespace CloudNapi
}  // namespace OHOS
#endif  // OHOS_CLOUD_ADS_INIT_COMMON_H