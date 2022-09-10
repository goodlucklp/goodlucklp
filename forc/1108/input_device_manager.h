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

// #include "input_common.h"
#include "input_type.h"
#include <vector>
#include <sys/epoll.h>
// #include <refbase.h>
#include <thread>
#include <pthread.h>
#include <map>
#include <list>
#include <unistd.h>
#include <thread>
#include <functional>
#include <future>

namespace OHOS {
namespace Input {
using namespace std;
// manager the device node list
typedef struct ListNode_ {
    uint32_t   index;
    uint32_t   state;
    int32_t    fd;
    string     devPathNode;
    DeviceInfo* detailInfo;
} InputDevListNode;

#define EPOLL_EVENTS_MAX 32
#define MAX_SIZE     1024
#define EPOLL_EVENTS 100
#define FD_SIZE      1000
#define EPOLL_WAIT_TIMEOUT 0
#define INPUT_RET_NORMAL 0
#define IPNUT_RET_NG -1
#define EVENT_SIZE (sizeof (struct inotify_event))
#define EVENT_BUFFER_SIZE 512
#define DEVICE_INFO_SIZE 80

class InputDeviceManager
{
public:
    InputDeviceManager();
    virtual ~InputDeviceManager() = default;
    vector<string> GetFiles(string path);
    void AddEvent(int epollfd, int fd, int state);
    void DeleteEvent(int epollfd, int fd, int state);
    void ModifyEvent(int epollfd, int fd, int state);
    void DoRead(int fd, struct input_event* event, size_t size);
    int32_t OpenInputDeivce(string devPath);
    int32_t CloseInputDeivce(string devPath);
    void DoWrite(int epollfd,int fd,char *buf);
    void HandlAaccpet(int epollfd,int listenfd);
    void HandleEvents(int epollFd,
                      struct epoll_event *events,
                      int num,
                      struct input_event* evtBuffer);
    void DoEpollWatch(int nEpollFd, int listenFd);
    void DoEpollWait(void);
    void InotifyDoAction(void);
    int32_t GetInputDeviceInfo(int fd, DeviceInfo* detailInfo);
    void EpollWatchThread();
    void InotifyThread();
    std::map<int32_t, InputDevListNode*> GetInputDeviceListMap()
    {
        return inputDevList_;
    }
    void SetInputEpollFd(int32_t& fd)
    {
        nEpollFd_ = fd;
    }
    void SetInputNotifyFd(int32_t& fd)
    {
        nNotifyFd_ = fd;
    }
    void SetInputWatchFd(int32_t& fd)
    {
        nInputWatchFd_ = fd;
    }
    int32_t GetInputNotifyFd(void)
    {
        return nNotifyFd_;
    }
    int32_t GetInputEpollFd(void)
    {
        return nEpollFd_;
    }
    int32_t GetInputWatchFd(void)
    {
        return nInputWatchFd_;
    }
    void dumpInfoList(InputDevListNode& in)
    {
        printf("index: %u state:%u fd:%d devPathNode: %s\n",
                in.index, in.state, in.fd, in.devPathNode.c_str());

        if (in.detailInfo) {
            printf("devIndex: %u devType:%u chipInfo:%s vendorName: %s chipName: %s attrSet.devName: %s\n"
                    "attrSet.id.busType: %u attrSet.id.vendor: %u attrSet.id.product: %u attrSet.id.version: %u\n",
                    in.detailInfo->devIndex, in.detailInfo->devType, in.detailInfo->chipInfo, in.detailInfo->vendorName,
                    in.detailInfo->chipName, in.detailInfo->attrSet.devName, in.detailInfo->attrSet.id.busType,
                    in.detailInfo->attrSet.id.vendor, in.detailInfo->attrSet.id.product, in.detailInfo->attrSet.id.version);
            for (int i = 0; i < ABS_CNT; i++ ) {
                printf("attrSet.axisInfo.axis: %d attrSet.axisInfo.min: %d attrSet.axisInfo.max: %d attrSet.axisInfo.fuzz: %d\n"
                    "attrSet.axisInfo.flat: %d attrSet.axisInfo.range: %d \n",
                    in.detailInfo->attrSet.axisInfo[i].axis, in.detailInfo->attrSet.axisInfo[i].flat,
                    in.detailInfo->attrSet.axisInfo[i].fuzz, in.detailInfo->attrSet.axisInfo[i].max,
                    in.detailInfo->attrSet.axisInfo[i].min, in.detailInfo->attrSet.axisInfo[i].range);
            }
        }
    }
private:
    std::map<int32_t, InputDevListNode*> inputDevList_;
    int32_t nNotifyFd_ {0};
    int32_t nInputWatchFd_ {0};
    int32_t nEpollFd_ {0};
    bool bDevicieChanged_ {false};
    int32_t nDoWithEventIndex_ {0};
    int32_t nDoWithEventCount_ {0};
    list<int32_t> nOpenListFd_ {0};
    list<int32_t> nCloseListFd_ {0};
    struct epoll_event epollEventList_[EPOLL_EVENTS_MAX] {};
    std::thread thread_;
    std::thread thread1_;
};
}
}
#endif // INPUT_DEVICE_MANAGER_H
