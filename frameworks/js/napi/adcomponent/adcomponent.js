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

let fs = globalThis.requireNapi('file.fs');
let hilog = globalThis.requireNapi('hilog');
let configPolicy = globalThis.requireNapi('configPolicy');
const READ_FILE_BUFFER_SIZE = 4096;
const HILOG_DOMAIN_CODE = 65280;

class AdComponent extends ViewPU {
  constructor(e, t, o, s = -1) {
    super(e, o, s);
    this.ads = void 0;
    this.displayOptions = void 0;
    this.interactionListener = void 0;
    this.want = void 0;
    this.__showComponent = new ObservedPropertySimplePU(!1, this, 'showComponent');
    this.setInitiallyProvidedValue(t);
  }

  setInitiallyProvidedValue(e) {
    void 0 !== e.ads && (this.ads = e.ads);
    void 0 !== e.displayOptions && (this.displayOptions = e.displayOptions);
    void 0 !== e.interactionListener && (this.interactionListener = e.interactionListener);
    void 0 !== e.want && (this.want = e.want);
    void 0 !== e.showComponent && (this.showComponent = e.showComponent);
  }

  updateStateVars(e) {}

  purgeVariableDependenciesOnElmtId(e) {
    this.__showComponent.purgeDependencyOnElmtId(e);
  }

  aboutToBeDeleted() {
    this.__showComponent.aboutToBeDeleted();
    SubscriberManager.Get().delete(this.id__());
    this.aboutToBeDeletedInternal();
  }

  get showComponent() {
    return this.__showComponent.get();
  }

  set showComponent(e) {
    this.__showComponent.set(e);
  }

  aboutToAppear() {
    this.getConfigJsonData();
  }

  getConfigJsonData() {
    let e = null;
    configPolicy.getOneCfgFile('etc/advertising/ads_framework/ad_service_config.json', ((t, o) => {
      if (null == t) {
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'value is ' + o);
        try {
          const t = fs.openSync(o);
          const i = new ArrayBuffer(READ_FILE_BUFFER_SIZE);
          let s = fs.readSync(t.fd, i);
          fs.closeSync(t);
          let n = String.fromCharCode(...new Uint8Array(i.slice(0, s)));
          n = n.replace(/[\r\n\t\"]/g, '').replace(/\s*/g, '').replace(/\[|\]/g, '');
          e = this.toMap(n);
          hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'file succeed');
          e || hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'get config json failed');
          this.setWant(e);
        } catch (t) {
          hilog.error(HILOG_DOMAIN_CODE, 'AdComponent', `open file failed with error:${t.code}, message:${t.message}`);
          this.setWant(e);
        }
      } else {
        hilog.info(HILOG_DOMAIN_CODE, 'AdComponent', 'error occurs ' + t);
        this.setWant(e);
      }
    }));
  }

  setWant(e) {
    if (e) {
      this.want = {
        bundleName: null == e ? void 0 : e.providerBundleName,
        abilityName: null == e ? void 0 : e.providerUEAAbilityName,
        parameters: {
          ads: this.ads,
          displayOptions: this.displayOptions,
          'ability.want.params.uiExtensionType': 'ads'
        }
      };
      this.showComponent = !0;
    }
  }

  toMap(e) {
    const t = (e = e.replace(/[{}]/g, '')).split(',');
    const o = {};
    for (let e = 0; e < t.length; e++) {
      const s = t[e];
      const i = s.indexOf(':');
      if (i > -1) {
        const e = s.substring(0, i);
        const t = s.substring(i + 1);
        o[e] = t;
      }
    }
    return o;
  }

  initialRender() {
    this.observeComponentCreation(((e, t) => {
      ViewStackProcessor.StartGetAccessRecordingFor(e);
      Row.create();
      Row.height('100%');
      t || Row.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));

    this.observeComponentCreation(((e, t) => {
      ViewStackProcessor.StartGetAccessRecordingFor(e);
      Column.create();
      Column.width('100%');
      t || Column.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));

    this.observeComponentCreation(((e, t) => {
      ViewStackProcessor.StartGetAccessRecordingFor(e);
      If.create();
      this.showComponent ? this.ifElseBranchUpdateFunction(0, (() => {
        this.observeComponentCreation(((e, t) => {
          ViewStackProcessor.StartGetAccessRecordingFor(e);
          UIExtensionComponent.create(this.want);
          UIExtensionComponent.onReceive((e => {
            this.interactionListener.onStatusChanged(e.status, e.ad, e.data);
          }));
          UIExtensionComponent.width('100%');
          UIExtensionComponent.height('100%');
          t || UIExtensionComponent.pop();
          ViewStackProcessor.StopGetAccessRecording();
        }));
      })) : If.branchId(1);
      t || If.pop();
      ViewStackProcessor.StopGetAccessRecording();
    }));

    If.pop();
    Column.pop();
    Row.pop();
  }

  rerender() {
    this.updateDirtyElements();
  }
}

export default { AdComponent };