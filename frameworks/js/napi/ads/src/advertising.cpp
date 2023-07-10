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

#include "napi_common_want.h"
#include "napi/native_common.h"
#include <fstream>
#include <memory>
#include <uv.h>
#include "json/json.h"
#include "securec.h"
#include "want_agent_helper.h"
#include "advertising.h"
#include "ad_hilog_wreapper.h"
#include "ad_load_napi_common.h"
#include "ad_service_client.h"
#include "ad_inner_error_code.h"
#include "ad_constant.h"
#include "ability_manager_client.h"
#include "ad_service_interface.h"
#include "ad_init_common.h"
#include "ad_init_ability_connect.h"
#include "tag_for_child.h"
#include "non_personalized_ad.h"
#include "content_classification.h"
#include "ad_constant.h"
#include "ad_init_callback_stub.h"
#include "config_policy_utils.h"

namespace OHOS {
namespace CloudNapi {
namespace AdsNapi {
namespace {
const int32_t NAPI_RETURN_ONE = 1;
const int32_t NO_ERROR_INT32 = 0;
const size_t CALLBACK_ARGS_LENGTH = 2;
const int8_t CALLBACK_REEOR = 0;
const int8_t CALLBACK_DATA = 1;
const int32_t ERROR = -1;
const size_t ADS_MAX_PARA = 3;
static constexpr const int MAX_STRING_LENGTH = 65536;
} // namespace

using Want = OHOS::AAFwk::Want;

static const int32_t SHOW_AD_PARA = 3;
static const int32_t AD_LOADER_PARA = 3;
const std::string AD_LOADER_CLASS_NAME = "AdLoader";
thread_local napi_ref Advertising::adRef_ = nullptr;
static const int32_t STR_MAX_SIZE = 256;
static const int32_t CUSTOM_DATA_MAX_SIZE = 1024 * 1024; // 1M

std::mutex getAdsLock_;
napi_ref g_callback;
napi_deferred g_deferred;
bool g_isCallback = false;
napi_threadsafe_function tsfn;

napi_value NapiGetNull(napi_env env)
{
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

napi_value WrapVoidToJS(napi_env env)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_null(env, &result));
    return result;
}

napi_value SetNonErrorValue(napi_env env, int32_t errCode)
{
    napi_value result = nullptr;
    napi_value eCode = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errCode, &eCode));
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Set non error value to callback result.");
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_set_named_property(env, result, "code", eCode));
    return result;
}

napi_value SetCallbackErrorValue(napi_env env, int32_t errCode)
{
    napi_value result = nullptr;
    napi_value eCode = nullptr;
    napi_value eData = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errCode, &eCode));
    NAPI_CALL(env, napi_create_string_utf8(env, adsErrCodeMsgMap[errCode].c_str(), NAPI_AUTO_LENGTH, &eData));
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "errorCode is : %{public}d , errordata is : %{public}s", errCode,
        adsErrCodeMsgMap[errCode].c_str());
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_set_named_property(env, result, "code", eCode));
    NAPI_CALL(env, napi_set_named_property(env, result, "data", eData));
    return result;
}

void SetPromise(const napi_env &env, const napi_deferred &deferred, const int32_t &errorCode)
{
    napi_value result = nullptr;
    result = WrapVoidToJS(env);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "errorCode is : %{public}d ", errorCode);
    if (errorCode == NO_ERROR_INT32) {
        NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, deferred, result));
    } else {
        result = SetCallbackErrorValue(env, errorCode);
        NAPI_CALL_RETURN_VOID(env, napi_reject_deferred(env, deferred, result));
    }
}

napi_value ParaError(const napi_env &env, const napi_ref &callback)
{
    if (callback != nullptr) {
        return NapiGetNull(env);
    }

    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    napi_create_promise(env, &deferred, &promise);
    SetPromise(env, deferred, ERROR);
    return promise;
}

void SetCallback(const napi_env &env, const napi_ref &callbackIn, const int32_t &errorCode)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "errorCode is : %{public}d ", errorCode);
    napi_value callback = nullptr;
    napi_value resultout = nullptr;
    napi_get_reference_value(env, callbackIn, &callback);
    napi_value results[CALLBACK_ARGS_LENGTH] = {0};
    if (errorCode == NO_ERROR_INT32) {
        results[CALLBACK_REEOR] = SetNonErrorValue(env, errorCode);
    } else {
        results[CALLBACK_REEOR] = SetCallbackErrorValue(env, errorCode);
    }
    results[CALLBACK_DATA] = WrapVoidToJS(env);
    NAPI_CALL_RETURN_VOID(env,
        napi_call_function(env, undefined, callback, CALLBACK_ARGS_LENGTH, &results[CALLBACK_REEOR], &resultout));
}

void ReturnCallbackPromise(const napi_env &env, AsyncCallbackInfoAdsInit *&info)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "isCallback is : %{public}d", info->isCallback);
    if (info->isCallback) {
        SetCallback(env, info->callback, info->errorCode);
    } else {
        SetPromise(env, info->deferred, info->errorCode);
    }
}

void DoCallback(napi_env env, napi_value callbackfunc, void *context, void *data)
{
    ThreadSafeResultCode *retData = (struct ThreadSafeResultCode *)data;
    struct AsyncCallbackInfoAdsInit *asynccallbackinfo = new AsyncCallbackInfoAdsInit;
    asynccallbackinfo->callback = g_callback;
    asynccallbackinfo->deferred = g_deferred;
    asynccallbackinfo->isCallback = g_isCallback;

    if (retData->resultCode == ERR_INVALID_PARAM) {
        asynccallbackinfo->errorCode = ERR_INVALID_PARAM;
    } else if (retData->resultCode == ERR_FROM_KIT_SYSTEM_INTERNAL) {
        asynccallbackinfo->errorCode = ERR_SYSTEM_INTERNAL;
    } else if (retData->resultCode == ERR_FROM_KIT_INIT_OR_UPDATE_SERVICE) {
        asynccallbackinfo->errorCode = ERR_INIT_OR_UPDATE_SERVICE;
    }
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "errorCode is : %{public}d", asynccallbackinfo->errorCode);
    ReturnCallbackPromise(env, asynccallbackinfo);
    delete retData;
    retData = nullptr;
    delete asynccallbackinfo;
    asynccallbackinfo = nullptr;
    napi_delete_reference(env, g_callback);
    g_deferred = NULL;
}

int32_t GetInt32FromValue(napi_env env, napi_value value)
{
    int32_t ret = 2;
    NAPI_CALL_BASE(env, napi_get_value_int32(env, value, &ret), ret);
    return ret;
}

napi_value GetNamedProperty(napi_env env, napi_value object, const std::string &propertyName)
{
    napi_value value = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, object, propertyName.c_str(), &value));
    return value;
}

bool HasNamedProperty(napi_env env, napi_value object, const std::string &propertyName)
{
    bool hasProperty = false;
    NAPI_CALL_BASE(env, napi_has_named_property(env, object, propertyName.c_str(), &hasProperty), false);
    return hasProperty;
}

bool CheckNapiValueType(napi_env env, napi_value value)
{
    if (value) {
        napi_valuetype valueType = napi_valuetype::napi_undefined;
        napi_typeof(env, value, &valueType);
        if (valueType != napi_valuetype::napi_undefined && valueType != napi_valuetype::napi_null) {
            return true;
        }
    }
    return false;
}

bool CheckNullProperty(napi_env env, napi_value object, const std::string &propertyName)
{
    if (!HasNamedProperty(env, object, propertyName)) {
        return true;
    }
    napi_value value = GetNamedProperty(env, object, propertyName);

    if (CheckNapiValueType(env, value)) {
        return false;
    }
    return true;
}

std::string GetStringFromValueUtf8(napi_env env, napi_value value)
{
    std::string result;
    char str[MAX_STRING_LENGTH] = {0};
    size_t length = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, str, MAX_STRING_LENGTH, &length), result);
    if (length > 0) {
        return result.append(str, length);
    }
    return result;
}

void CheckAdOptions(const napi_env &env, napi_value object, const std::string &adOptionName)
{
    bool typeNull = CheckNullProperty(env, object, adOptionName);
    if (typeNull) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Invalid %{public}s value", adOptionName.c_str());
    }
}

std::string UnwrapStringFromJS(napi_env env, napi_value param, const std::string &defaultValue)
{
    size_t size = 0;
    if (napi_get_value_string_utf8(env, param, nullptr, 0, &size) != napi_ok) {
        return defaultValue;
    }

    std::string value("");
    if (size == 0) {
        return defaultValue;
    }

    char *buf = new (std::nothrow) char[size + 1];
    if (buf == nullptr) {
        return value;
    }
    (void)memset_s(buf, size + 1, 0, size + 1);

    bool rev = napi_get_value_string_utf8(env, param, buf, size + 1, &size) == napi_ok;
    if (rev) {
        value = buf;
    } else {
        value = defaultValue;
    }

    delete[] buf;
    buf = nullptr;
    return value;
}

void SetNonPersonalizedAd(const napi_env &env, napi_value &jsonValue, napi_valuetype &jsonValueType, Json::Value &root)
{
    if (jsonValueType == napi_valuetype::napi_number) {
        int32_t adOptionValue = GetInt32FromValue(env, jsonValue);
        if (adOptionValue != ALLOW_ALL && adOptionValue != ALLOW_NON_PERSONALIZED) {
            ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Invalid %{public}s value", NON_PERSONALIZED_AD.c_str());
        } else {
            root[NON_PERSONALIZED_AD] = adOptionValue;
        }
    } else {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Invalid %{public}s value", NON_PERSONALIZED_AD.c_str());
    }
}

void SetAdContentClassification(const napi_env &env, napi_value &jsonValue, napi_valuetype &jsonValueType,
    Json::Value &root)
{
    if (jsonValueType == napi_valuetype::napi_string) {
        std::string adOptionValue = GetStringFromValueUtf8(env, jsonValue);
        if (adOptionValue.compare(AD_CONTENT_CLASSIFICATION_W) != 0 &&
            adOptionValue.compare(AD_CONTENT_CLASSIFICATION_PI) != 0 &&
            adOptionValue.compare(AD_CONTENT_CLASSIFICATION_J) != 0 &&
            adOptionValue.compare(AD_CONTENT_CLASSIFICATION_A) != 0 &&
            adOptionValue.compare(AD_CONTENT_CLASSIFICATION_UNKOWN) != 0) {
            ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Invalid %{public}s value", AD_CONTENT_CLASSIFICATION.c_str());
        } else {
            root[AD_CONTENT_CLASSIFICATION] = adOptionValue;
        }
    } else {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Invalid %{public}s value", AD_CONTENT_CLASSIFICATION.c_str());
    }
}

void SetTagForChildProtection(const napi_env &env, napi_value &jsonValue, napi_valuetype &jsonValueType,
    Json::Value &root)
{
    if (jsonValueType == napi_valuetype::napi_number) {
        int32_t adOptionValue = GetInt32FromValue(env, jsonValue);
        if (adOptionValue != TAG_FOR_CHILD_PROTECTION_UNSPECIFIED && adOptionValue != TAG_FOR_CHILD_PROTECTION_FALSE &&
            adOptionValue != TAG_FOR_CHILD_PROTECTION_TRUE) {
            ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Invalid %{public}s value", TAG_FOR_CHILD_PROTECTION.c_str());
        } else {
            root[TAG_FOR_CHILD_PROTECTION] = adOptionValue;
        }
    } else {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Invalid %{public}s value", TAG_FOR_CHILD_PROTECTION.c_str());
    }
}

void SetDefaultAdOption(const napi_env &env, const std::string &adOptionKey, napi_value &jsonValue,
    napi_valuetype &jsonValueType, Json::Value &root)
{
    if (adOptionKey.compare(TAG_FOR_CHILD_PROTECTION) == 0) {
        SetTagForChildProtection(env, jsonValue, jsonValueType, root);
    } else if (adOptionKey.compare(AD_CONTENT_CLASSIFICATION) == 0) {
        SetAdContentClassification(env, jsonValue, jsonValueType, root);
    } else if (adOptionKey.compare(NON_PERSONALIZED_AD) == 0) {
        SetNonPersonalizedAd(env, jsonValue, jsonValueType, root);
    }
}

void SetOtherAdOption(napi_env &env, napi_value &jsonValue, napi_valuetype &jsonValueType,
    const std::string &adOptionKey, Json::Value &root)
{
    switch (jsonValueType) {
        case napi_string: {
            std::string adOptionValue = GetStringFromValueUtf8(env, jsonValue);
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "adOptionName is : %{public}s adOptionValue is : %{public}s",
                adOptionKey.c_str(), adOptionValue.c_str());
            root[adOptionKey] = adOptionValue;
            break;
        }
        case napi_boolean: {
            bool adOptionValue = false;
            if (napi_get_value_bool(env, jsonValue, &adOptionValue) == napi_ok) {
                ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "adOptionName is : %{public}s value is : %{public}d",
                    adOptionKey.c_str(), adOptionValue);
                root[adOptionKey] = adOptionValue;
            }
            break;
        }
        case napi_number: {
            int32_t adOptionValue = GetInt32FromValue(env, jsonValue);
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "adOptionName is : %{public}s value is : %{public}d",
                adOptionKey.c_str(), adOptionValue);
            root[adOptionKey] = adOptionValue;
            break;
        }
        default:
            break;
    }
}

bool UnwrapParams(napi_env env, napi_value param, Json::Value &root)
{
    napi_valuetype jsonValueType = napi_undefined;
    napi_value jsonNameList = nullptr;
    uint32_t jsonCount = 0;
    NAPI_CALL_BASE(env, napi_get_property_names(env, param, &jsonNameList), false);
    NAPI_CALL_BASE(env, napi_get_array_length(env, jsonNameList, &jsonCount), false);
    napi_value jsonName = nullptr;
    napi_value jsonValue = nullptr;
    for (uint32_t index = 0; index < jsonCount; index++) {
        NAPI_CALL_BASE(env, napi_get_element(env, jsonNameList, index, &jsonName), false);
        std::string adOptionKey = GetStringFromValueUtf8(env, jsonName);
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "%{public}s called. Property name = %{public}s.", __func__,
            adOptionKey.c_str());
        NAPI_CALL_BASE(env, napi_get_named_property(env, param, adOptionKey.c_str(), &jsonValue), false);
        NAPI_CALL_BASE(env, napi_typeof(env, jsonValue, &jsonValueType), false);
        if (adOptionKey.compare(TAG_FOR_CHILD_PROTECTION) == 0 || adOptionKey.compare(AD_CONTENT_CLASSIFICATION) == 0 ||
            adOptionKey.compare(NON_PERSONALIZED_AD) == 0) {
            SetDefaultAdOption(env, adOptionKey, jsonValue, jsonValueType, root); // Set the default three options
            continue;
        }
        SetOtherAdOption(env, jsonValue, jsonValueType, adOptionKey, root);
    }
    return true;
}

void InitAdConfStrJson(Json::Value &root)
{
    if (root[TAG_FOR_CHILD_PROTECTION].isNull() && root[AD_CONTENT_CLASSIFICATION].isNull() &&
        root[NON_PERSONALIZED_AD].isNull()) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Init successed.");
    } else {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Init failed.");
    }
}

napi_value ParseParameters(const napi_env &env, const napi_value (&argv)[ADS_MAX_PARA], const size_t &argc,
    napi_ref &callback, std::string &adConfigStrJson)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Begin.");
    NAPI_ASSERT(env, argc >= ADS_MAX_PARA - 1, "Wrong number of arguments.");

    napi_valuetype valuetype = napi_undefined;

    NAPI_CALL(env, napi_typeof(env, argv[1], &valuetype));
    NAPI_ASSERT(env, valuetype == napi_object, "Wrong argument type, argv[1] object expected.");

    CheckAdOptions(env, argv[1], TAG_FOR_CHILD_PROTECTION);
    CheckAdOptions(env, argv[1], AD_CONTENT_CLASSIFICATION);
    CheckAdOptions(env, argv[1], NON_PERSONALIZED_AD);

    Json::Value root; // AdOptions Json object
    InitAdConfStrJson(root);

    bool unWrapResult = UnwrapParams(env, argv[1], root);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "unWrapResult is : %{public}d", unWrapResult);

    std::string strJsonMsg = Json::FastWriter().write(root);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "strJsonMsg is : %{public}s", strJsonMsg.c_str());
    adConfigStrJson = strJsonMsg;

    if (argc >= ADS_MAX_PARA) {
        valuetype = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[2], &valuetype));  // 2 params
        NAPI_ASSERT(env, valuetype == napi_function, "Wrong argument type, function expected.");
        NAPI_CALL(env, napi_create_reference(env, argv[2], NAPI_RETURN_ONE, &callback)); // 2 params
    }

    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "End.");
    return NapiGetNull(env);
}

void PaddingCallbackInfo(const napi_env &env, AsyncCallbackInfoAdsInit *&asynccallbackinfo, const napi_ref &callback,
    napi_value &promise)
{
    if (callback != nullptr) {
        asynccallbackinfo->isCallback = true;
        asynccallbackinfo->callback = callback;
        g_isCallback = true;
        g_callback = callback;
    } else {
        asynccallbackinfo->isCallback = false;
        napi_deferred deferred = nullptr;
        g_deferred = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_create_promise(env, &deferred, &promise));
        asynccallbackinfo->deferred = deferred;
        g_deferred = deferred;
    }
}

void GetCloudServiceProvider(CloudServiceProvider &cloudServiceProvider)
{
    char pathBuff[MAX_PATH_LEN];
    GetOneCfgFile(DEPENDENCY_CONFIG_FILE_RELATIVE_PATH.c_str(), pathBuff, MAX_PATH_LEN);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Config path is %{public}s", pathBuff);

    char realPath[PATH_MAX];
    if (realpath(pathBuff, realPath) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Parse realpath fail");
        return;
    }

    std::ifstream ifs;
    ifs.open(realPath);
    if (!ifs) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Open file error.");
        return;
    }

    Json::Value jsonValue;
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, ifs, &jsonValue, &errs)) {
        ifs.close();
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Read file failed %{public}s.", errs.c_str());
        return;
    }

    Json::Value cloudServiceBundleName = jsonValue["providerBundleName"];
    Json::Value cloudServiceAbilityName = jsonValue["providerAbilityName"];

    cloudServiceProvider.bundleName = cloudServiceBundleName[0].asString();
    cloudServiceProvider.abilityName = cloudServiceAbilityName[0].asString();

    ifs.close();

    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI,
        "Cloud Service provider from config bundleName is %{public}s, abilityName is %{public}s",
        cloudServiceProvider.bundleName.c_str(), cloudServiceProvider.abilityName.c_str());
}

int32_t ConnectServiceExtensionAbility(std::string &adConfigStrJson, const sptr<IAdsInitCallback> &callback)
{
    sptr<IRemoteObject> callbackObj = nullptr;
    if (callback != nullptr) {
        callbackObj = callback->AsObject();
    }

    CloudServiceProvider cloudServiceProvider;
    GetCloudServiceProvider(cloudServiceProvider);
    std::string bundleName = cloudServiceProvider.bundleName;
    std::string abilityName = cloudServiceProvider.abilityName;
    AAFwk::Want want;
    want.SetElementName(bundleName, abilityName);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "%{public}s begin. bundleName=%{public}s, abilityName=%{public}s",
        __func__, want.GetElement().GetBundleName().c_str(), want.GetElement().GetAbilityName().c_str());

    sptr<AdsInitAbilityConnection> abilityConnection = new AdsInitAbilityConnection(adConfigStrJson, callbackObj);

    int32_t ret = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, abilityConnection, USER_ID);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ConnectAbility err code is : %{public}d", ret);
    return ret;
}

void GetAdsExecuteCallBack(napi_env env, void *data)
{
    std::lock_guard<std::mutex> autoLock(getAdsLock_);
    AsyncCallbackInfoAdsInit *asynccallbackinfo = (AsyncCallbackInfoAdsInit *)data;
    std::string adConfigStrJson = asynccallbackinfo->adConfigStrJson;

    int32_t result = ConnectServiceExtensionAbility(adConfigStrJson, asynccallbackinfo->adsCb);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "result is %{public}d", result);
}

void GetAdsCompleteCallBack(napi_env env, napi_status status, void *data)
{
    AsyncCallbackInfoAdsInit *asynccallbackinfo = (AsyncCallbackInfoAdsInit *)data;
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, asynccallbackinfo->asyncWork));
    delete asynccallbackinfo;
    asynccallbackinfo = nullptr;
}

AsyncCallbackInfoAdsInit *GetAsyncCallbackInfoAdsInit(napi_env &env, napi_callback_info &info, napi_ref &callback,
    napi_value &promise)
{
    size_t argc = ADS_MAX_PARA;
    napi_value argv[ADS_MAX_PARA] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));

    std::string adConfigStrJson;
    if (ParseParameters(env, argv, argc, callback, adConfigStrJson) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetAsyncCallbackInfoAdsInit ParseParameters return null !");
        return nullptr;
    }
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "adConfigStrJson is : %{public}s", adConfigStrJson.c_str());
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetAdsExGetAsyncCallbackInfoAdsInit ParseParameters done");
    AsyncCallbackInfoAdsInit *asynccallbackinfo = new (std::nothrow) AsyncCallbackInfoAdsInit{
        .env = env,
        .asyncWork = nullptr
    };
    if (!asynccallbackinfo) {
        return nullptr;
    }
    asynccallbackinfo->adConfigStrJson = adConfigStrJson;
    PaddingCallbackInfo(env, asynccallbackinfo, callback, promise);
    asynccallbackinfo->adsCb = new (std::nothrow) AdsInitCallback(&tsfn);
    return asynccallbackinfo;
}

napi_value Advertising::Init(napi_env env, napi_callback_info info)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Begin.");
    napi_ref callback = nullptr;
    napi_value promise = nullptr;
    AsyncCallbackInfoAdsInit *asynccallbackinfo = GetAsyncCallbackInfoAdsInit(env, info, callback, promise);
    if (asynccallbackinfo == nullptr) {
        return ParaError(env, callback);
    }

    napi_status napiStatus;
    napi_value workName;
    napi_create_string_utf8(env, "threadsafeCallback", NAPI_AUTO_LENGTH, &workName);
    napiStatus = napi_create_threadsafe_function(env, NULL, NULL, workName, 0, 1, NULL, NULL, NULL, DoCallback, &tsfn);
    NAPI_ASSERT(env, napiStatus == napi_ok, "napi create threadsafe function failed");

    napi_value resourceName = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "init", NAPI_AUTO_LENGTH, &resourceName));
    napi_async_execute_callback getAdsExecuteCallBack = GetAdsExecuteCallBack;
    napi_async_complete_callback getAdsCompleteCallBack = GetAdsCompleteCallBack;
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resourceName, getAdsExecuteCallBack, getAdsCompleteCallBack,
        (void *)asynccallbackinfo, &asynccallbackinfo->asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, asynccallbackinfo->asyncWork));

    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "End.");
    if (asynccallbackinfo->isCallback) {
        return NapiGetNull(env);
    } else {
        return promise;
    }
}

napi_value GetLongStringProperty(const napi_env &env, const napi_value &value, const std::string &key,
    std::string &stringValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetLongStringProperty enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    char str[CUSTOM_DATA_MAX_SIZE] = {0};
    size_t strLen = 0;
    bool hasLongStrProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasLongStrProperty));
    if (hasLongStrProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_string) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. String expected.");
            return nullptr;
        }
        NAPI_CALL(env, napi_get_value_string_utf8(env, result, str, CUSTOM_DATA_MAX_SIZE, &strLen));
        if (strLen > CUSTOM_DATA_MAX_SIZE) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "data over size");
            return nullptr;
        }
        stringValue = str;
    }
    return NapiGetNull(env);
}

napi_value GetShortStringProperty(const napi_env &env, const napi_value &value, const std::string &key,
    std::string &stringValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetShortStringProperty enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    char str[STR_MAX_SIZE] = {0};
    size_t strLen = 0;
    bool hasProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_string) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. String expected.");
            return nullptr;
        }
        NAPI_CALL(env, napi_get_value_string_utf8(env, result, str, STR_MAX_SIZE - 1, &strLen));
        stringValue = str;
    }
    return NapiGetNull(env);
}

napi_value GetBoolProperty(const napi_env &env, const napi_value &value, const std::string &key, bool &boolValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetBoolProperty enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    bool hasProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_boolean) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. Boolean expected.");
            return nullptr;
        }
        napi_get_value_bool(env, result, &boolValue);
    }
    return NapiGetNull(env);
}

napi_value GetInt32Property(const napi_env &env, const napi_value &value, const std::string &key, uint32_t &intValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetInt32Property enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    bool hasProperty = false;
    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_number) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. Number expected.");
            return nullptr;
        }
        napi_get_value_uint32(env, result, &intValue);
    }
    return NapiGetNull(env);
}

napi_value GetStringArrayProperty(const napi_env &env, const napi_value &value, const std::string &key,
    std::vector<std::string> &arrayValue)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetStringArrayProperty enter");
    napi_valuetype valuetype;
    napi_value result = nullptr;
    bool isArray = false;
    char str[STR_MAX_SIZE] = {0};
    size_t strLen = 0;
    bool hasProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        napi_is_array(env, result, &isArray);
        if (!isArray) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "parse param is not array");
            return nullptr;
        }
        uint32_t length = 0;
        napi_get_array_length(env, result, &length);
        if (length <= 0) {
            ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "parse array size is invalid");
            return nullptr;
        }
        for (uint32_t i = 0; i < length; ++i) {
            napi_value singleString = nullptr;
            napi_get_element(env, result, i, &singleString);
            NAPI_CALL(env, napi_typeof(env, singleString, &valuetype));
            if (valuetype != napi_string) {
                ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. String expected.");
                return nullptr;
            }
            if (memset_s(str, STR_MAX_SIZE, 0, STR_MAX_SIZE) != 0) {
                ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "memset_s failed.");
                return nullptr;
            }
            NAPI_CALL(env, napi_get_value_string_utf8(env, singleString, str, STR_MAX_SIZE - 1, &strLen));
            arrayValue.emplace_back(str);
        }
    }
    return NapiGetNull(env);
}

napi_value GetWantProperty(const napi_env &env, const napi_value &value, const std::string &key,
    AAFwk::WantParams &wantParams)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "GetWantProperty enter");
    napi_valuetype valuetype = napi_undefined;
    napi_value result = nullptr;
    bool hasProperty = false;

    NAPI_CALL(env, napi_has_named_property(env, value, key.c_str(), &hasProperty));
    if (hasProperty) {
        napi_get_named_property(env, value, key.c_str(), &result);
        NAPI_CALL(env, napi_typeof(env, result, &valuetype));
        if (valuetype != napi_object) {
            ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. Object expected.");
            return nullptr;
        }
        if (!OHOS::AppExecFwk::UnwrapWantParams(env, result, wantParams)) {
            return nullptr;
        }
    }
    return NapiGetNull(env);
}

napi_value ParseObjectFromJs(napi_env env, napi_value argv, Json::Value &root)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseObjectFromJs enter");
    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv, &valuetype));
    if (valuetype != napi_object) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. argv Object expected.");
        return nullptr;
    }
    napi_value valueList = nullptr;
    uint32_t valueCount = 0;
    napi_value elementName = nullptr;
    napi_value elementValue = nullptr;
    napi_valuetype elementType = napi_undefined;
    NAPI_CALL(env, napi_get_property_names(env, argv, &valueList));
    NAPI_CALL(env, napi_get_array_length(env, valueList, &valueCount));
    for (uint32_t index = 0; index < valueCount; index++) {
        NAPI_CALL(env, napi_get_element(env, valueList, index, &elementName));
        std::string strName = GetStringFromValueUtf8(env, elementName);
        NAPI_CALL(env, napi_get_named_property(env, argv, strName.c_str(), &elementValue));
        NAPI_CALL(env, napi_typeof(env, elementValue, &elementType));
        switch (elementType) {
            case napi_string: {
                std::string displayOptionValue = GetStringFromValueUtf8(env, elementValue);
                ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "napi_string key is : %{public}s ", strName.c_str());
                root[strName] = displayOptionValue;
                break;
            }
            case napi_boolean: {
                bool displayOptionValue = false;
                NAPI_CALL(env, napi_get_value_bool(env, elementValue, &displayOptionValue));
                ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "napi_boolean key is : %{public}s ", strName.c_str());
                root[strName] = displayOptionValue;
                break;
            }
            case napi_number: {
                int32_t displayOptionValue = GetInt32FromValue(env, elementValue);
                ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "napi_number key is : %{public}s ", strName.c_str());
                root[strName] = displayOptionValue;
                break;
            }
            default:
                break;
        }
    }
    return NapiGetNull(env);
}

napi_value ParseAdvertismentByAd(const napi_env &env, const napi_value (&argv)[SHOW_AD_PARA],
    Advertisment &advertisment)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdvertismentByAd enter");
    napi_valuetype valuetype;
    // argv[1]
    NAPI_CALL(env, napi_typeof(env, argv[1], &valuetype));
    if (valuetype != napi_object) {
        ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Wrong argument type. argv[1] Object expected.");
        return nullptr;
    }
    if (GetInt32Property(env, argv[1], AD_RESPONSE_AD_TYPE, advertisment.adType) == nullptr) {
        return nullptr;
    }
    if (GetShortStringProperty(env, argv[1], AD_RESPONSE_CONTENT_ID, advertisment.contentId) == nullptr) {
        return nullptr;
    }
    if (GetStringArrayProperty(env, argv[1], AD_RESPONSE_KEYWORDS, advertisment.adCloseKeywords) == nullptr) {
        return nullptr;
    }
    if (GetInt32Property(env, argv[1], AD_RESPONSE_CREATIVE_TYPE, advertisment.creativeType) == nullptr) {
        return nullptr;
    }
    if (GetLongStringProperty(env, argv[1], AD_RESPONSE_CONTENT_DATA, advertisment.adContentData) == nullptr) {
        return nullptr;
    }
    if (GetShortStringProperty(env, argv[1], AD_RESPONSE_UNIQUE_ID, advertisment.uniqueId) == nullptr) {
        return nullptr;
    }
    if (GetBoolProperty(env, argv[1], AD_RESPONSE_REWARDED, advertisment.rewarded) == nullptr) {
        return nullptr;
    }
    if (GetBoolProperty(env, argv[1], AD_RESPONSE_SHOWN, advertisment.shown) == nullptr) {
        return nullptr;
    }
    if (GetBoolProperty(env, argv[1], AD_RESPONSE_CLICKED, advertisment.clicked) == nullptr) {
        return nullptr;
    }
    return NapiGetNull(env);
}

void AssembShowAdParas(Want &want, const Advertisment &advertisment)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AssembShowAdParas enter");
    Json::Value advertismentRoot;
    advertismentRoot[AD_RESPONSE_CONTENT_DATA] = advertisment.adContentData;
    advertismentRoot[AD_RESPONSE_AD_TYPE] = advertisment.adType;
    advertismentRoot[AD_RESPONSE_CONTENT_ID] = advertisment.contentId;
    Json::Value keywordsArray;
    for (int i = 0; i < advertisment.adCloseKeywords.size(); i++) {
        keywordsArray[i] = advertisment.adCloseKeywords.at(i);
    }
    advertismentRoot[AD_RESPONSE_KEYWORDS] = keywordsArray;
    advertismentRoot[AD_RESPONSE_CREATIVE_TYPE] = advertisment.creativeType;
    advertismentRoot[AD_RESPONSE_REWARD_CONFIG] = {{}, {}};
    advertismentRoot[AD_RESPONSE_UNIQUE_ID] = advertisment.uniqueId;
    advertismentRoot[AD_RESPONSE_REWARDED] = advertisment.rewarded;
    advertismentRoot[AD_RESPONSE_SHOWN] = advertisment.shown;
    advertismentRoot[AD_RESPONSE_CLICKED] = advertisment.clicked;
    std::string advertismentString = Json::FastWriter().write(advertismentRoot);
    want.SetParam(AD_ADVERTISMENT, advertismentString);
}

napi_value Advertising::ShowAd(napi_env env, napi_callback_info info)
{
    size_t argc = SHOW_AD_PARA;
    napi_value argv[SHOW_AD_PARA] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    if (argc < SHOW_AD_PARA) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "wrong number of showad arguments");
        return NapiGetNull(env);
    }
    Json::Value adDisplayOptionsRoot;
    if (ParseObjectFromJs(env, argv[2], adDisplayOptionsRoot) == nullptr) {  // 2 params
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseDisplayOptionsByShowAd failed");
        return NapiGetNull(env);
    }
    std::string displayOptionsString = Json::FastWriter().write(adDisplayOptionsRoot);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "enter show ad display is %{public}s", displayOptionsString.c_str());
    Want want;
    Advertisment advertisment;
    if (ParseAdvertismentByAd(env, argv, advertisment) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdvertismentByAd failed");
        return NapiGetNull(env);
    }
    // assemble
    AssembShowAdParas(want, advertisment);
    want.SetElementName(AD_DEFAULT_BUNDLE_NAME, AD_DEFAULT_SHOW_AD_ABILITY_NAME);
    want.SetParam(AD_DISPLAY_OPTIONS, displayOptionsString);
    ErrCode ret = AAFwk::AbilityManagerClient::GetInstance()->StartAbility(want);
    if (ret != ERR_SEND_OK) {
        ADS_HILOGE(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Fail to show ad, err:%{public}d", ret);
    }
    return NapiGetNull(env);
}

bool GetCallbackProperty(napi_env env, napi_value obj, napi_ref &property, int argc)
{
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, obj, &valueType), false);
    if (valueType != napi_function) {
        return false;
    }
    NAPI_CALL_BASE(env, napi_create_reference(env, obj, argc, &property), false);
    return true;
}

bool GetNamedFunction(napi_env env, napi_value object, const std::string &name, napi_ref &funcRef)
{
    napi_value value = nullptr;
    napi_get_named_property(env, object, name.c_str(), &value);
    return GetCallbackProperty(env, value, funcRef, 1);
}

bool ParseJSCallback(const napi_env &env, const napi_value &argv, AdJSCallback &callback)
{
    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, argv, &valueType);
    if (valueType != napi_object) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AdJSCallback parse failed");
        return false;
    }
    bool hasPropFailed = false;
    napi_has_named_property(env, argv, "onAdLoadFailed", &hasPropFailed);
    bool hasPropSuccess = false;
    napi_has_named_property(env, argv, "onAdLoadSuccess", &hasPropSuccess);
    if (!hasPropFailed || !hasPropSuccess) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "AdJSCallback has no function");
        return false;
    }
    return GetNamedFunction(env, argv, "onAdLoadFailed", callback.onAdLoadFailed) &&
        GetNamedFunction(env, argv, "onAdLoadSuccess", callback.onAdLoadSuccess);
}

napi_value ParseContextForLoadAd(napi_env env, napi_callback_info info, AdvertisingRequestContext *context)
{
    size_t argc = AD_LOADER_PARA;
    napi_value argv[AD_LOADER_PARA] = {nullptr};
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    // argv[0]
    Json::Value requestRoot;
    if (ParseObjectFromJs(env, argv[0], requestRoot) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdRequestByLoadAd failed");
        return NapiGetNull(env);
    }
    std::string requestRootString = Json::FastWriter().write(requestRoot);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "requestRootString is: %{public}s", requestRootString.c_str());
    context->requestString = requestRootString;
    // argv[1]
    Json::Value optionRoot;
    if (ParseObjectFromJs(env, argv[1], optionRoot) == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "ParseAdOptionsByLoadAd failed");
        return NapiGetNull(env);
    }
    std::string optionRootString = Json::FastWriter().write(optionRoot);
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "optionRootString is: %{public}s", optionRootString.c_str());
    context->optionString = optionRootString;
    // argv[2]
    AdJSCallback callback;
    ParseJSCallback(env, argv[2], callback);
    context->adLoadCallback = new (std::nothrow) AdLoadListenerCallback(env, callback);  // 2 params
    return NapiGetNull(env);
}

napi_value Advertising::LoadAd(napi_env env, napi_callback_info info)
{
    ADS_HILOGI(OHOS::Cloud::ADS_MODULE_JS_NAPI, "Advertising::LoadAd enter");
    auto *asyncContext = new (std::nothrow) AdvertisingRequestContext();
    if (asyncContext == nullptr) {
        ADS_HILOGW(OHOS::Cloud::ADS_MODULE_JS_NAPI, "create asyncContext failed");
        return NapiGetNull(env);
    }
    asyncContext->env = env;
    ParseContextForLoadAd(env, info, asyncContext);
    napi_value resourceName = nullptr;
    NAPI_CALL(env, napi_create_string_latin1(env, "LoadAd", NAPI_AUTO_LENGTH, &resourceName));
    NAPI_CALL(env, napi_create_async_work(
        env, nullptr, resourceName,
        [](napi_env env, void *data) {
            auto *asyncContext = reinterpret_cast<AdvertisingRequestContext *>(data);
            ErrCode errCode = AdvertisingServiceClient::GetInstance()->LoadAd(asyncContext->requestString,
                asyncContext->optionString, asyncContext->adLoadCallback);
            asyncContext->errorCode = errCode;
        },
        [](napi_env env, napi_status status, void *data) {
            auto *asyncContext = reinterpret_cast<AdvertisingRequestContext *>(data);
            if ((asyncContext->errorCode != 0) && (asyncContext->adLoadCallback != nullptr)) {
                asyncContext->adLoadCallback->OnAdLoadFailed(asyncContext->errorCode, "failed");
            }
            napi_delete_async_work(env, asyncContext->asyncWork);
            delete asyncContext;
            asyncContext = nullptr;
        },
        reinterpret_cast<void *>(asyncContext), &asyncContext->asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->asyncWork));
    return NapiGetNull(env);
}

napi_value Advertising::AdvertisingInit(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptor[] = {
        DECLARE_NAPI_FUNCTION("init", Init),
        DECLARE_NAPI_FUNCTION("showAd", ShowAd),
    };
    NAPI_CALL(env,
        napi_define_properties(env, exports, sizeof(descriptor) / sizeof(napi_property_descriptor), descriptor));
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("loadAd", LoadAd),
    };
    napi_value constructor = nullptr;
    NAPI_CALL(env, napi_define_class(env, AD_LOADER_CLASS_NAME.c_str(), AD_LOADER_CLASS_NAME.size(), JsConstructor,
        nullptr, sizeof(properties) / sizeof(napi_property_descriptor), properties, &constructor));
    NAPI_CALL(env, napi_set_named_property(env, exports, AD_LOADER_CLASS_NAME.c_str(), constructor));
    return exports;
}

napi_value Advertising::JsConstructor(napi_env env, napi_callback_info info)
{
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
    return thisVar;
}
} // namespace AdsNapi
} // namespace CloudNapi
} // namespace OHOS
