#include "input_device_manager.h"
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <string.h>
#include <memory>
using namespace OHOS::Input;
using namespace std;
int main(void)
{
    string devPath = "/dev/input";
    uint32_t devIndex = 1;
    InputDeviceManager* inputDeviceManager = new InputDeviceManager();
    std::vector<std::string> flist = inputDeviceManager->GetFiles(devPath);
    int nEpollFd {0};
    int nNotifyFd {0};
    int nInputWatchFd {0};
    nEpollFd = epoll_create1(EPOLL_CLOEXEC);
    nNotifyFd = inotify_init();
    inputDeviceManager->SetInputEpollFd(nEpollFd);
    inputDeviceManager->SetInputNotifyFd(nNotifyFd);
    string inputDevicePath = "/dev/input";
    nInputWatchFd = inotify_add_watch(nNotifyFd, inputDevicePath.c_str(), IN_CREATE | IN_DELETE );
    inputDeviceManager->SetInputWatchFd(nInputWatchFd);
    // create epoll fd
    std::shared_ptr<DeviceInfo> detailInfo = std::make_shared<DeviceInfo>();
    printf("flist size %ld\n", flist.size());
    for (unsigned i=0; i < flist.size(); i++) {
        printf("%s\n", flist[i].c_str());
        string devPathNode = devPath + "/" + flist[i];
        std::string::size_type n = devPathNode.find("event");
        if (n != std::string::npos) {
            auto fd = inputDeviceManager->OpenInputDeivce(devPathNode);
            if (fd < 0) {
                printf("opend node failed !!! line %d\n", __LINE__);
            }
            auto ret = inputDeviceManager->GetInputDeviceInfo(fd, detailInfo.get());
            if (ret != INPUT_SUCCESS) {
                printf("get input device info failed !!! line %d\n", __LINE__);
                return ret;
            }
            auto inputDevList = std::make_shared<InputDevListNode>();
            inputDevList->index = devIndex;
            inputDevList->state = 0;
            inputDevList->fd = fd;
            inputDevList->devPathNode = devPathNode;
            inputDevList->detailInfo = detailInfo.get();
            std::map<int32_t, InputDevListNode*> inputDevListMap = inputDeviceManager->GetInputDeviceListMap();
            inputDevListMap[devIndex] = inputDevList.get();
            inputDeviceManager->DoEpollWatch(nEpollFd, fd);
        }
    }
    inputDeviceManager->EpollWatchThread();
    inputDeviceManager->InotifyThread();
    return 0;
}
