/*
 * Copyright (c) 2021 Huawei InputDevice Co., Ltd.
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

#ifndef INPUT_DEVICE_MANAGER_H
#define INPUT_DEVICE_MANAGER_H

#include "input_common.h"
#include <vector>
#include <linux/input.h>
#include <sys/epoll.h>
#include <refbase.h>
#include <thread>
#include <map>
namespace OHOS {
namespace Input {
/* Convenience constants. */
#define BTN_FIRST 0x100  // first button code
#define BTN_LAST 0x15f   // last button code
#define INDENT "  "
#define INDENT2 "    "
#define INDENT3 "      "
#define THOUSAND 1000
#define HUNDRED 1000
#define MV8 8
#define MV16 16
#define FMAX 0xff
#define ONE 1
#define SEVEN 7
#define INDEX0 0
#define INDEX1 1
#define INDEX2 2
#define EVENTBUFFERSIZE 512
#define DEVICEINFOSIZE 80
struct devicAction {
    uint64_t when;
    int32_t deviceIndex;
    int32_t type;
    int32_t code;
    int32_t value;
};
struct InputDeviceDetailInfo {
    inline InputDeviceDetailInfo() : bus(0), vendor(0), product(0), version(0)
    {
    }
    // Information provided by the kernel.
    std::string name;
    std::string location;
    std::string uniqueId;
    uint16_t bus;
    uint16_t vendor;
    uint16_t product;
    uint16_t version;
    std::string descriptor;
    uint16_t nonce;
};
struct InputDevice {
    InputDevice* next;
    int nodeFd; // may be -1 if device is closed
    const int32_t id;
    const std::string path;
    const InputDeviceDetailInfo detailInfo;
    uint32_t classes;
    InputDevice(int fd, int32_t id, const std::string& path, const InputDeviceDetailInfo& detailInfo);
    ~InputDevice();
    void close();
    bool enabled; // initially true
    int32_t enable();
    int32_t disable();
    bool hasValidFd();
    const bool isVirtual; // set if fd < 0 is passed to constructor
    DevAbility devAbility;
};
class InputDeviceManagerInterface : public virtual OHOS::RefBase
{
public:
    enum {
        DEVICE_ADDED = 0x10000000,
        DEVICE_REMOVED = 0x20000000,
        FINISHED_DEVICE_SCAN = 0x30000000,
        FIRST_SYNTHETIC_EVENT = DEVICE_ADDED,
    };
    virtual uint32_t getDeviceClasses(int32_t deviceIndex) const = 0;
    virtual InputDeviceDetailInfo getInputDeviceDetailInfo(int32_t deviceIndex) const = 0;
    virtual size_t getActions(int timeoutMillis, devicAction* buffer, size_t bufferSize) = 0;
    virtual int32_t enableDevice(int32_t deviceIndex) = 0;
    virtual int32_t disableDevice(int32_t deviceIndex) = 0;
protected:
    InputDeviceManagerInterface()
    {
    }
    virtual ~InputDeviceManagerInterface()
    {
    }
};
class InputDeviceManager : public InputDeviceManagerInterface
{
public:
    enum {
        NO_BUILT_IN_KEYBOARD = -2,
    };
    InputDeviceManager();
    virtual uint32_t getDeviceClasses(int32_t deviceIndex) const;
    virtual InputDeviceDetailInfo getInputDeviceDetailInfo(int32_t deviceIndex) const;
    virtual size_t getActions(int timeoutMillis, devicAction* buffer, size_t bufferSize);
    virtual void getDeviceCloseActionCheck(devicAction*& buffer, size_t& capacity);
    virtual void getDeviceOpenActionCheck(devicAction*& buffer, size_t& capacity);
    virtual void getDeviceTouchAction(devicAction*& buffer, size_t& capacity, struct input_event* readBuffer);
    virtual void getDeviceActionResult(void);
    virtual void getEventsDoActionScrren(devicAction*& buffer, size_t& capacity, struct input_event* readBuffer);
    virtual void getEventsDoActionOthers(devicAction*& buffer,
                                      size_t& capacity,
                                      struct input_event*
                                      readBuffer,
                                      InputDevice*& device);
    virtual int32_t getInputDeviceDetailInfo(int& fd, InputDeviceDetailInfo& detailInfo);
public:
    int32_t openDevice(const char* devicePath);
    void addDevice(InputDevice* device);
    void assignDescriptor(InputDeviceDetailInfo& detailInfo);
    void closeDeviceByPath(const char *devicePath);
    void closeVideoDeviceByPath(const std::string& devicePath);
    void closeDevice(InputDevice* device);
    void closeAllDevices();
    int32_t enableDevice(int32_t deviceIndex);
    int32_t disableDevice(int32_t deviceIndex);
    int32_t registerFdForEpoll(int fd);
    int32_t unregisterFdFromEpoll(int fd);
    int32_t registerDeviceForEpoll(InputDevice* device);
    int32_t unregisterDeviceFromEpoll(InputDevice* device);
    int32_t scanDir(const char *dirname);
    int32_t scanVideoDir(const std::string& dirname);
    void scanDevices();
    int32_t readNotify();
    InputDevice* getDevice(int32_t deviceIndex) const;
    InputDevice* getDeviceByPath(const char* devicePath) const;
    InputDevice** getDeviceList(void);
    InputDevice* getDeviceByFd(int fd) const;
    void addToEpfd(int epfd, int fd);
    int32_t readInputEvent(std::string filename);
    void createInputDevice(int fd,
                           int32_t id,
                           const std::string& path,
                           const InputDeviceDetailInfo& detailInfo,
                           int driverVersion);
    uint32_t getDeviceListCnt(void) const
    {
        return inputDevices_.size();
    }
    const char* toString(bool value)
    {
        return value ? "true" : "false";
    }
    uint64_t seconds_to_nanoseconds(uint64_t secs)
    {
        return secs * THOUSAND * THOUSAND * THOUSAND;
    }
    uint64_t microseconds_to_nanoseconds(uint64_t secs)
    {
        return secs * THOUSAND;
    }
    uint64_t processEventTimestamp(const struct input_event& event)
    {
        const uint64_t inputEventTime = seconds_to_nanoseconds(event.time.tv_sec) +
                                        microseconds_to_nanoseconds(event.time.tv_usec);
        return inputEventTime;
    }
    uint64_t systemTimeInput()
    {
        struct timeval t = {};
        t.tv_sec = 0;
        t.tv_usec = 0;
        gettimeofday(&t, nullptr);
        return uint64_t(t.tv_sec)*THOUSAND*THOUSAND*THOUSAND + uint64_t(t.tv_usec)*THOUSAND;
    }
    struct input_absinfo* getABSInof(void)
    {
        return infoABS_;
    }
    mutable std::mutex lock_;
    int32_t builtInKeyboardId_;
    int32_t deviceConut_;
    std::map<int32_t, InputDevice*> inputDevices_;
    InputDevice *openingDevices_;
    InputDevice *closingDevices_;
    InputDevice* inputDevList_ = nullptr;
    bool needToSendFinishedDeviceScan_;
    bool needToReopenDevices_;
    bool needToScanDevices_;
    int epollFd_;
    int iNotifyFd_;
    int readDefaultFd_;
    int inputWd_;
    static const int EPOLL_MAX_EVENTS = 16;
    const char *inputDevicePath_ = "/dev/input";
    struct epoll_event pendingEventItems_[EPOLL_MAX_EVENTS];
    size_t pendingEventCount_;
    size_t pendingEventIndex_;
    bool pendingINotify_;
    bool usingEpollWakeup_;
    std::string watchPath_;
    int pollResult_ = 0;
    bool deviceChanged_ = false;
    struct input_absinfo infoABS_[ABS_CNT] {};
protected:
    virtual ~InputDeviceManager();
};
}
}
#endif // INPUT_DEVICE_MANAGER_H
