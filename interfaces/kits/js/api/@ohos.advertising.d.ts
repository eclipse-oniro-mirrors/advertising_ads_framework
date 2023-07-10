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

import UIAbilityContext from './application/UIAbilityContext';
import { AsyncCallback } from '@ohos.base';
import { Advertisement as _Advertisement } from './advertising/advertisement';

/**
 * Advertising.
 *
 * @namespace advertising
 * @syscap SystemCapability.Cloud.Ads
 * @since 10
 */
declare namespace advertising {
  /**
   * Indicates the advertisement.
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  export type Advertisement = _Advertisement;

  /**
   * The parameters in the request for loading one or more advertisements.
   *
   * @interface AdRequestParams
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  export interface AdRequestParams {
    /**
     * The advertisement slot id.
     * @type { string }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    adId: string;

    /**
     * The advertisement type of request.
     * @type { string }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    adType?: number;

    /**
     * The advertisement quantity of request.
     * @type { number }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    adCount?: number;

    /**
     * The advertisement view size width that expect.
     * @type { number }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    adWidth?: number;

    /**
     * The advertisement view size height that expect.
     * @type { number }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    adHeight?: number;

    /**
     * The extended attributes for request parameters.
     * @type { number | boolean | string | undefined }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    [key: string]: number | boolean | string | undefined;
  }

  /**
   * Load advertising options.
   * @interface AdOptions
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  export interface AdOptions {
    /**
     * The tags for children's content.
     * @type { number }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    tagForChildProtection?: number;

    /**
     * Advertisement content Classification setting.
     * @type { string }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    adContentClassification?: string;

    /**
     * Non-personalized advertising settings.
     * @type { number }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    nonPersonalizedAd?: number;

    /**
     * The extended attributes for advertising options.
     * @type { number | boolean | string | undefined }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    [key: string]: number | boolean | string | undefined;
  }

  /**
   * The interaction options info for show advertising.
   * @interface AdDisplayOptions
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  export interface AdDisplayOptions {
    /**
     * Advertising custom data.
     * @type { string }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    customData?: string;

    /**
     * Advertising user Id.
     * @type { string }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    userId?: string;

    /**
     * Indicates whether a dialog box is displayed to notify users of video playback
     * and application download in non-Wi-Fi scenarios.
     * @type { boolean }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    useMobileDataReminder?: boolean;

    /**
     * Indicates whether to mute the playback of the incentive ad video.
     * @type { boolean }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    mute?: boolean;

    /**
     * The type of the scenario where the audio focus is obtained during video playback.
     * @type { number }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    audioFocusType?: number;

    /**
     * The extended attributes for interaction options.
     * @type { number | boolean | string | undefined }
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    [key: string]: number | boolean | string | undefined;
  }

  /**
   * The listener of ad interaction.
   * @interface AdInteractionListener
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  export interface AdInteractionListener {
    /**
     * Ads status callback
     * @param status The current advertising status. The status contains onAdOpen,onAdClose,onAdReward,onAdClick,onVideoPlayBegin and onVideoPlayEnd
     * @param ad - The ad which status is changed
     * @param data The data of current advertising status
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    onStatusChanged(status: string, ad: Advertisement, data: string);
  }

  /**
   * The listener of load advertising.
   * @interface AdLoadListener
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  export interface AdLoadListener {
    /**
     * Called by system when the ad load has been failed.
     *
     * @param { number } errorCode - code of ad loading failure.
     * @param { string } errorMsg - error message
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    onAdLoadFailed(errorCode: number, errorMsg: string): void;

    /**
     * Called by system when the ad load has been successed.
     *
     * @param { Advertisement[] } ads - advertisements are loaded successfully.
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    onAdLoadSuccess(ads: Array<Advertisement>): void;
  }

  /**
   * Initializing the global advertisement configuration.
   *
   * @param { UIAbilityContext } abilityContext - Context of the Media Application.
   * @param { AdOptions } adConfig - the global advertisement configuration.
   * @param { AsyncCallback<void> } callback - Indicates the callback to ads init.
   * @throws {BusinessError} 401 - Invalid input parameter.
   * @throws {BusinessError} 21800001 - System internal error.
   * @throws {BusinessError} 21800002 - Failed to initialize the Advertising configuration.
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  function init(abilityContext: UIAbilityContext, adConfig: AdOptions, callback: AsyncCallback<void>): void;

  /**
   * Initializing the global advertisement configuration.
   *
   * @param { UIAbilityContext } abilityContext - Context of the Media Application.
   * @param { AdOptions } adConfig - the global advertisement configuration.
   * @returns { Promise<void> } the promise returned by the function.
   * @throws {BusinessError} 401 - Invalid input parameter.
   * @throws {BusinessError} 21800001 - System internal error.
   * @throws {BusinessError} 21800002 - Failed to initialize the Advertising configuration.
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  function init(abilityContext: UIAbilityContext, adConfig: AdOptions): Promise<void>;

  /**
   * Show full screen advertising.
   *
   * @param { UIAbilityContext } context - Indicates the UIAbility Context of the Media Application.
   * @param { Advertisement } ad - Indicates the Advertisement content information.
   * @param { AdDisplayOptions } option - Indicates Interaction option object use to the show Ad object.
   * @throws {BusinessError} 401 - Invalid input parameter.
   * @throws {BusinessError} 21800001 - System internal error.
   * @throws {BusinessError} 21800004 - Failed to display the Advertising.
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  function showAd(context: UIAbilityContext, ad: Advertisement, option: AdDisplayOptions): void;

  /**
   * A class for advertising loader.
   *
   * The AdLoader contains the function of requesting to load advertising.
   * All advertising parameters are obtained from this function.
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  export class AdLoader {
    /**
     * Constructs a AdLoader object, UIAbilityContext should be transferred.
     *
     * @param { UIAbilityContext } abilityContext - Indicates the UIAbility Context of the Media Application.
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    constructor(abilityContext: UIAbilityContext);

    /**
     * Load advertising.
     *
     * @param { AdRequestParams } adParam - Indicates the parameters in the request for load ad.
     * @param { AdOptions } adOptions - Indicates the global advertisement configuration.
     * @param { AdLoadListener } listener - Indicates the listener to be registered that use to load Ad.
     * @throws {BusinessError} 401 - Invalid input parameter.
     * @throws {BusinessError} 21800001 - System internal error.
     * @throws {BusinessError} 21800003 - Failed to load the ad request.
     * @syscap SystemCapability.Cloud.Ads
     * @since 10
     */
    loadAd(adParam: AdRequestParams, adOptions: AdOptions, listener: AdLoadListener): void;
  }
}

export default advertising;