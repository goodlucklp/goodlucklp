#include "input_device_manager.h"
#include <iostream>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "string.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <fcntl.h>

namespace OHOS {
namespace Input {
using namespace std;
// get the nodefile list
void InputDeviceManager::EpollWatchThread()
{
    std::future<void> fuResult = std::async(std::launch::async, [this]() {
        while (true) {
            DoEpollWait();
        }
        return;
    });
    fuResult.get();
}

void InputDeviceManager::InotifyThread()
{
    std::future<void> fuResult = std::async(std::launch::async, [this]() {
        while (true) {
            InotifyThread();
        }
        return;
    });
    fuResult.get();
}

   InputDeviceManager::InputDeviceManager()
    {
        std::thread t1(&InputDeviceManager::DoEpollWait,this);
        thread_ = std::move(t1);
        thread_.detach();
        std::thread t2(&InputDeviceManager::InotifyThread, this);
        thread1_ = std::move(t2);
        thread1_.detach();
    };

vector<string> InputDeviceManager::GetFiles(string path)
{
    vector<string> fileList {};
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        cout<<"no files"<<endl;
        return fileList;
    }
    struct dirent* dEnt = nullptr;
    string sDot = ".";
    string sDotDot = "..";
    while ((dEnt = readdir(dir)) != nullptr) {
        if ((string(dEnt->d_name) != sDot) && (string(dEnt->d_name) != sDotDot)) {
            if (dEnt->d_type != DT_DIR) {
                string d_name(dEnt->d_name);
                fileList.push_back(string(dEnt->d_name));
            }
        }
    }
    // sort the returned files
    sort(fileList.begin(), fileList.end());
    return fileList;
}

// add epoll watch fd
void InputDeviceManager::AddEvent(int epollfd, int fd, int state)
{
    struct epoll_event ev {};
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

// delete epoll watch fd
void InputDeviceManager::DeleteEvent(int epollfd, int fd, int state)
{
    struct epoll_event ev {};
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

// modify epoll watch fd
void InputDeviceManager::ModifyEvent(int epollfd, int fd, int state)
{
    struct epoll_event ev {};
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

// read action
void InputDeviceManager::DoRead(int fd, struct input_event* event, size_t size)
{
    // printf("%s %d come in !!! fd is %d size is %ld\n", __func__, __LINE__, fd, size);
    int32_t readLen = read(fd, event, sizeof(struct input_event) * size);
    if (readLen == 0 || (readLen < 0 && errno == ENODEV)) {
        return;
    } else if (readLen < 0) {
        if (errno != EAGAIN && errno != EINTR) {
            printf("could not get event (errno=%d)\n", errno);
        }
    } else if ((readLen % sizeof(struct input_event)) != 0) {
        printf("could not get one event size %ld  readLen size: %d\n", sizeof(struct input_event), readLen);
    } else {
        size_t count = size_t(readLen) / sizeof(struct input_event);
        for (size_t i = 0; i < count; i++) {
            struct input_event& iEvent = event[i];
            printf("####type: %d code: %d value %d \n", iEvent.type, iEvent.code, iEvent.value );
        }
    }
}

// open input device node
int32_t InputDeviceManager::OpenInputDeivce(string devPath)
{
    // printf("%s %d come in !!! devPath is %s\n", __func__, __LINE__, devPath.c_str());
    int nodeFd = open(devPath.c_str(), O_RDWR | O_CLOEXEC | O_NONBLOCK);
    if (nodeFd < 0) {
        printf("could not open %s, %d %s", devPath.c_str(), errno, strerror(errno));
        return -1;
    }
    return nodeFd;
}

// close input device node
int32_t InputDeviceManager::CloseInputDeivce(string devPath)
{
    std::map<int32_t, InputDevListNode*> inputDevListMap = GetInputDeviceListMap();
    for (auto &e : inputDevListMap) {
        if (e.second->devPathNode == devPath) {
            int fd = e.second->fd;
            if (fd > 0) {
                nCloseListFd_.push_back(fd);
                close(fd);
            }
            DeleteEvent(nEpollFd_, fd, 2);
            e.second->state = 2;
            return INPUT_SUCCESS;
        }
    }
    // device list remove this node
    return INPUT_FAILURE;
}

// write action
void InputDeviceManager::DoWrite(int epollfd,int fd,char *buf)
{
    int nWrite {};
    nWrite = write(fd, buf, strlen(buf));
    if (nWrite == IPNUT_RET_NG) {
        perror("write error:");
        close(fd);
        DeleteEvent(epollfd, fd, EPOLLOUT);
    }
    else {
        ModifyEvent(epollfd, fd, EPOLLIN);
    }
    memset(buf, 0, MAX_SIZE);
}

// do with input events happed
void InputDeviceManager::HandleEvents(int epollFd,
                                      struct epoll_event *eventsList,
                                      int num,
                                      struct input_event* evtBuffer)
{
    int i {};
    int fd {};
    // do loop
    // printf("%s %d come in !!! epollFd is %d num is %d\n", __func__, __LINE__, epollFd, num);
    for (i = 0;i < num; i++) {
        fd = eventsList[i].data.fd;
        nDoWithEventIndex_++;
        if (fd == nNotifyFd_) {
            bDevicieChanged_ = true;
        }
        printf("####HandleEvents fd is %d events is 0x%x\n", fd, eventsList[i].events);
        // dispitch events
        if ((eventsList[i].events & EPOLLIN) || (eventsList[i].events & EPOLLET)) {
            // printf("%s %d reade actiong come in !!!\n", __func__, __LINE__);
            DoRead(fd, evtBuffer, EVENT_BUFFER_SIZE);
        }
        else if (eventsList[i].events & EPOLLHUP) {
            bDevicieChanged_ = true;
            printf("%s %d write action come in !!!\n", __func__, __LINE__);
            // DoWrite(epollFd, fd, buf);
        } else {
            // do nothing
        }
        if (bDevicieChanged_ && nDoWithEventIndex_ >= num) {
            // InotifyDoAction(nNotifyFd_, nInputWatchFd_, epollFd);
            bDevicieChanged_ = false;
        }
    }
}

// epoll watch file
void InputDeviceManager::DoEpollWatch(int nEpollFd, int listenFd)
{
    // printf("%s %d come in nEpollFd is %d listenFd is %d!!!\n", __func__, __LINE__, nEpollFd, listenFd);
    // add watch
    AddEvent(nEpollFd, listenFd, EPOLLIN);
    // close(nEpollFd);
}

// epoll watch file
void InputDeviceManager::DoEpollWait(void)
{
    int epollTimeOut {EPOLL_WAIT_TIMEOUT};
    char buf[MAX_SIZE] {};
    int ret {0};
    struct input_event evtBuffer[EVENT_BUFFER_SIZE];
    // get input device events
    nDoWithEventCount_ = epoll_wait(nEpollFd_, epollEventList_, EPOLL_EVENTS, epollTimeOut);
    if (nDoWithEventCount_ > 0) {
        // InotifyDoAction();
        HandleEvents(nEpollFd_, epollEventList_, nDoWithEventCount_, evtBuffer);
    }

}

void InputDeviceManager::InotifyDoAction(void)
{
    // printf("####%s %d come in !!!\n", __func__, __LINE__);
    int length {0};
    char buffer[EVENT_BUFFER_SIZE] {};
    // nNotifyFd_ = inotify_init();
    // nInputWatchFd_ = inotify_add_watch(nNotifyFd_, "/dev/input", IN_CREATE | IN_DELETE );
    if (nNotifyFd_ < 0) {
        printf("####%s %d come in !!!\n", __func__, __LINE__);
        perror("inotify_init");
    }
    struct inotify_event* event;
    struct inotify_event* end;
    length = read(nNotifyFd_, buffer, EVENT_BUFFER_SIZE);
    if (length < 0) {
        printf("####%s %d come in !!!\n", __func__, __LINE__);
        perror("read");
        return;
    }
    printf("####read length is %d\n", length);
    // Our current event pointer
    event = (struct inotify_event*) &buffer[0];
    // an end pointer so that we know when to stop looping below
    end = (struct inotify_event*) &buffer[length];
    printf("####%s %d come in event: %p end: %p !!!\n", __func__, __LINE__, event, end);
    while (event < end) {
        if (event->wd == nInputWatchFd_) {
            string nodeDevPath = string("/dev/input/") + string(event->name);
            printf("####nodeDevPath is %s \n", nodeDevPath.c_str());
            string::size_type n = nodeDevPath.find("mouse");
            if (n != std::string::npos) {
                if (event->mask & IN_CREATE) {
                    printf("####add action!!!\n");
                    auto fd = OpenInputDeivce(nodeDevPath);
                    if (fd > 0) {
                        nOpenListFd_.push_back(fd);
                        DoEpollWatch(nEpollFd_, fd);
                    } else {
                        printf("####OpenInputDeivce failed %d", fd);
                        break;
                    }
                } else if (event->mask & IN_DELETE) {
                    printf("####delete action!!!\n");
                    (void)CloseInputDeivce(nodeDevPath);
                } else {
                    // do nothing continue
                    printf("####others aciont!!!\n");
                    continue;
                }
            }
        }
        // move to the next event
        event = (struct inotify_event *)((char*)event) + sizeof(*event) + event->len;
    }
}

int32_t InputDeviceManager::GetInputDeviceInfo(int fd, DeviceInfo* detailInfo)
{
    int driverVersion {};
    char buffer[DEVICE_INFO_SIZE] {};
    struct input_id inputId {};
    // get the abilitys.
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(detailInfo->abilitySet.keyCode)), &detailInfo->abilitySet.keyCode);
    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(detailInfo->abilitySet.relCode)), &detailInfo->abilitySet.relCode);
    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(detailInfo->abilitySet.absCode)), &detailInfo->abilitySet.absCode);
    ioctl(fd, EVIOCGBIT(EV_MSC, sizeof(detailInfo->abilitySet.miscCode)), &detailInfo->abilitySet.miscCode);
    ioctl(fd, EVIOCGBIT(EV_SW, sizeof(detailInfo->abilitySet.switchCode)), &detailInfo->abilitySet.switchCode);
    ioctl(fd, EVIOCGBIT(EV_LED, sizeof(detailInfo->abilitySet.ledType)), &detailInfo->abilitySet.ledType);
    ioctl(fd, EVIOCGBIT(EV_SND, sizeof(detailInfo->abilitySet.soundCode)), &detailInfo->abilitySet.soundCode);
    ioctl(fd, EVIOCGBIT(EV_FF, sizeof(detailInfo->abilitySet.forceCode)), &detailInfo->abilitySet.forceCode);
    // device name.
    if (ioctl(fd, EVIOCGNAME(sizeof(buffer) - 1), &buffer) < 1) {
        printf("get device name failed errormsg %s\n", strerror(errno));
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
        strcpy(detailInfo->attrSet.devName, buffer);
        printf("get the devName: %s\n", detailInfo->attrSet.devName);
    }
    // device detailInfo.
    if (ioctl(fd, EVIOCGID, &inputId)) {
        printf("get device input id errormsg %s\n", strerror(errno));
        close(fd);
        return INPUT_FAILURE;
    }
    detailInfo->attrSet.id.busType = inputId.bustype;
    detailInfo->attrSet.id.product = inputId.product;
    detailInfo->attrSet.id.vendor = inputId.vendor;
    detailInfo->attrSet.id.version = inputId.version;
    printf("get the busType: %d product: %d vendor: %d version: %d\n",
           detailInfo->attrSet.id.busType,
           detailInfo->attrSet.id.product,
           detailInfo->attrSet.id.vendor,
           detailInfo->attrSet.id.version);
    // abs Inof
    if (detailInfo->attrSet.devName == "goodix-ts") {
        for (int i = 0; i < ABS_CNT; i++) {
            if (ioctl(fd, EVIOCGABS(i), &detailInfo->attrSet.axisInfo[i])) {
                printf("reading absolute  get axis info failed fd= %d name=%s errormsg=%s",
                       fd, detailInfo->attrSet.devName, strerror(errno));
                continue;
            }
        }
    }
    return INPUT_SUCCESS;
}
}
}
