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

#ifndef ADS_INIT_SEND_REQUEST_PROXY_H
#define ADS_INIT_SEND_REQUEST_PROXY_H

#include <iremote_proxy.h>
#include "iad_init_send_request_proxy.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
class AdsInitSendRequestProxy : public IRemoteProxy<IAdsInitSendRequest> {
public:
    explicit AdsInitSendRequestProxy(const sptr<IRemoteObject> &remote) : IRemoteProxy<IAdsInitSendRequest>(remote)
    {}

    virtual ~AdsInitSendRequestProxy()
    {}

    void SendAdInitOption(const std::string &adConfigStrJson, const sptr<IRemoteObject> &callback) override;

private:
    void SendRequest(uint32_t code, MessageParcel &data);
};
}  // namespace AdsNapi
}  // namespace CloudNapi
}  // namespace OHOS
#endif  // ADS_INIT_SEND_REQUEST_PROXY_H
