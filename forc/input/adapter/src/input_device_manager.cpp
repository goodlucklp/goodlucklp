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
#include "input_device_manager.h"
#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <memory.h>
#include "securec.h"
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <new>

#define HDF_LOG_TAG InputDeviceManager
#define ASSERT assert
namespace OHOS {
namespace Input {
// --- InputDevice ---
InputDevice::InputDevice(int fd, int32_t id, const std::string& path, const InputDeviceDetailInfo& detailInfo):
    next(nullptr),
    nodeFd(fd),
    id(id),
    path(path),
    detailInfo(detailInfo),
    classes(0),
    enabled(false),
    isVirtual(nodeFd < 0)
{
    memset_s(&devAbility, sizeof(devAbility), 0, sizeof(devAbility));
}

InputDevice::~InputDevice()
{
    close();
}

int32_t InputDevice::enable()
{
    nodeFd = open(path.c_str(), O_RDWR | O_CLOEXEC | O_NONBLOCK);
    if (nodeFd < 0) {
        HDF_LOGD("could not open %{public}s, %{public}s", path.c_str(), strerror(errno));
        return -errno;
    }
    enabled = true;
    return INPUT_SUCCESS;
}

int32_t InputDevice::disable()
{
    close();
    enabled = false;
    return INPUT_SUCCESS;
}

bool InputDevice::hasValidFd()
{
    return !isVirtual && enabled;
}

void InputDevice::close()
{
    if (nodeFd >= 0) {
        ::close(nodeFd);
        nodeFd = -1;
    }
}

// --- InputDeviceManager ---
InputDeviceManager::InputDeviceManager(void) :
    builtInKeyboardId_(NO_BUILT_IN_KEYBOARD),
    deviceConut_(INDEX1),
    openingDevices_(nullptr),
    closingDevices_(nullptr),
    needToSendFinishedDeviceScan_(false),
    needToReopenDevices_(false),
    needToScanDevices_(true),
    pendingEventCount_(INDEX0),
    pendingEventIndex_(INDEX0),
    pendingINotify_(false)
{
    epollFd_ = epoll_create1(EPOLL_CLOEXEC);
    iNotifyFd_ = inotify_init();
    inputWd_ = inotify_add_watch(iNotifyFd_, inputDevicePath_, IN_DELETE | IN_CREATE | IN_OPEN);
    struct epoll_event eventItem;
    memset(&eventItem, 0, sizeof(eventItem));
    eventItem.events = EPOLLIN;
    eventItem.data.fd = iNotifyFd_;
    int result = epoll_ctl(epollFd_, EPOLL_CTL_ADD, iNotifyFd_, &eventItem);
    scanDevices();
    HDF_LOGD("watchPath is :%{public}s", watchPath_.c_str());
    readDefaultFd_ = open(watchPath_.c_str(), O_RDONLY);
    eventItem.data.fd = readDefaultFd_;
    HDF_LOGI("result is: %{public}d readDefaultFd_ is :%{public}d", result, readDefaultFd_);
    result = epoll_ctl(epollFd_, EPOLL_CTL_ADD, readDefaultFd_, &eventItem);
}

InputDeviceManager::~InputDeviceManager(void)
{
    closeAllDevices();
    while (closingDevices_) {
        InputDevice* device = closingDevices_;
        closingDevices_ = device->next;
        delete device;
    }
    ::close(epollFd_);
    ::close(iNotifyFd_);
}

InputDeviceDetailInfo InputDeviceManager::getInputDeviceDetailInfo(int32_t deviceIndex) const
{
    std::lock_guard<std::mutex> guard(lock_);
    InputDevice* device = getDevice(deviceIndex);
    if (device == nullptr) {
        return InputDeviceDetailInfo();
    }
    return device->detailInfo;
}

uint32_t InputDeviceManager::getDeviceClasses(int32_t deviceIndex) const
{
    std::lock_guard<std::mutex> guard(lock_);
    InputDevice* device = getDevice(deviceIndex);
    if (device == nullptr) {
        return INPUT_SUCCESS;
    }
    return device->classes;
}

static std::string generateDescriptor(InputDeviceDetailInfo& detailInfo)
{
    std::string rawDescriptor;
    rawDescriptor += std::to_string(detailInfo.vendor) + std::to_string(detailInfo.product);
    if (!detailInfo.uniqueId.empty()) {
        rawDescriptor += "uniqueId:";
        rawDescriptor += detailInfo.uniqueId;
    } else if (detailInfo.nonce != 0) {
        rawDescriptor += std::to_string(detailInfo.nonce);
    }
    if (detailInfo.vendor == 0 && detailInfo.product == 0) {
        if (!detailInfo.name.empty()) {
            rawDescriptor += "name:";
            rawDescriptor += detailInfo.name;
        } else if (!detailInfo.location.empty()) {
            rawDescriptor += "location:";
            rawDescriptor += detailInfo.location;
        }
    }
    return rawDescriptor;
}

void InputDeviceManager::assignDescriptor(InputDeviceDetailInfo& detailInfo)
{
    detailInfo.nonce = INDEX0;
    std::string rawDescriptor = generateDescriptor(detailInfo);
    if (detailInfo.uniqueId.empty()) {
        rawDescriptor = generateDescriptor(detailInfo);
    }
    HDF_LOGD("created descriptor: raw=%{public}s, cooked=%{public}s", rawDescriptor.c_str(),
             detailInfo.descriptor.c_str());
}

InputDevice* InputDeviceManager::getDevice(int32_t deviceIndex) const
{
    HDF_LOGI("get devIndex: %{public}d", deviceIndex);
    for (size_t i = INDEX0; i <= inputDevices_.size(); i++) {
        auto it = inputDevices_.find(i);
        if (it != inputDevices_.end()) {
            InputDevice* device = it->second;
            HDF_LOGI("device id is:%{public}d, path is:%{public}s", device->id, device->path.c_str());
            if (device->id == deviceIndex) {
                return device;
            }
        } else {
            continue;
        }
    }
    return nullptr;
}

InputDevice* InputDeviceManager::getDeviceByPath(const char* devicePath) const
{
    for (size_t i = INDEX0; i <= inputDevices_.size(); i++) {
        auto it = inputDevices_.find(i);
        if (it != inputDevices_.end()) {
            InputDevice* device = it->second;
            if (device->path == std::string(devicePath)) {
                return device;
            }
        } else {
            continue;
        }
    }
    return nullptr;
}

InputDevice** InputDeviceManager::getDeviceList(void)
{
    HDF_LOGE("DevcLsit Size:%{public}d",inputDevices_.size());
    inputDevList_ = (InputDevice*)(new char[sizeof(InputDevice) * inputDevices_.size()]);
    for (size_t i=INDEX1; i <= inputDevices_.size(); i++) {
        memcpy_s(inputDevList_ + i, sizeof(InputDevice), inputDevices_[i], sizeof(InputDevice));
    }
    return inputDevices_.size() > 0 ? &inputDevList_ : nullptr;
}

InputDevice* InputDeviceManager::getDeviceByFd(int fd) const
{
    HDF_LOGI("InputDevice fd: %{public}d", fd);
    for (size_t i = INDEX0; i <= inputDevices_.size(); i++) {
        auto it = inputDevices_.find(i);
        if (it != inputDevices_.end()) {
            InputDevice* device = it->second;
            HDF_LOGI("get device id:%{public}d, path:%{public}s",device->id, device->path.c_str());
            if (device->nodeFd == fd) {
                return device;
            }
        } else {
            continue;
        }
    }
    return nullptr;
}

void InputDeviceManager::getDeviceCloseActionCheck(devicAction*& event, size_t& bufferSize)
{
    // report any devices that had last been added/removed.
    while (closingDevices_) {
        InputDevice* device = closingDevices_;
        HDF_LOGD("Reporting device closed: id=%{public}d, name=%{public}s",
                 device->id, device->path.c_str());
        closingDevices_ = device->next;
        event->when = systemTimeInput();
        event->deviceIndex = device->id;
        event->type = DEVICE_REMOVED;
        event += 1;
        delete device;
        needToSendFinishedDeviceScan_ = true;
        if (--bufferSize == 0) {
            break;
        }
    }
}

void InputDeviceManager::getDeviceOpenActionCheck(devicAction*& event, size_t& bufferSize)
{
    while (openingDevices_ != nullptr) {
        InputDevice* device = openingDevices_;
        HDF_LOGD("Reporting device opened: id=%{public}d, name=%{public}s",
                 device->id, device->path.c_str());
        openingDevices_ = device->next;
        event->when = systemTimeInput();
        event->deviceIndex = ((device->id == builtInKeyboardId_) ? 0 : device->id);
        event->type = DEVICE_ADDED;
        event += 1;
        needToSendFinishedDeviceScan_ = true;
        if (--bufferSize == 0) {
            break;
        }
    }
}

void InputDeviceManager::getEventsDoActionScrren(devicAction*& eventHappend, size_t& capacity, struct input_event* readBuffer)
{
    int32_t readSize = read(readDefaultFd_, readBuffer, sizeof(struct input_event) * capacity);
    if (readSize == INDEX0 || (readSize < INDEX0 && errno == ENODEV)) {
        // device was removed before INotify noticed.
        return;
    } else if (readSize < INDEX0) {
        if (errno != EAGAIN && errno != EINTR) {
            HDF_LOGD("could not get event (errno=%{public}d)", errno);
        }
    } else if ((readSize % sizeof(struct input_event)) != 0) {
        HDF_LOGD("could not get event (wrong size: %{public}d)", readSize);
    } else {
        InputDevice* device = getDeviceByPath(watchPath_.c_str());
        if (device != nullptr) {
            HDF_LOGI("device is null !!!");
            HDF_LOGD("index:%{public}d", device->id);
            size_t count = size_t(readSize) / sizeof(struct input_event);
            for (size_t i = INDEX0; i < count; i++) {
                struct input_event& iEvent = readBuffer[i];
                eventHappend->when = processEventTimestamp(iEvent);
                eventHappend->deviceIndex = device->id;
                eventHappend->code = iEvent.code;
                eventHappend->type = iEvent.type;
                eventHappend->value = iEvent.value;
                eventHappend += 1;
                capacity -= 1;
            }
            if (capacity == INDEX0) {
                // if the result buffer is full. reset the pending event index
                pendingEventIndex_ -= 1;
                return;
            }
        }
    }
}

void InputDeviceManager::getEventsDoActionOthers(devicAction*& eventHappend,
                                    size_t& capacity,
                                    struct input_event* readBuffer,
                                    InputDevice*& device)
{
    int32_t readSize = read(device->nodeFd, readBuffer, sizeof(struct input_event) * capacity);
    if (readSize == INDEX0 || (readSize < INDEX0 && errno == ENODEV)) {
        // InputDevice was removed before INotify noticed.
        HDF_LOGD("could not get event, removed? (fd: %{public}d size: %" PRId32
                 " capacity: %zu errno: %{public}d)",
                 device->nodeFd, readSize, capacity, errno);
        deviceChanged_ = true;
        closeDevice(device);
    } else if (readSize < INDEX0) {
        if (errno != EAGAIN && errno != EINTR) {
            HDF_LOGD("could not get event (errno=%{public}d)", errno);
        }
    } else if ((readSize % sizeof(struct input_event)) != INDEX0) {
        HDF_LOGD("could not get event (wrong size: %{public}d)", readSize);
    } else {
        int32_t deviceIndex = device->id;
        size_t count = size_t(readSize) / sizeof(struct input_event);
        for (size_t i = INDEX0; i < count; i++) {
            struct input_event& iEvent = readBuffer[i];
            eventHappend->when = processEventTimestamp(iEvent);
            eventHappend->deviceIndex = deviceIndex;
            eventHappend->type = iEvent.type;
            eventHappend->code = iEvent.code;
            eventHappend->value = iEvent.value;
            eventHappend += 1;
            capacity -= 1;
        }
        if (capacity == INDEX0) {
            // if result buffer is full. reset the pending event index
            pendingEventIndex_ -= 1;
            return;
        }
    }
}

void InputDeviceManager::getDeviceTouchAction(devicAction*& event, size_t& capacity, struct input_event* readBuffer)
{
    while (pendingEventIndex_ < pendingEventCount_) {
        const struct epoll_event& eventItem = pendingEventItems_[pendingEventIndex_++];
        if (eventItem.data.fd == iNotifyFd_) {
            if (eventItem.events & EPOLLIN) {
                pendingINotify_ = true;
            } else {
                HDF_LOGD("Received unexpected epoll event 0x%08x for INotify.", eventItem.events);
            }
            continue;
        }
        if (eventItem.data.fd == readDefaultFd_) {
            if ((eventItem.events & EPOLLIN) || (eventItem.events & EPOLLET)) {
                getEventsDoActionScrren(event, capacity, readBuffer);
            } else {
                HDF_LOGD("Received unexpected epoll event 0x%{public}08x for INotify.", eventItem.events);
            }
        }
        InputDevice* device = getDeviceByFd(eventItem.data.fd);
        if (!device || device->detailInfo.name == "VSoC touchscreen") {
            HDF_LOGD("Received unexpected epoll event 0x%{public}08x for unknown fd %{public}d.",
                     eventItem.events, eventItem.data.fd);
            continue;
        }
        // should be an input event
        if ((eventItem.events & EPOLLIN) || (eventItem.events & EPOLLET)) {
            getEventsDoActionOthers(event, capacity, readBuffer, device);
        } else if (eventItem.events & EPOLLHUP) {
            HDF_LOGD("Removing device %{public}s due to epoll hang-up event.", device->detailInfo.name.c_str());
            deviceChanged_ = true;
            closeDevice(device);
        } else {
            HDF_LOGD("Received unexpected epoll event 0x%08x for device %{public}s.",
                     eventItem.events, device->detailInfo.name.c_str());
        }
    }
}

void InputDeviceManager::getDeviceActionResult(void)
{
    if (pollResult_ < INDEX0) {
        // error occurred.
        pendingEventCount_ = INDEX0;
        HDF_LOGI(" epoll_wait ");
        // sleep after errors to avoid locking up the system.
        if (errno != EINTR) {
            HDF_LOGD("poll failed (errno=%{public}d)", errno);
            usleep(HUNDRED * THOUSAND);
        }
    } else {
        // events occurred.
        pendingEventCount_ = size_t(pollResult_);
    }
}

size_t InputDeviceManager::getActions(int timeoutMillis, devicAction* buffer, size_t bufferSize)
{
    if (bufferSize < INDEX1) {
        HDF_LOGI("bufferSize wrong!!!");
        return INPUT_SUCCESS;
    }
    std::lock_guard<std::mutex> guard(lock_);
    devicAction* event = buffer;
    size_t capacity = bufferSize;
    struct input_event readBuffer[bufferSize];
    bool awoken = false;
    while (true) {
        uint64_t now = systemTimeInput();
        // repen input devices if needed.
        if (needToReopenDevices_) {
            needToReopenDevices_ = false;
            HDF_LOGD("Reopening all input devices due to a configuration change.");
            closeAllDevices();
            needToScanDevices_ = true;
            break;
        }
        // Do ActionDelete
        getDeviceCloseActionCheck(event, capacity);
        if (needToScanDevices_) {
            needToScanDevices_ = false;
            needToSendFinishedDeviceScan_ = true;
        }
        // Do ActionAdd
        getDeviceOpenActionCheck(event, capacity);
        if (needToSendFinishedDeviceScan_) {
            needToSendFinishedDeviceScan_ = false;
            event->when = now;
            event->type = FINISHED_DEVICE_SCAN;
            event += 1;
            if (--capacity == INDEX0) {
                break;
            }
        }
        // next input event.
        deviceChanged_ = false;
        getDeviceTouchAction(event, capacity, readBuffer);
        // readNotify modify the list of devices
        if (pendingINotify_ && pendingEventIndex_ >= pendingEventCount_) {
            pendingINotify_ = false;
            readNotify();
            deviceChanged_ = true;
        }
        // added or removed devices happened.
        if (deviceChanged_) {
            continue;
        }
        // return now if we have collected any events or if we were explicitly awoken.
        if (event != buffer || awoken) {
            break;
        }
        // epoll for events.  mind the wake lock dance!
        pendingEventIndex_ = 0;
        lock_.unlock(); // release lock before poll, must be before release_wake_lock
        pollResult_ = epoll_wait(epollFd_, pendingEventItems_, EPOLL_MAX_EVENTS, timeoutMillis);
        lock_.lock(); // reacquire lock after poll, must be after acquire_wake_lock
        if (pollResult_ == INDEX0) {
            // timed out.
            HDF_LOGI("pollResult_ is zero or TimeOut");
            pendingEventCount_ = 0;
            break;
        }
        getDeviceActionResult();
    }
    // return the number of events happened.
    return event - buffer;
}

void InputDeviceManager::scanDevices()
{
    closeAllDevices();
    deviceConut_ = 1;
    int32_t result = scanDir(inputDevicePath_);
    if (result < INDEX0) {
        HDF_LOGE("scan dir failed for %{public}s", inputDevicePath_);
    }
    HDF_LOGI(" inputDevices size :%{public}d", inputDevices_.size());
}

int32_t InputDeviceManager::registerFdForEpoll(int fd)
{
    struct epoll_event eventItem = {};
    eventItem.events = EPOLLIN | EPOLLWAKEUP;
    eventItem.data.fd = fd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &eventItem)) {
        HDF_LOGE(" #Could not add fd to epoll instance: %{public}s", strerror(errno));
        return -errno;
    }
    return INPUT_SUCCESS;
}

int32_t InputDeviceManager::unregisterFdFromEpoll(int fd)
{
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr)) {
        HDF_LOGE(" Could not remove fd from epoll instance: %{public}s", strerror(errno));
        return -errno;
    }
    return INPUT_SUCCESS;
}

int32_t InputDeviceManager::registerDeviceForEpoll(InputDevice* device)
{
    if (device == nullptr) {
        HDF_LOGE("Cannot call registerDeviceForEpoll with null InputDevice");
        return INPUT_FAILURE;
    }
    int32_t result = registerFdForEpoll(device->nodeFd);
    if (result != INPUT_SUCCESS) {
        HDF_LOGE("Could not add input device fd to epoll for device %" PRId32, device->id);
        return result;
    }
    return result;
}

int32_t InputDeviceManager::unregisterDeviceFromEpoll(InputDevice* device)
{
    if (device->hasValidFd()) {
        int32_t result = unregisterFdFromEpoll(device->nodeFd);
        if (result != INPUT_SUCCESS) {
            HDF_LOGE("Could not remove input device fd from epoll for device %" PRId32, device->id);
            return result;
        }
    }
    return INPUT_SUCCESS;
}

void InputDeviceManager::createInputDevice(int fd,
                                 int32_t id,
                                 const std::string& path,
                                 const InputDeviceDetailInfo& detailInfo,
                                 int driverVersion )
{
    InputDevice* device = new InputDevice(fd, id, path, detailInfo);
    HDF_LOGD("  add device %{public}d: %{public}s\n", id, path.c_str());
    HDF_LOGD("  bus:        %{public}04x\n"
             "  vendor      %{public}04x\n"
             "  product     %{public}04x\n"
             "  version     %{public}04x\n",
             detailInfo.bus, detailInfo.vendor, detailInfo.product, detailInfo.version);
    HDF_LOGD("   name:       \"%{public}s\"\n", detailInfo.name.c_str());
    HDF_LOGD("   location:   \"%{public}s\"\n", detailInfo.location.c_str());
    HDF_LOGD("   unique id:  \"%{public}s\"\n", detailInfo.uniqueId.c_str());
    HDF_LOGD("  descriptor: \"%{public}s\"\n", detailInfo.descriptor.c_str());
    HDF_LOGD("  driver:     v%{public}d.%{public}d.%{public}d",
             driverVersion >> MV16, (driverVersion >> MV8) & 0xff, driverVersion & FMAX);

    // get the kinds of events the device reports.
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(device->devAbility.keyCode)), &device->devAbility.keyCode);
    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(device->devAbility.relCode)), &device->devAbility.relCode);
    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(device->devAbility.absCode)), &device->devAbility.absCode);
    ioctl(fd, EVIOCGBIT(EV_MSC, sizeof(device->devAbility.miscCode)), &device->devAbility.miscCode);
    ioctl(fd, EVIOCGBIT(EV_SW, sizeof(device->devAbility.switchCode)), &device->devAbility.switchCode);
    ioctl(fd, EVIOCGBIT(EV_LED, sizeof(device->devAbility.ledType)), &device->devAbility.ledType);
    ioctl(fd, EVIOCGBIT(EV_SND, sizeof(device->devAbility.soundCode)), &device->devAbility.soundCode);
    // ioctl(fd, EVIOCGBIT(EV_REP, sizeof(devAbility.)), device->ledBitmask);
    ioctl(fd, EVIOCGBIT(EV_FF, sizeof(device->devAbility.forceCode)), &device->devAbility.forceCode);
    // ioctl(fd, EVIOCGBIT(EV_PWR, sizeof(devAbility.)), device->ffBitmask);

    HDF_LOGD(" New device: id=%{public}d, fd=%{public}d, path='%{public}s', "
             " name='%{public}s', classes=0x%{public}x",
             id, fd, path.c_str(), device->detailInfo.name.c_str(), device->classes);
    addDevice(device);
}

int32_t InputDeviceManager::getInputDeviceDetailInfo(int& fd, InputDeviceDetailInfo& detailInfo)
{
    char buffer[DEVICEINFOSIZE];
    // device name.
    if (ioctl(fd, EVIOCGNAME(sizeof(buffer) - 1), &buffer) < 1) {
        HDF_LOGE("Could not get device name errormsg %{public}s", strerror(errno));
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
        detailInfo.name = buffer;
    }
    // device detailInfo.
    struct input_id inputId;
    if (ioctl(fd, EVIOCGID, &inputId)) {
        HDF_LOGE("could not get device input id errormsg %{public}s", strerror(errno));
        close(fd);
        return INPUT_FAILURE;
    }
    detailInfo.bus = inputId.bustype;
    detailInfo.product = inputId.product;
    detailInfo.vendor = inputId.vendor;
    detailInfo.version = inputId.version;
    // physical location.
    if (ioctl(fd, EVIOCGPHYS(sizeof(buffer)-1), &buffer) < 1) {
        HDF_LOGE("Get device physical location failed : errormsg %{public}s", strerror(errno));
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
        detailInfo.location = buffer;
    }
    // unique id.
    if (ioctl(fd, EVIOCGUNIQ(sizeof(buffer)-1), &buffer) < 1) {
        HDF_LOGE("Get device unique id failed : errormsg %{public}s", strerror(errno));
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
        detailInfo.uniqueId = buffer;
    }
    return INPUT_SUCCESS;
}

int32_t InputDeviceManager::openDevice(const char* devicePath)
{
    // get the device info
    HDF_LOGD("Opening device: %{public}s", devicePath);
    int fd = open(devicePath, O_RDWR | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) {
        HDF_LOGE("could not open %{public}s, %{public}s", devicePath, strerror(errno));
        return INPUT_FAILURE;
    }
    // device driver version.
    int driverVersion;
    if (ioctl(fd, EVIOCGVERSION, &driverVersion)) {
        HDF_LOGE("could not get driver version errormsg %{public}s", strerror(errno));
        close(fd);
        return INPUT_FAILURE;
    }
    // the descriptor.
    InputDeviceDetailInfo detailInfo {};
    int32_t ret = getInputDeviceDetailInfo(fd, detailInfo);
    if (ret != INPUT_SUCCESS) {
        return ret;
    }
    // abs Inof
    if (detailInfo.name == "goodix-ts") {
        for (int i = 0; i < ABS_CNT; i++) {
            if (ioctl(fd, EVIOCGABS(i), &infoABS_[i])) {
                HDF_LOGE(" Error reading absolute  get axis info failed fd= %{public}d name=%{public}s errormsg=%{public}s",
                         fd, detailInfo.name.c_str(),strerror(errno));
                continue;
            }
        }
    }
    assignDescriptor(detailInfo);
    // allocate device
    int32_t deviceIndex = deviceConut_++;
    enableDevice(deviceIndex);
    createInputDevice(fd, deviceIndex, devicePath, detailInfo, driverVersion);
    return INPUT_SUCCESS;
}

int32_t InputDeviceManager::enableDevice(int32_t deviceIndex)
{
    InputDevice* device = getDevice(deviceIndex);
    if (device == nullptr) {
        HDF_LOGE("Invalid device id=%{public}d provided to %{public}s", deviceIndex, __func__);
        return INPUT_FAILURE;
    }
    return registerDeviceForEpoll(device);
}

int32_t InputDeviceManager::disableDevice(int32_t deviceIndex)
{
    std::lock_guard<std::mutex> guard(lock_);
    InputDevice* device = getDevice(deviceIndex);
    if (device == nullptr) {
        HDF_LOGE("Invalid device id=%" PRId32 " provided to %{public}s", deviceIndex, __func__);
        return INPUT_FAILURE;
    }
    if (!device->enabled) {
        HDF_LOGD("Duplicate call to %{public}s, input device already disabled", __func__);
        return INPUT_SUCCESS;
    }
    unregisterDeviceFromEpoll(device);
    return device->disable();
}

void InputDeviceManager::addDevice(InputDevice* device)
{
    inputDevices_.insert_or_assign(device->id, device);
    device->next = openingDevices_;
    openingDevices_ = device;
}

void InputDeviceManager::closeDeviceByPath(const char *devicePath)
{
    InputDevice* device = getDeviceByPath(devicePath);
    if (device) {
        closeDevice(device);
        return;
    }
    HDF_LOGD("Remove device: %{public}s not found, device may already have been removed.", devicePath);
}

void InputDeviceManager::closeAllDevices()
{
    for (size_t i = INDEX0; i <= inputDevices_.size(); i++) {
        auto it = inputDevices_.find(i);
        if (it != inputDevices_.end()) {
            InputDevice* device = it->second;
            closeDevice(device);
        } else {
            continue;
        }
    }
}

void InputDeviceManager::closeDevice(InputDevice* device)
{
    deviceConut_--;
    HDF_LOGD("Removed device: path=%{public}s name=%{public}s id=%{public}d fd=%{public}d classes=0x%{public}x",
             device->path.c_str(), device->detailInfo.name.c_str(), device->id,
             device->nodeFd, device->classes);

    if (device->id == builtInKeyboardId_) {
        HDF_LOGD("built-in keyboard device %{public}s (id=%{public}d) is closing! the apps will not like this",
                 device->path.c_str(), builtInKeyboardId_);
        builtInKeyboardId_ = NO_BUILT_IN_KEYBOARD;
    }
    unregisterDeviceFromEpoll(device);
    inputDevices_.erase(device->id);
    device->close();
    InputDevice* pred = nullptr;
    bool found = false;
    for (InputDevice* entry = openingDevices_; entry != nullptr;) {
        if (entry == device) {
            found = true;
            break;
        }
        pred = entry;
        entry = entry->next;
    }
    if (found) {
        HDF_LOGD("InputDevice %{public}s was immediately closed after opening.", device->path.c_str());
        if (pred) {
            pred->next = device->next;
        } else {
            openingDevices_ = device->next;
        }
        delete device;
    } else {
        device->next = closingDevices_;
        closingDevices_ = device;
    }
}

int32_t InputDeviceManager::readNotify()
{
    HDF_LOGI("InputDeviceManager:%{public}s line: %{public}d", __func__, __LINE__);
    int res;
    char event_buf[EVENTBUFFERSIZE];
    int event_size;
    int event_pos = 0;
    struct inotify_event *event {};
    HDF_LOGD("InputDeviceManager::readNotify nfd: %{public}d", iNotifyFd_);
    res = read(iNotifyFd_, event_buf, sizeof(event_buf));
    HDF_LOGD("InputDeviceManager::readNotify read fd: %{public}d", res);
    if (res < (int)sizeof(*event)) {
        if (errno == EINTR) {
            return INPUT_SUCCESS;
        }
        HDF_LOGE("could not get event, %{public}s", strerror(errno));
        return INPUT_FAILURE;
    }
    while (res >= (int)sizeof(*event)) {
        event = (struct inotify_event *)(event_buf + event_pos);
        if (event->len) {
                std::string filename = std::string(inputDevicePath_) + "/" + std::string(event->name);
            if (event->wd == inputWd_) {
                if (event->mask & IN_CREATE) {
                    openDevice(filename.c_str());
                    readInputEvent(filename.c_str());
                } else if (event->mask & IN_DELETE) {
                    closeDeviceByPath(filename.c_str());
                } else {
                    // do nothing continue
                }
            }
        }
        event_size = sizeof(*event) + event->len;
        res -= event_size;
        event_pos += event_size;
    }
    return INPUT_SUCCESS;
}

int32_t InputDeviceManager::scanDir(const char *dirname)
{
    char devname[PATH_MAX];
    char *filename = nullptr;
    DIR *dir;
    struct dirent *de = nullptr;
    dir = opendir(dirname);
    if (dir == nullptr) {
        return INPUT_FAILURE;
    }
    strcpy_s(devname, PATH_MAX, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while ((de = readdir(dir))) {
        if (de->d_name[INDEX0] == '.' &&
            (de->d_name[INDEX1] == '\0' ||
            (de->d_name[INDEX1] == '.' && de->d_name[INDEX2] == '\0'))) {
            continue;
        }
        strcpy(filename, de->d_name);
        openDevice(devname);
        HDF_LOGI(" read devname:%{public}s, filename:%{public}s",devname, filename);
    }
    closedir(dir);
    for (size_t i = INDEX0; i <= inputDevices_.size(); i++) {
        auto it = inputDevices_.find(i);
        if (it != inputDevices_.end()) {
            const InputDevice* device = it->second;
            HDF_LOGD(" device id:%{public}d, path:%{public}s", device->id, device->path.c_str());
            if (device->detailInfo.name == "goodix-ts") {
                watchPath_ = device->path;
            }
        } else {
            continue;
        }
    }
    return INPUT_SUCCESS;
}

void InputDeviceManager::addToEpfd(int epfd, int fd)
{
    int ret;
    struct epoll_event event = {
        .events = EPOLLIN,
        .data = {
            .fd = fd,
        },
    };
    int flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        HDF_LOGD("set fd non-block error:%{public}d %{public}s", errno, strerror(errno));
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        HDF_LOGD("set fd non-block error:%{public}d %{public}s", errno, strerror(errno));
        return;
    }

    ret = epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event);
    HDF_LOGD("epoll_ctl fd: %{public}d ret: %{public}d error:%{public}d errorstd:%{public}s",
             fd, ret, errno, strerror(errno));
}

int32_t InputDeviceManager::readInputEvent(std::string filename)
{
    InputDevice* device = getDeviceByPath(filename.c_str());
    if (!device) {
        HDF_LOGI("device is null");
        return INPUT_FAILURE;
    }
    addToEpfd(epollFd_, device->nodeFd);
    return INPUT_SUCCESS;
}
}  // Input
}; // OHOS
