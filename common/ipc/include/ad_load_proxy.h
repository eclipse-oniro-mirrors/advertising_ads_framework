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

#ifndef OHOS_CLOUD_ADVERTISING_LOAD_PROXY_H
#define OHOS_CLOUD_ADVERTISING_LOAD_PROXY_H

#include <iremote_proxy.h>
#include "iad_load_proxy.h"

namespace OHOS {
namespace Cloud {
class AdLoadSendRequestProxy : public IRemoteProxy<IAdLoadSendRequest> {
public:
    explicit AdLoadSendRequestProxy(const sptr<IRemoteObject> &remote) : IRemoteProxy<IAdLoadSendRequest>(remote) {}

    virtual ~AdLoadSendRequestProxy() {}

    ErrCode SendAdLoadRequest(const sptr<AdRequestData> &data, const sptr<IAdLoadCallback> &callback,
        int32_t loadAdType) override;

private:
    ErrCode SendAdLoadIpcRequest(int32_t code, MessageParcel &data);
};

class AdRequestBodySendProxy : public IRemoteProxy<IAdRequestBodySend> {
public:
    explicit AdRequestBodySendProxy(const sptr<IRemoteObject> &remote) : IRemoteProxy<IAdRequestBodySend>(remote) {}
    virtual ~AdRequestBodySendProxy() {}
    void SendAdBodyRequest(const sptr<AdRequestData> &data, const sptr<IAdRequestBody> &callback);
};
} // namespace Cloud
} // namespace OHOS
#endif // OHOS_CLOUD_ADVERTISING_LOAD_PROXY_H