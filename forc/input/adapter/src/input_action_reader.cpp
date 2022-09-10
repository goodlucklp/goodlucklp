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

#include "input_action_reader.h"
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include "securec.h"
#include <unistd.h>
#include <iostream>
#include <filesystem>
using namespace std::filesystem;
using namespace std::chrono;
using namespace std;
#define HDF_LOG_TAG InputActionReader

namespace OHOS {
namespace Input {
void InputActionReader::workerThread()
{
    std::future<void> fuResult = std::async(std::launch::async, [this]() {
        while (true) {
            if (callbackFlag_) {
                loopOnce();
            }
        }
        return;
    });
    fuResult.get();
}

void InputActionReader::loopOnce()
{
    int32_t timeoutMillis = -1;
    size_t count = inputDeviceManger_->getActions(timeoutMillis, eventBuffer_, EVENT_BUFFER_SIZE);
    if (count) {
        ProcessEvents(eventBuffer_, count);
    }
}

void InputActionReader::ProcessEvents(const devicAction* rawEvents, size_t count)
{
    if (rawEvents->value == 0 || rawEvents->value == 1 || rawEvents->value == -1) {
        HDF_LOGD("processEvents: type=%{public}d Count=%{public}zu "
                 "code=%{public}d value=%{public}d deviceIndex=%{public}d",
                 rawEvents->type, count,rawEvents->code,rawEvents->value,rawEvents->deviceIndex);
    }
    for (const devicAction* rawEvent = rawEvents; count;) {
        size_t batchSize = INDEX1;
        int32_t deviceIndex = rawEvent->deviceIndex;
        while (batchSize < count) {
            if (rawEvent[batchSize].type >= InputDeviceManagerInterface::FIRST_SYNTHETIC_EVENT
                    || rawEvent[batchSize].deviceIndex != deviceIndex) {
                break;
            }
            batchSize += 1;
        }
        ProcessEventsForDevice(deviceIndex, rawEvent, batchSize);
        count -= batchSize;
        rawEvent += batchSize;
    }
}

void InputActionReader::ProcessEventsForDevice(int32_t deviceIndex,
                                               const devicAction* rawEvents,
                                               size_t count)
{
    HDF_LOGD("InputActionReader::%{public}s: called deviceIndex:%{public}d count:%{public}u "
             "rawEvents->type%{public}d rawEvents->code%{public}d rawEvents->value%{public}d",
             __func__, deviceIndex, count, rawEvents->type, rawEvents->code, rawEvents->value);
    if (rawEvents->type == InputDeviceManagerInterface::DEVICE_ADDED ||
        rawEvents->type == InputDeviceManagerInterface::DEVICE_REMOVED ||
        rawEvents->type == InputDeviceManagerInterface::FINISHED_DEVICE_SCAN ) {
        // hot plug evnets happend
        HotPlugEvent* evtPlusPkg = (HotPlugEvent*)malloc(sizeof(HotPlugEvent));
        for (uint32_t i = INDEX0; i<count; i++) {
            evtPlusPkg->devType = (rawEvents + i)->type;
            evtPlusPkg->devIndex = deviceIndex;
            evtPlusPkg->status = 1;
            HDF_LOGD("deviceIndex:%{public}d count:%{public}u ", deviceIndex, count);
            if (reportHotPlugEventCallback_ != nullptr) {
                reportHotPlugEventCallback_->HotPlugCallback(evtPlusPkg);
            }
        }
        free(evtPlusPkg);
        evtPlusPkg = nullptr;
    } else {
        // device action events happend
        EventPackage** evtPkg = (EventPackage**)malloc(sizeof(int) * count);
        for (uint32_t i=INDEX0; i<count; i++) {
            *(evtPkg + i) = (EventPackage*)malloc(sizeof(EventPackage));
            evtPkg[i]->type = (rawEvents + i)->type;
            evtPkg[i]->code = (rawEvents + i)->code;
            evtPkg[i]->value = (rawEvents + i)->value;
            evtPkg[i]->timestamp = (rawEvents + i)->when;
            HDF_LOGD(" InputActionReader::%{public}s: called deviceIndex:%{public}d count:%{public}u "
                     "type%{public}d code%{public}d value%{public}d",
                     __func__, deviceIndex, count, evtPkg[i]->type, evtPkg[i]->code, evtPkg[i]->value);
        }
        for (auto &e : reportEventPkgCallback_) {
            HDF_LOGD(" reportEventPkgCallback_ first :%{public}d second:%{public}p ", e.first, e.second);
            if (e.second != nullptr) {
                HDF_LOGD(" reportCallback come in !!!!");
                e.second->EventPkgCallback(const_cast<const EventPackage**>(evtPkg), count, deviceIndex);
            }
        }
        free(evtPkg);
        evtPkg = nullptr;
    }
}

RetStatus InputActionReader::Init(void)
{
    RetStatus rc = INPUT_SUCCESS;
    inputDeviceManger_ = new InputDeviceManager();
    std::thread t1(std::bind(&InputActionReader::workerThread, this));
    std::string wholeName1 = std::to_string(getpid()) + "_" + std::to_string(gettid());
    thread_ = std::move(t1);
    thread_.detach();
    if (!inputDeviceManger_) {
        HDF_LOGE("InputActionReader::%{public}s: fail to creatd eventhub", __func__);
        return INPUT_FAILURE;
    }
   	// path devPath("/dev/input");
	// if (!exists(devPath)) {
	// 	return INPUT_FAILURE;
	// }
	// //path entry
	// directory_entry entry(devPath);	
	// if (entry.status().type() == file_type::directory) {
	// 	cout << "this is a path" << endl;
	// }
	// // file list
	// directory_iterator list(devPath);
	// for (auto& it:list) {
	// 	cout << it.path().filename()<< endl;
	// }     


	vector<string> fileNames;
	string path("D:\\test"); 	//自己选择目录测试
	getFileNames(path, fileNames);
	for (const auto &ph : fileNames) {
		std::cout << ph << "\n";
	}
    
	intptr_t hFile = 0;
	//文件信息
	struct _finddata_t fileinfo;
	string p;
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//如果是目录,递归查找
			//如果不是,把文件绝对路径存入vector中
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					getFileNames(p.assign(path).append("\\").append(fileinfo.name), files);
			}
			else
			{
				files.push_back(p.assign(path).append("\\").append(fileinfo.name));
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
    }
    return rc;
}
// InputManager
RetStatus InputActionReader::ScanDevice(void)
{
    RetStatus rc = INPUT_SUCCESS;
    inputDeviceManger_->scanDevices();
    HDF_LOGD("InputActionReader %{public}s called after line: %{public}d", __func__, __LINE__);
    return rc;
}

RetStatus InputActionReader::OpenDevice(int32_t devIndex)
{
    HDF_LOGI("InputActionReader::%{public}s:called start devIndex: %{public}d", __func__, devIndex);
    RetStatus rc = INPUT_SUCCESS;
    if (inputDeviceManger_->enableDevice(devIndex) != INPUT_SUCCESS) {
        HDF_LOGE("InputActionReader %{public}s called failed line: %{public}d devIndex: %{public}d",
                 __func__, __LINE__, devIndex);
        return INPUT_FAILURE;
    }
    HDF_LOGI("InputActionReader %{public}s called end line: %{public}d devIndex: %{public}d",
             __func__, __LINE__, devIndex);
    return rc;
}

RetStatus InputActionReader::CloseDevice(int32_t devIndex)
{
    RetStatus rc = INPUT_SUCCESS;
    if (inputDeviceManger_->disableDevice(devIndex) != INPUT_SUCCESS) {
        HDF_LOGE("InputActionReader %{public}s called failed line: %{public}d devIndex: %{public}d",
                 __func__, __LINE__, devIndex);
        return INPUT_FAILURE;
    }
    return rc;
}

RetStatus InputActionReader::ConvertDeviceInfo(InputDevice* inInfo, DeviceInfo* outInfo)
{
    RetStatus rc = INPUT_SUCCESS;
    if (inInfo == nullptr || outInfo == nullptr) {
        HDF_LOGE("InputActionReader %{public}s inInfo is null failed", __func__);
        rc = INPUT_FAILURE;
        return rc;
    }
    outInfo->devIndex = inInfo->id;
    outInfo->devType = inInfo->classes;
    memcpy_s(outInfo->chipInfo, CHIP_INFO_LEN, inInfo->detailInfo.location.c_str(), CHIP_INFO_LEN);
    memcpy_s(outInfo->vendorName, VENDOR_NAME_LEN, inInfo->detailInfo.name.c_str(), VENDOR_NAME_LEN);
    memcpy_s(outInfo->chipName, CHIP_NAME_LEN, inInfo->detailInfo.uniqueId.c_str(), CHIP_NAME_LEN);
    memcpy_s(outInfo->attrSet.devName, DEV_NAME_LEN, inInfo->detailInfo.descriptor.c_str(), CHIP_NAME_LEN);
    outInfo->attrSet.id.busType = inInfo->detailInfo.bus;
    outInfo->attrSet.id.vendor = inInfo->detailInfo.vendor;
    outInfo->attrSet.id.product = inInfo->detailInfo.product;
    outInfo->attrSet.id.version = inInfo->detailInfo.version;
    for (int i = 0; i < ABS_CNT; i++) {
        HDF_LOGE(" InputActionReader getAbsoluteAxisInfo successful line: %{public}d "
                 " axis:%{public}d minValue:%{public}d "
                 " maxValue:%{public}d flat%{public}d fuzz%{public}d resolution:%{public}d ",
                 __LINE__,
                 (inputDeviceManger_->getABSInof() + i)->value,
                 (inputDeviceManger_->getABSInof() + i)->minimum,
                 (inputDeviceManger_->getABSInof() + i)->maximum,
                 (inputDeviceManger_->getABSInof() + i)->flat,
                 (inputDeviceManger_->getABSInof() + i)->fuzz,
                 (inputDeviceManger_->getABSInof() + i)->resolution);
        outInfo->attrSet.axisInfo[i].axis = (inputDeviceManger_->getABSInof() + i)->value;
        outInfo->attrSet.axisInfo[i].min = (inputDeviceManger_->getABSInof() + i)->minimum;
        outInfo->attrSet.axisInfo[i].max = (inputDeviceManger_->getABSInof() + i)->maximum;
        outInfo->attrSet.axisInfo[i].flat = (inputDeviceManger_->getABSInof() + i)->flat;
        outInfo->attrSet.axisInfo[i].fuzz = (inputDeviceManger_->getABSInof() + i)->fuzz;
        outInfo->attrSet.axisInfo[i].range = (inputDeviceManger_->getABSInof() + i)->resolution;
    }
    memcpy_s(&outInfo->abilitySet,
             sizeof(outInfo->abilitySet),
             &inInfo->devAbility,
             sizeof(outInfo->abilitySet));
    HDF_LOGD("InputActionReader::%{public}s devIndex: %{public}d, devTyp: %{public}d line%{public}d",
             __func__, outInfo->devIndex, outInfo->devType, __LINE__);
    return rc;
}

RetStatus InputActionReader::GetDevice(int32_t devIndex, DeviceInfo **devInfo)
{
    InputDevice* device = inputDeviceManger_->getDevice(devIndex);
    if (device == nullptr) {
        HDF_LOGE("InputActionReader::%{public}s:  called failed line: %{public}d devIndex: %{public}d",
                 __func__, __LINE__, devIndex);
        return INPUT_FAILURE;
    }
    HDF_LOGD("InputActionReader::%{public}s:  called line: %{public}d devIndex: %{public}d",
             __func__, __LINE__, devIndex);
    return ConvertDeviceInfo(device, *devInfo);
}

RetStatus InputActionReader::GetDeviceList(uint32_t *devNum, DeviceInfo **deviceList, uint32_t size)
{
    RetStatus rc = INPUT_SUCCESS;
    inputDeviceList_.clear();
    uint32_t devCount = INDEX0;
    InputDevice **device = inputDeviceManger_->getDeviceList();
    if (device == nullptr) {
        HDF_LOGE("InputActionReader::%{public}s:called failed line %{public}d", __func__, __LINE__);
        return INPUT_NOMEM;
    }
    for (uint32_t i = INDEX0; i < inputDeviceManger_->getDeviceListCnt(); i++) {
        devCount++;
        ConvertDeviceInfo((*device) + i, (*deviceList) + i);
        HDF_LOGD("InputActionReader %{public}s: called line:%{public}d index:%{public}d type:%{public}d",
                 __func__, __LINE__, (*deviceList+i)->devIndex, (*deviceList+i)->devType);
        inputDeviceList_.emplace((*deviceList + i)->devIndex, *(*deviceList + i));
    }
    HDF_LOGI("InputActionReader::%{public}s:called line %{public}d inputDeviceList_.size: %{public}d",
             __func__, __LINE__, inputDeviceList_.size());
    for (uint32_t i = INDEX0; i < inputDeviceList_.size(); i++) {
        HDF_LOGD("InputActionReader %{public}s: called line:%{public}d index:%{public}d type:%{public}d",
                 __func__, __LINE__, (*deviceList+i)->devIndex, (*deviceList+i)->devType);
    }
    *devNum = devCount;
    HDF_LOGD("DevCnt %{public}d", *devNum);
    return rc;
}

// InputController
RetStatus InputActionReader::SetPowerStatus(uint32_t devIndex, uint32_t status)
{
    RetStatus rc = INPUT_SUCCESS;
    return rc;
}

RetStatus InputActionReader::GetPowerStatus(uint32_t devIndex, uint32_t *status)
{
    RetStatus rc = INPUT_SUCCESS;
    return rc;
}

RetStatus InputActionReader::GetDeviceType(uint32_t devIndex, uint32_t *deviceType)
{
    RetStatus rc = INPUT_SUCCESS;
    *deviceType = inputDeviceList_[devIndex].devType;
    HDF_LOGD("devType:%{public}d", *deviceType);
    return rc;
}

RetStatus InputActionReader::GetChipInfo(uint32_t devIndex, char *chipInfo, uint32_t length)
{
    RetStatus rc = INPUT_SUCCESS;
    memcpy_s(chipInfo, length, inputDeviceList_[devIndex].chipInfo, length);
    HDF_LOGD("chipInfo:%{public}s", chipInfo);
    return rc;
}

RetStatus InputActionReader::GetVendorName(uint32_t devIndex, char *vendorName, uint32_t length)
{
    RetStatus rc = INPUT_SUCCESS;
    memcpy_s(vendorName, length, inputDeviceList_[devIndex].vendorName, length);
    HDF_LOGD("vendorName:%{public}s", vendorName);
    return rc;
}
RetStatus InputActionReader::GetChipName(uint32_t devIndex, char *chipName, uint32_t length)
{
    RetStatus rc = INPUT_SUCCESS;
    memcpy_s(chipName, length, inputDeviceList_[devIndex].chipName, length);
    HDF_LOGD("chipName:%{public}s", chipName);
    return rc;
}

RetStatus InputActionReader::SetGestureMode(uint32_t devIndex, uint32_t gestureMode)
{
    RetStatus rc = INPUT_SUCCESS;
    return rc;
}

RetStatus InputActionReader::RunCapacitanceTest(uint32_t devIndex, uint32_t testType, char *result, uint32_t length)
{
    RetStatus rc = INPUT_SUCCESS;
    return rc;
}

RetStatus InputActionReader::RunExtraCommand(uint32_t devIndex, InputExtraCmd *cmd)
{
    RetStatus rc = INPUT_SUCCESS;
    return rc;
}

// InputReporter
RetStatus InputActionReader::RegisterReportCallback(uint32_t devIndex, InputEventCb* callback)
{
    RetStatus rc = INPUT_SUCCESS;
    reportEventPkgCallback_[devIndex] = callback;
    callbackFlag_ = true;
    return rc;
}

RetStatus InputActionReader::UnregisterReportCallback(uint32_t devIndex)
{
    HDF_LOGI("devIndex: %{public}d ", devIndex);
    RetStatus rc = INPUT_SUCCESS;
    reportEventPkgCallback_[devIndex] = nullptr;
    callbackFlag_ = false;
    return rc;
}

RetStatus InputActionReader::RegisterHotPlugCallback(InputHostCb* callback)
{
    RetStatus rc = INPUT_SUCCESS;
    reportHotPlugEventCallback_ = callback;
    HDF_LOGI("InputActionReader::%{public}s:called line %{public}d ", __func__, __LINE__);
    return rc;
}

RetStatus InputActionReader::UnregisterHotPlugCallback(void)
{
    RetStatus rc = INPUT_SUCCESS;
    reportHotPlugEventCallback_ = nullptr;
    HDF_LOGI("InputActionReader::%{public}s:called line %{public}d ", __func__, __LINE__);
    return rc;
}
}
}; // end namespace OHOS::Input

