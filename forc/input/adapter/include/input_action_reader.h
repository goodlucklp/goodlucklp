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
#ifndef INPUT_READER_H
#define INPUT_READER_H
#include "input_type.h"
#include "input_device_manager.h"
#include <memory>
#include <map>
#include <thread>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <signal.h>
#include <cstring>
#include <sys/time.h>
#include <functional>
namespace OHOS  {
namespace Input {
#define EVENT_BUFFER_SIZE 256
#define INDEX0 0
class InputActionReader {
public:
    InputActionReader() = default;
    virtual ~InputActionReader() = default;
    InputActionReader(const InputActionReader &other) = delete;
    InputActionReader(InputActionReader &&other) = delete;
    InputActionReader &operator=(const InputActionReader &other) = delete;
    InputActionReader &operator=(InputActionReader &&other) = delete;
    RetStatus Init(void);
    void loopOnce();
    void workerThread();
    // InputManager
    RetStatus ScanDevice(void);
    RetStatus OpenDevice(int32_t devIndex);
    RetStatus CloseDevice(int32_t devIndex);
    RetStatus GetDevice(int32_t devIndex, DeviceInfo **devInfo);
    RetStatus GetDeviceList(uint32_t *devNum, DeviceInfo **deviceList, uint32_t size);
    // InputController
    RetStatus SetPowerStatus(uint32_t devIndex, uint32_t status);
    RetStatus GetPowerStatus(uint32_t devIndex, uint32_t *status);
    RetStatus GetDeviceType(uint32_t devIndex, uint32_t *deviceType);
    RetStatus GetChipInfo(uint32_t devIndex, char *chipInfo, uint32_t length);
    RetStatus GetVendorName(uint32_t devIndex, char *vendorName, uint32_t length);
    RetStatus GetChipName(uint32_t devIndex, char *chipName, uint32_t length);
    RetStatus SetGestureMode(uint32_t devIndex, uint32_t gestureMode);
    RetStatus RunCapacitanceTest(uint32_t devIndex, uint32_t testType, char *result, uint32_t length);
    RetStatus RunExtraCommand(uint32_t devIndex, InputExtraCmd *cmd);
    // InputReporter
    RetStatus RegisterReportCallback(uint32_t devIndex, InputEventCb* callback);
    RetStatus UnregisterReportCallback(uint32_t devIndex);
    RetStatus RegisterHotPlugCallback(InputHostCb* callback);
    RetStatus UnregisterHotPlugCallback(void);
    RetStatus ConvertDeviceInfo(InputDevice* inInfo, DeviceInfo* outInfo);
    void ProcessEvents(const devicAction* rawEvents, size_t count);
    void ProcessEventsForDevice(int32_t deviceId, const devicAction* rawEvents, size_t count);
    bool getCallBackFlag()
    {
        return callbackFlag_;
    }
private:
    InputDeviceManager* inputDeviceManger_ {nullptr};
    devicAction eventBuffer_[EVENT_BUFFER_SIZE];
    std::map<int32_t, DeviceInfo> inputDeviceList_;
    std::map<int32_t, InputEventCb*> reportEventPkgCallback_;
    InputHostCb* reportHotPlugEventCallback_ = nullptr;
    bool callbackFlag_ {false};
    std::thread thread_;
};
} // namespace Input
} // namespace OHOS
#endif // INPUT_READER_H