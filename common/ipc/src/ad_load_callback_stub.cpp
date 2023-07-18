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

#include <string>
#include <vector>

#include "want.h"
#include "json/json.h"
#include "ad_hilog_wreapper.h"
#include "ad_constant.h"
#include "ad_load_callback_stub.h"
#include "ad_hilog_wreapper.h"
#include "ad_inner_error_code.h"

namespace OHOS {
namespace Cloud {
AdLoadCallbackStub::AdLoadCallbackStub() {}
AdLoadCallbackStub::~AdLoadCallbackStub() {}

inline std::string Str16ToStr8(const std::u16string &str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::string result = convert.to_bytes(str);
    return result;
}

inline void ParseSingleAd(std::vector<AAFwk::Want> &ads, Json::Value &root)
{
    AAFwk::Want want;
    want.SetParam(AD_RESPONSE_AD_TYPE, root[AD_RESPONSE_AD_TYPE].asInt());
    std::string rewardConfig = Json::FastWriter().write(root[AD_RESPONSE_REWARD_CONFIG]);
    want.SetParam(AD_RESPONSE_REWARD_CONFIG, rewardConfig);
    want.SetParam(AD_RESPONSE_UNIQUE_ID, root[AD_RESPONSE_UNIQUE_ID].asString());
    want.SetParam(AD_RESPONSE_REWARDED, root[AD_RESPONSE_REWARDED].asBool());
    want.SetParam(AD_RESPONSE_SHOWN, root[AD_RESPONSE_SHOWN].asBool());
    want.SetParam(AD_RESPONSE_CLICKED, root[AD_RESPONSE_CLICKED].asBool());
    ads.emplace_back(want);
}

void ParseAdArray(std::string adsString, std::vector<AAFwk::Want> &ads)
{
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(adsString, root)) {
        if (root.type() == Json::arrayValue) {
            int size = root.size();
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_COMMON, "ads size is: %{public}u.", size);
            for (int i = 0; i < size; i++) {
                ParseSingleAd(ads, root[i]);
            }
        }
    }
}

int AdLoadCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    switch (code) {
        case static_cast<uint32_t>(IAdLoadCallback::Message::AD_LOAD_SUCCESS): {
            std::string adsArray = Str16ToStr8(data.ReadString16());
            std::vector<AAFwk::Want> ads;
            ParseAdArray(adsArray, ads);
            OnAdLoadSuccess(ads);
            break;
        }
        case static_cast<uint32_t>(IAdLoadCallback::Message::AD_LOAD_PARAMS_ERROR):
        case static_cast<uint32_t>(IAdLoadCallback::Message::AD_LOAD_INNER_ERROR):
        case static_cast<uint32_t>(IAdLoadCallback::Message::AD_LOAD_FAIL): {
            std::string msg = Str16ToStr8(data.ReadString16());
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "OnAdLoadFailed kit return code = %{public}u, msg = %{public}s",
                code, msg.c_str());
            OnAdLoadFailed(code, msg);
            break;
        }
        default:
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_COMMON, "default, code = %{public}u, flags = %{public}u", code,
                option.GetFlags());
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return ERR_NONE;
}
} // namespace Cloud
} // namespace OHOS