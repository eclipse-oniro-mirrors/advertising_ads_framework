/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_CLOUD_ADVERTISING_LOAD_AD_H
#define OHOS_CLOUD_ADVERTISING_LOAD_AD_H

#include <mutex>
#include <string>

#include "bundle_mgr_interface.h"
#include "ability_connect_callback_stub.h"
#include "system_ability.h"
#include "errors.h"
#include "refbase.h"
#include "ad_load_proxy.h"
#include "iad_load_proxy.h"
#include "iad_load_callback.h"
#include "iad_request_body.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
struct AdServiceElementName {
    std::string bundleName;
    std::string extensionName;
    std::string apiServiceName;
    int userId;
};

class AdLoadService : public RefBase {
public:
    static sptr<AdLoadService> GetInstance();
    AdLoadService();
    ~AdLoadService();
    ErrCode LoadAd(const std::string &request, const std::string &options, const sptr<Cloud::IAdLoadCallback> &callback,
        int32_t loadAdType);
    int32_t RequestAdBody(const std::string &request, const std::string &options,
        const sptr<Cloud::IAdRequestBody> &callback);
    void GetAdServiceElement(AdServiceElementName &adServiceElementName);

private:
    bool ConnectAdKit(const sptr<Cloud::AdRequestData> &data, const sptr<Cloud::IAdLoadCallback> &callback,
        int32_t loadAdType);
    void GetConfigItem(const char *path, AdServiceElementName &adServiceElementName);
    static std::mutex lock_;
    static sptr<AdLoadService> instance_;
    AdServiceElementName adServiceElementName_;
};

class AdRequestConnection : public AAFwk::AbilityConnectionStub {
public:
    AdRequestConnection(const sptr<Cloud::AdRequestData> &data, const sptr<Cloud::IAdLoadCallback> &callback,
        int32_t loadAdType, AdServiceElementName &element)
        : data_(data), callback_(callback), loadAdType_(loadAdType), currAdServiceElementName_(element){};
    AdRequestConnection(const sptr<Cloud::AdRequestData> &data, const sptr<Cloud::IAdRequestBody> &callback,
        AdServiceElementName &element)
        : data_(data), bodyCallback_(callback), currAdServiceElementName_(element){};
    ~AdRequestConnection() = default;
    void OnAbilityConnectDone(const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject,
        int32_t resultCode) override;
    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) override;

private:
    sptr<Cloud::AdRequestData> data_;
    sptr<Cloud::IAdLoadCallback> callback_;
    sptr<Cloud::IAdRequestBody> bodyCallback_;
    int32_t loadAdType_{0};
    sptr<Cloud::AdLoadSendRequestProxy> proxy_{ nullptr };
    sptr<Cloud::AdRequestBodySendProxy> bodyProxy_{ nullptr };
    AdServiceElementName currAdServiceElementName_;
};
} // namespace AdsNapi
} // namespace CloudNapi
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_LOAD_AD_H