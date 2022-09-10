/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "input_manager.h"
#include "input_common.h"
#include "input_host_adapter.h"
#include "hdf_io_service_if.h"
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <functional>
#include <malloc.h>
#include <sys/ioctl.h>
#include <securec.h>
#include <unistd.h>
using namespace OHOS::Input;
static std::shared_ptr<InputHostAdapter> gAdapter_ = std::make_shared<InputHostAdapter>();
static int32_t ScanInputDevice(DevDesc *staArr, uint32_t arrLen)
{
    HDF_LOGI("input_manager %{public}s: %{public}d", __func__, __LINE__);
    if (!gAdapter_) {
        HDF_LOGE("input_manager %{public}s: failed !!!", __func__);
        return INPUT_FAILURE;
    }
    return gAdapter_->ScanInputDevice(staArr, arrLen);
}

static int32_t OpenInputDevice(uint32_t devIndex)
{
    HDF_LOGI("input_manager %{public}s: %{public}d", __func__, __LINE__);
    if (!gAdapter_) {
        HDF_LOGE("input_manager %{public}s: failed !!!", __func__);
        return INPUT_FAILURE;
    }
    return gAdapter_->OpenInputDevice(devIndex);
}

static int32_t CloseInputDevice(uint32_t devIndex)
{
    HDF_LOGI("input_manager %{public}s: %{public}d", __func__, __LINE__);
    if (!gAdapter_) {
        HDF_LOGE("input_manager %{public}s: failed !!!", __func__);
        return INPUT_FAILURE;
    }
    return gAdapter_->CloseInputDevice(devIndex);
}

static int32_t GetInputDevice(uint32_t devIndex, DeviceInfo **devInfo)
{
    HDF_LOGI("input_manager %{public}s: %{public}d", __func__, __LINE__);
    if (!gAdapter_) {
        HDF_LOGE("input_manager %{public}s: failed !!!", __func__);
        return INPUT_FAILURE;
    }
    return gAdapter_->GetInputDevice(devIndex, devInfo);
}

static int32_t GetInputDeviceList(uint32_t *devNum, DeviceInfo **devList, uint32_t size)
{
    HDF_LOGI("input_manager %{public}s: %{public}d", __func__, __LINE__);
    if (!gAdapter_) {
        HDF_LOGE("input_manager %{public}s: failed !!!", __func__);
        return INPUT_FAILURE;
    }
    return gAdapter_->GetInputDeviceList(devNum, devList, size);
}

static int32_t RegisterReportCallback(uint32_t devIndex, InputEventCb *callback)
{
    HDF_LOGI("input_manager %{public}s: %{public}d", __func__, __LINE__);
    if (!gAdapter_) {
        HDF_LOGE("input_manager %{public}s: failed !!!", __func__);
        return INPUT_FAILURE;
    }
    return gAdapter_->RegisterReportCallback(devIndex, callback);
}

static int32_t UnregisterReportCallback(uint32_t devIndex)
{
    HDF_LOGI("input_manager %{public}s: %{public}d", __func__, __LINE__);
    if (!gAdapter_) {
        HDF_LOGE("input_manager %{public}s: failed !!!", __func__);
        return INPUT_FAILURE;
    }
    return gAdapter_->UnregisterReportCallback(devIndex);
}

static int32_t RegisterHotPlugCallback(InputHostCb *callback)
{
    HDF_LOGI("input_manager %{public}s: %{public}d", __func__, __LINE__);
    if (!gAdapter_) {
        HDF_LOGE("input_manager %{public}s: failed !!!", __func__);
        return INPUT_FAILURE;
    }
    return gAdapter_->RegisterHotPlugCallback(callback);
}

static int32_t UnregisterHotPlugCallback(void)
{
    HDF_LOGI("input_manager %{public}s: %{public}d", __func__, __LINE__);
    if (!gAdapter_) {
        HDF_LOGE("input_manager %{public}s: failed !!!", __func__);
        return INPUT_FAILURE;
    }
    return gAdapter_->UnregisterHotPlugCallback();
}

#ifdef __cplusplus
extern "C" {
#endif
int32_t InstanceReporterHdi(InputReporter **hdi);
int32_t InstanceControllerHdi(InputController **hdi);
static int32_t InstanceManagerHdi(InputManager **manager)
{
    InputManager *managerHdi = (InputManager *)malloc(sizeof(InputManager));
    if (managerHdi == nullptr) {
        HDF_LOGE("%s: malloc fail", __func__);
        return INPUT_NOMEM;
    }
    (void)memset_s(managerHdi, sizeof(InputManager), 0, sizeof(InputManager));
    managerHdi->ScanInputDevice = &ScanInputDevice;
    managerHdi->OpenInputDevice = &OpenInputDevice;
    managerHdi->CloseInputDevice = &CloseInputDevice;
    managerHdi->GetInputDevice = &GetInputDevice;
    managerHdi->GetInputDeviceList = GetInputDeviceList;
    *manager = managerHdi;
    return INPUT_SUCCESS;
}

int32_t InstanceControllerHdi(InputController **controller)
{
    InputController *controllerHdi = (InputController *)malloc(sizeof(InputController));
    if (controllerHdi == nullptr) {
        HDF_LOGE("%{public}s: malloc fail", __func__);
        return INPUT_NOMEM;
    }
    return INPUT_SUCCESS;
}

int32_t InstanceReporterHdi(InputReporter **reporter)
{
    InputReporter *reporterHdi = (InputReporter *)malloc(sizeof(InputReporter));
    if (reporterHdi == nullptr) {
        HDF_LOGE("%s: malloc fail", __func__);
        return INPUT_NOMEM;
    }
    (void)memset_s(reporterHdi, sizeof(InputReporter), 0, sizeof(InputReporter));
    reporterHdi->RegisterReportCallback = RegisterReportCallback;
    reporterHdi->UnregisterReportCallback = UnregisterReportCallback;
    reporterHdi->RegisterHotPlugCallback = RegisterHotPlugCallback;
    reporterHdi->UnregisterHotPlugCallback = UnregisterHotPlugCallback;
    *reporter = reporterHdi;
    return INPUT_SUCCESS;
}

static int32_t InitDevManager(void)
{
    return INPUT_SUCCESS;
}

static void FreeInputHdi(IInputInterface *hdi)
{
    if (hdi->iInputManager != nullptr) {
        free(hdi->iInputManager);
        hdi->iInputManager = nullptr;
    }
    if (hdi->iInputController != nullptr) {
        free(hdi->iInputController);
        hdi->iInputController = nullptr;
    }
    if (hdi->iInputReporter != nullptr) {
        free(hdi->iInputReporter);
        hdi->iInputReporter = nullptr;
    }
    free(hdi);
}

static IInputInterface *InstanceInputHdi(void)
{
    int32_t ret;
    IInputInterface *hdi = (IInputInterface *)malloc(sizeof(IInputInterface));
    if (hdi == nullptr) {
        HDF_LOGE("%s: malloc fail", __func__);
        return nullptr;
    }
    (void)memset_s(hdi, sizeof(IInputInterface), 0, sizeof(IInputInterface));

    ret = InstanceManagerHdi(&hdi->iInputManager);
    if (ret != INPUT_SUCCESS) {
        FreeInputHdi(hdi);
        return nullptr;
    }
    ret = InstanceControllerHdi(&hdi->iInputController);
    if (ret != INPUT_SUCCESS) {
        FreeInputHdi(hdi);
        return nullptr;
    }
    ret = InstanceReporterHdi(&hdi->iInputReporter);
    if (ret != INPUT_SUCCESS) {
        FreeInputHdi(hdi);
        return nullptr;
    }
    return hdi;
}

int32_t GetInputInterface(IInputInterface **inputInterface)
{
    int32_t ret;
    IInputInterface *inputHdi = nullptr;
    if (inputInterface == nullptr) {
        HDF_LOGE("%{public}s: parameter is null", __func__);
        return INPUT_INVALID_PARAM;
    }
    inputHdi = InstanceInputHdi();
    if (inputHdi == nullptr) {
        HDF_LOGE("%{public}s: failed to instance hdi", __func__);
        return INPUT_NULL_PTR;
    }
    ret = InitDevManager();
    if (ret != INPUT_SUCCESS) {
        HDF_LOGE("%{public}s: failed to initialize manager", __func__);
        FreeInputHdi(inputHdi);
        return INPUT_FAILURE;
    }
    ret = gAdapter_->getInputActionReader()->Init();
    if (INPUT_SUCCESS != ret) {
        HDF_LOGE("input_adapter %{public}s: init failed !!!", __func__);
        FreeInputHdi(inputHdi);
        return INPUT_FAILURE;
    }
    *inputInterface = inputHdi;
    HDF_LOGI("%{public}s: exit succ", __func__);
    return INPUT_SUCCESS;
}

void ReleaseInputInterface(IInputInterface *inputInterface)
{
    if (inputInterface == nullptr) {
        return;
    }
    FreeInputHdi(inputInterface);
    inputInterface = nullptr;

}
#ifdef __cplusplus
}
#endif
