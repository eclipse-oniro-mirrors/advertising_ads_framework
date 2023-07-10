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
#include <cstddef>

#include "napi_common_want.h"
#include "want_params.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi/native_common.h"
#include "ad_load_napi_common.h"
#include "ad_hilog_wreapper.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
AdLoadListenerCallback::AdLoadListenerCallback(napi_env env, AdJSCallback callback) : env_(env), callback_(callback)
{}

AdLoadListenerCallback::~AdLoadListenerCallback()
{}

bool AdLoadListenerCallback::InitAdLoadCallbackWorkEnv(
    napi_env env, uv_loop_s **loop, uv_work_t **work, AdCallbackParam **param)
{
    napi_get_uv_event_loop(env, loop);
    if (*loop == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "loop instance is nullptr");
        return false;
    }
    *work = new (std::nothrow) uv_work_t;
    if (*work == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "work is null");
        return false;
    }
    *param = new (std::nothrow) AdCallbackParam();
    if (*param == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "failed to create AdCallbackParam");
        delete *work;
        *work = nullptr;
        *loop = nullptr;
        return false;
    }
    (*param)->env = env;
    return true;
}

void UvQueneWorkOnAdLoadSuccess(uv_work_t *work, int status)
{
    if ((work == nullptr) || (work->data == nullptr)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "work or data is nullptr");
        return;
    }
    AdCallbackParam *data = reinterpret_cast<AdCallbackParam *>(work->data);
    napi_value results = nullptr;
    size_t adSize = data->ads.size();
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "return ad size is, adSize = %{public}u", static_cast<int>(adSize));
    napi_create_array(data->env, &results);
    for (size_t i = 0; i < adSize; ++i) {
        napi_value ad = AppExecFwk::WrapWantParams(data->env, data->ads.at(i).GetParams());
        napi_set_element(data->env, results, i, ad);
    }
    napi_value undefined = nullptr;
    napi_get_undefined(data->env, &undefined);
    napi_value onAdLoadSuccessFunc = nullptr;
    napi_value resultout = nullptr;
    napi_get_reference_value(data->env, data->callback.onAdLoadSuccess, &onAdLoadSuccessFunc);
    napi_call_function(data->env, undefined, onAdLoadSuccessFunc, 1, &results, &resultout);
    delete data;
    data = nullptr;
    delete work;
}

void UvQueneWorkOnAdLoadFailed(uv_work_t *work, int status)
{
    if ((work == nullptr) || (work->data == nullptr)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "work or data is nullptr");
        return;
    }
    AdCallbackParam *data = reinterpret_cast<AdCallbackParam *>(work->data);
    napi_value results[2] = {0};
    napi_create_int32(data->env, data->errCode, &results[0]);
    napi_create_string_utf8(data->env, data->errMsg.c_str(), NAPI_AUTO_LENGTH, &results[1]);
    napi_value undefined = nullptr;
    napi_get_undefined(data->env, &undefined);
    napi_value onAdLoadFailedFunc = nullptr;
    napi_value resultout = nullptr;
    napi_get_reference_value(data->env, data->callback.onAdLoadFailed, &onAdLoadFailedFunc);
    napi_call_function(data->env, undefined, onAdLoadFailedFunc, 2, results, &resultout);  // 2 params
    delete data;
    data = nullptr;
    delete work;
}

void AdLoadListenerCallback::OnAdLoadSuccess(const std::vector<AAFwk::Want> &result)
{
    uv_loop_s *loop = nullptr;
    uv_work_t *work = nullptr;
    AdCallbackParam *param = nullptr;
    if (!InitAdLoadCallbackWorkEnv(env_, &loop, &work, &param)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "failed to init ad load work environment");
        return;
    }
    param->ads = result;
    param->callback = callback_;
    work->data = reinterpret_cast<void *>(param);
    uv_queue_work(
        loop, work, [](uv_work_t *work) {}, UvQueneWorkOnAdLoadSuccess);
}

void AdLoadListenerCallback::OnAdLoadFailed(int32_t resultCode, const std::string &resultMsg)
{
    uv_loop_s *loop = nullptr;
    uv_work_t *work = nullptr;
    AdCallbackParam *param = nullptr;
    if (!InitAdLoadCallbackWorkEnv(env_, &loop, &work, &param)) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "failed to init ad load work environment");
        return;
    }
    param->errCode = resultCode;
    param->errMsg = resultMsg;
    param->callback = callback_;
    work->data = reinterpret_cast<void *>(param);
    uv_queue_work(
        loop, work, [](uv_work_t *work) {}, UvQueneWorkOnAdLoadFailed);
}
}  // namespace AdsNapi
}  // namespace CloudNapi
}  // namespace OHOS