/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
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
 * @addtogroup Input
 * @{
 *
 * @brief Provides driver interfaces for the input service.
 *
 * These driver interfaces can be used to open and close input device files, get input events, query device information,
 * register callback functions, and control the feature status.
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file input_manager.h
 *
 * @brief Declares the driver interfaces for managing input devices.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef INPUT_HOST_ADAPTER_H
#define INPUT_HOST_ADAPTER_H

#include "input_action_reader.h"
namespace OHOS {
namespace Input {
class InputHostAdapter
{
public:
    InputHostAdapter()
    {
        InputActionReader_ = std::make_shared<InputActionReader>();
    };
    virtual ~InputHostAdapter() = default;
    InputHostAdapter(const InputHostAdapter &other) = delete;
    InputHostAdapter(InputHostAdapter &&other) = delete;
    InputHostAdapter &operator=(const InputHostAdapter &other) = delete;
    InputHostAdapter &operator=(InputHostAdapter &&other) = delete;
    int32_t ScanInputDevice(DevDesc *staArr, uint32_t arrLen);
    int32_t OpenInputDevice(uint32_t devIndex);
    int32_t CloseInputDevice(uint32_t devIndex);
    int32_t GetInputDevice(uint32_t devIndex, DeviceInfo **devInfo);
    int32_t GetInputDeviceList(uint32_t *devNum, DeviceInfo **deviceList, uint32_t size);
    int32_t RegisterReportCallback(uint32_t devIndex, InputEventCb* callback);
    int32_t SetPowerStatus(uint32_t devIndex, uint32_t status);
    int32_t GetPowerStatus(uint32_t devIndex, uint32_t *status);
    int32_t GetDeviceType(uint32_t devIndex, uint32_t *deviceType);
    int32_t SetGestureMode(uint32_t devIndex, uint32_t gestureMode);
    int32_t RunCapacitanceTest(uint32_t devIndex, uint32_t testType, char *result, uint32_t length);
    int32_t RunExtraCommand(uint32_t devIndex, InputExtraCmd *cmdInfo);
    int32_t GetChipName(uint32_t devIndex, char *chipName, uint32_t length);
    int32_t GetChipInfo(uint32_t devIndex, char *chipInfo, uint32_t length);
    int32_t GetVendorName(uint32_t devIndex, char *vendorName, uint32_t length);
    int32_t UnregisterReportCallback(uint32_t devIndex);
    int32_t RegisterHotPlugCallback(InputHostCb* callback);
    int32_t UnregisterHotPlugCallback(void);
    std::shared_ptr<InputActionReader> getInputActionReader()
    {
        return InputActionReader_;
    }
private:
    std::shared_ptr<InputActionReader> InputActionReader_ {nullptr};

};
}
}
#endif // INPUT_HOST_ADAPTER_H

/** @} */
