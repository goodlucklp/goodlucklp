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
#include "input_host_adapter.h"
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <malloc.h>
#include <securec.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define HDF_LOG_TAG InputHostAdapter
namespace OHOS {
namespace Input {
int32_t InputHostAdapter::GetInputDevice(uint32_t devIndex, DeviceInfo **devInfo)
{
    // get device
    HDF_LOGI("InputHostAdapter %{public}s: InputHostAdapter called", __func__);
    return InputActionReader_->GetDevice(devIndex, devInfo);
}

int32_t InputHostAdapter::GetInputDeviceList(uint32_t *devNum, DeviceInfo **deviceList, uint32_t size)
{
    // get device list
    HDF_LOGI("InputHostAdapter %{public}s: InputHostAdapter called", __func__);
    return InputActionReader_->GetDeviceList(devNum, deviceList, size);
}

int32_t InputHostAdapter::CloseInputDevice(uint32_t devIndex)
{
    // close device
    HDF_LOGI("InputHostAdapter %{public}s: InputHostAdapter called", __func__);
    return InputActionReader_->CloseDevice(devIndex);
}

int32_t InputHostAdapter::OpenInputDevice(uint32_t devIndex)
{
    // open device
    HDF_LOGI("InputHostAdapter %{public}s: InputHostAdapter called", __func__);
    return InputActionReader_->OpenDevice(devIndex);
}

int32_t InputHostAdapter::ScanInputDevice(DevDesc *staArr, uint32_t arrLen)
{
    HDF_LOGI("InputHostAdapter %{public}s: called line: %{public}d", __func__, __LINE__);
    // scan input device list
    if (staArr == nullptr) {
        HDF_LOGE("InputHostAdapter %{public}s: called line: %{public}d", __func__, __LINE__);
        return INPUT_NULL_PTR;
    }
    RetStatus rc = InputActionReader_->ScanDevice();
    if (rc != INPUT_SUCCESS) {
        return INPUT_FAILURE;
    }
    uint32_t devNum = 0;
    DeviceInfo* buff = new DeviceInfo[arrLen];
    DeviceInfo* deviceList = (DeviceInfo*)buff;
    rc = InputActionReader_->GetDeviceList(&devNum, &deviceList, arrLen);
    HDF_LOGD("devNum:%{public}d", devNum);
    if (rc != INPUT_SUCCESS) {
        HDF_LOGE("getdevice list failed !!!");
        return rc;
    }
    for (uint32_t i=0; i<devNum; i++) {
        if (deviceList + i != nullptr) {
            (staArr + i)->devIndex = (deviceList + i)->devIndex;
            (staArr + i)->devType = (deviceList + i)->devType;
            HDF_LOGD("index:%{public}d type:%{public}d", (staArr + i)->devIndex, (staArr + i)->devType);
        }
    }
    return INPUT_SUCCESS;
}

int32_t InputHostAdapter::SetPowerStatus(uint32_t devIndex, uint32_t status)
{
    // get the input device
    DeviceInfo *devInfo = nullptr;
    int32_t ret = InputActionReader_->GetDevice(devIndex, &devInfo);
    if (ret != INPUT_SUCCESS) {
        HDF_LOGE("InputHostAdapter %{public}s: GetGeneralInfo Failed", __func__);
        return ret;
    }
    ret = InputActionReader_->OpenDevice(devIndex);
    if (ret != INPUT_SUCCESS) {
        HDF_LOGE("InputHostAdapter %{public}s: GetGeneralInfo Failed", __func__);
        return ret;
    }
    return INPUT_SUCCESS;
}

int32_t InputHostAdapter::GetPowerStatus(uint32_t devIndex, uint32_t *status)
{
    HDF_LOGI("InputHostAdapter::%{public}s: called", __func__);
    return InputActionReader_->GetPowerStatus(devIndex, status);
}

int32_t InputHostAdapter::GetDeviceType(uint32_t devIndex, uint32_t *deviceType)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return InputActionReader_->GetDeviceType(devIndex, deviceType);
    return INPUT_SUCCESS;
}

int32_t InputHostAdapter::GetChipName(uint32_t devIndex, char *chipName, uint32_t length)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return InputActionReader_->GetChipName(devIndex, chipName, length);
}

int32_t InputHostAdapter::GetChipInfo(uint32_t devIndex, char *chipInfo, uint32_t length)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return InputActionReader_->GetChipInfo(devIndex, chipInfo, length);
}

int32_t InputHostAdapter::GetVendorName(uint32_t devIndex, char *vendorName, uint32_t length)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return InputActionReader_->GetVendorName(devIndex, vendorName, length);
}

int32_t InputHostAdapter::SetGestureMode(uint32_t devIndex, uint32_t gestureMode)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return INPUT_SUCCESS;
}

int32_t InputHostAdapter::RunCapacitanceTest(uint32_t devIndex, uint32_t testType, char *result, uint32_t length)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return INPUT_SUCCESS;
}

int32_t InputHostAdapter::RunExtraCommand(uint32_t devIndex, InputExtraCmd *cmdInfo)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return INPUT_SUCCESS;
}

int32_t InputHostAdapter::RegisterReportCallback(uint32_t devIndex, InputEventCb* callback)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return InputActionReader_->RegisterReportCallback(devIndex, callback);
}

int32_t InputHostAdapter::UnregisterReportCallback(uint32_t devIndex)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return InputActionReader_->UnregisterReportCallback(devIndex);
}


int32_t InputHostAdapter::RegisterHotPlugCallback(InputHostCb* callback)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return InputActionReader_->RegisterHotPlugCallback(callback);
}

int32_t InputHostAdapter::UnregisterHotPlugCallback(void)
{
    HDF_LOGI("InputHostAdapter::%{public}s:  called", __func__);
    return InputActionReader_->UnregisterHotPlugCallback();
}
}
};







