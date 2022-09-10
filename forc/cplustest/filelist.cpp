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
#include <linux/input.h>
#include <fcntl.h>

using namespace std;

#define MAX_SIZE     1024
#define EPOLL_EVENTS 100
#define FD_SIZE      1000
#define EPOLL_WAIT_TIMEOUT -1
#define INPUT_RET_NORMAL 0
#define IPNUT_RET_NG -1
#define EVENT_SIZE (sizeof (struct inotify_event))
#define BUF_LEN  (1024 * (EVENT_SIZE + 16))
#define EVENT_BUFFER_SIZE 256

vector<string> GetFiles(string path);
// get the nodefile list
vector<string> GetFiles(string path)
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
static void AddEvent(int epollfd, int fd, int state)
{
    struct epoll_event ev {};
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

// delete epoll watch fd
static void DeleteEvent(int epollfd, int fd, int state)
{
    struct epoll_event ev {};
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

// modify epoll watch fd
static void ModifyEvent(int epollfd, int fd, int state)
{
    struct epoll_event ev {};
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

// read action
static void DoRead(int fd, struct input_event* event, size_t size)
{
    printf("%s %d come in !!! fd is %d size is %ld\n", __func__, __LINE__, fd, size);
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
            printf("type: %d code: %d value %d \n", iEvent.type, iEvent.code, iEvent.value );
        }
    }
}

// open input device node
int32_t OpenInputDeivce(string devPath)
{
    printf("%s %d come in !!! devPath is %s\n", __func__, __LINE__, devPath.c_str());
    int nodeFd = open(devPath.c_str(), O_RDWR | O_CLOEXEC | O_NONBLOCK);
    if (nodeFd < 0) {
        printf("could not open %s, %d %s", devPath.c_str(), errno, strerror(errno));
        return -1;
    }
    return nodeFd;
}

// close input device node
int32_t CloseInputDeivce(int nodeFd, string devPath)
{
    if (nodeFd > 0) {
        close(nodeFd);
        nodeFd = -1;
    }
    // device list remove this node 
    
    return 0;
}

// write action
static void DoWrite(int epollfd,int fd,char *buf)
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

static void HandlAaccpet(int epollfd,int listenfd)
{

}

// do with input events happed
static void HandleEvents(int epollFd,
                         struct epoll_event *events,
                         int num,
                         int listenFd,
                         struct input_event* evtBuffer)
{
    int i {};
    int fd {};
    // do loop
    printf("%s %d come in !!! epollFd is %d num is %d listenFd is %d\n", __func__, __LINE__, epollFd, num, listenFd);
    for (i = 0;i < num; i++) {
        fd = events[i].data.fd;
        // dispitch events
        if ((events[i].events & EPOLLIN) || (events[i].events & EPOLLET)) {
            printf("%s %d reade actiong come in !!!\n", __func__, __LINE__);
            DoRead(fd, evtBuffer, EVENT_BUFFER_SIZE);
        }
        else if (events[i].events & EPOLLOUT) {
            printf("%s %d write action come in !!!\n", __func__, __LINE__);
            // DoWrite(epollFd, fd, buf);
        } else {
            // do nothing
        }
    }
}

// epoll watch file
static void DoEpollWatch(int nEpollFd, int listenFd)
{
    printf("%s %d come in nEpollFd is %d listenFd is %d!!!\n", __func__, __LINE__, nEpollFd, listenFd);
    int epollTimeOut {EPOLL_WAIT_TIMEOUT};
    struct epoll_event events[EPOLL_EVENTS] {};
    int ret {0};
    char buf[MAX_SIZE] {};
    struct input_event evtBuffer[EVENT_BUFFER_SIZE];
    // add watch
    AddEvent(nEpollFd, listenFd, EPOLLIN);
    while (true) {
        // get input device events
        ret = epoll_wait(nEpollFd, events, EPOLL_EVENTS, epollTimeOut);
        if (ret > 0) {
            HandleEvents(nEpollFd, events, ret, listenFd, evtBuffer);
        }
    }
    close(nEpollFd);
}

static void InotifyDoAction(int& nNotifyFd, int nWacthId)
{
    printf("%s %d come in !!!\n", __func__, __LINE__);
    int length {0};
    int i      {0};
    int wd[32] {0};
    char buffer[BUF_LEN] {};
    if (nNotifyFd < 0) {
        perror("inotify_init");
    }
    while (true) {
        struct inotify_event* event;
        struct inotify_event* end;
        length = read(nNotifyFd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
            break;
        }
        // Our current event pointer
        event = (struct inotify_event*) &buffer[0];
        // an end pointer so that we know when to stop looping below
        end = (struct inotify_event*) &buffer[length];
        while (event < end) {
            if (event->wd == nWacthId) {
                if (event->mask & IN_CREATE) {
                    printf("add action!!!\n");
                } else if (event->mask & IN_DELETE) {
                    printf("delete action!!!\n");
                } else {
                    // do nothing continue
                    printf("others aciont!!!\n");
                }
            }
            // Do your printing or whatever here
            printf("name is %s\n", event->name);
            // Now move to the next event
            event = (struct inotify_event *)((char*)event) + sizeof(*event) + event->len;
        }
    }
    // (void) inotify_rm_watch(fd, wd[0]);
}

static void InotifyCreate(int& nNotifyFd, int& inputWd) {
    string inputDevicePath = "/dev/input";
    inputWd = inotify_add_watch(nNotifyFd, inputDevicePath.c_str(), IN_ALL_EVENTS );
}

// manager the device node list
typedef struct ListNode_ {
    ListNode_* nexet;
    uint32_t   index;
    uint32_t   state;
    int32_t    fd;
    string     devNodePath;
} ListNode;

using inputDeviceNodeList = ListNode* ;



int main()
{
    std::string devPath = "/dev/input";
    std::string nodeName = "event2";
    std::vector<std::string> flist = GetFiles(devPath);
    int nEpollFd {0};
    int nNotifyFd {0};
    int nInputDd {0};
    printf("flist size %ld\n", flist.size());
    for (unsigned i=0; i < flist.size(); i++) {
        printf("%s\n", flist[i].c_str());
    }
    auto fd = OpenInputDeivce(devPath + "/" + nodeName);
    if (fd < 0) {
        printf("opend node failed !!! line %d\n", __LINE__);

    }
    nNotifyFd = inotify_init();
    InotifyCreate(nNotifyFd, nInputDd);
    // create epoll fd
    nEpollFd = epoll_create1(EPOLL_CLOEXEC);
    DoEpollWatch(nEpollFd, fd);
    InotifyDoAction(nNotifyFd, nInputDd);
    // (void) close(nNotifyFd);
    // exit(0);
    return 0;
}

 