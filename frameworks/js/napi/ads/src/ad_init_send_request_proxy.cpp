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

#include "ad_init_send_request_proxy.h"
#include "ad_inner_error_code.h"
#include "ad_constant.h"
#include "iad_init_callback.h"
#include "ad_hilog_wreapper.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
using namespace OHOS::Cloud;

inline std::u16string Str8ToStr16(const std::string &str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::u16string result = convert.from_bytes(str);
    return result;
}

void AdsInitSendRequestProxy::SendAdInitOption(const std::string &adConfigStrJson, const sptr<IRemoteObject> &callback)
{
    MessageParcel data;

    sptr<IAdsInitCallback> iadscallback = iface_cast<IAdsInitCallback>(callback);

    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "adConfigStrJson. %{public}s", adConfigStrJson.c_str());

    data.WriteRemoteObject(iadscallback->AsObject());
    data.WriteString16(Str8ToStr16(adConfigStrJson));

    SendRequest(SEND_INIT_REQUEST_CODE, data);
}

void AdsInitSendRequestProxy::SendRequest(uint32_t code, MessageParcel &data)
{
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);

    int32_t result = Remote()->SendRequest(code, data, reply, option);
    if (result != ERR_SEND_OK) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI,
            "failed to SendRequest, code = %{public}d, result = %{public}d",
            code,
            result);
    }
    ADS_HILOGI(
        OHOS::Cloud::ADS_MODULE_JS_NAPI, "PrintServiceProxy UpdateExtensionInfo out. ret = [%{public}d]", result);
    result = reply.ReadInt32();
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "reply = [%{public}d]", result);
}

}  // namespace AdsNapi
}  // namespace CloudNapi
}  // namespace OHOS