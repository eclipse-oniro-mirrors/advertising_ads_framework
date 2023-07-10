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

/**
 * Defines the advertisement.
 *
 * @interface Advertisement
 * @syscap SystemCapability.Cloud.Ads
 * @since 10
 */
export interface Advertisement {
  /**
   * The advertising type.
   * @type { number }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  adType: number;

  /**
   * The content id that is being played in the media.
   * @type { string }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  contentId: string;

  /**
   * The advertising close keywords.
   * @type { string[] }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  adCloseKeywords: string[];

  /**
   * The detailed creative type returned by an advertisement.
   * @type { number }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  creativeType: number;

  /**
   * The server verifies the configuration parameters.
   * @type { Map<string, string> }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  rewardVerifyConfig: Map<string, string>;

  /**
   * The advertising content data.
   * @type { object }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  adContentData: object;

  /**
   * The unique identifier of the advertising event generation.
   * @type { string }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  uniqueId: string;

  /**
   * The subscriber has been rewarded.
   * @type { boolean }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  rewarded: boolean;

  /**
   * The ad has been shown.
   * @type { boolean }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  shown: boolean;

  /**
   * The ad has been clicked.
   * @type { boolean }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  clicked: boolean;

  /**
   * The extended attributes of advertisement.
   * @type { any }
   * @syscap SystemCapability.Cloud.Ads
   * @since 10
   */
  [key:string]: any;
}