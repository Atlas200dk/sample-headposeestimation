/*
 *   =======================================================================
 *   Copyright (C), 2018, Huawei Tech. Co., Ltd.

 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at

 *       http://www.apache.org/licenses/LICENSE-2.0

 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *   =======================================================================
 */

#include "Mind_Camera.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cstring>
#include <chrono>

#include "hiaiengine/log.h"
extern "C" {
#include "driver/peripheral_api.h"
}

using hiai::Engine;
using namespace hiai;
using namespace std;

#define MAX_IMAGE_SIZE (4096*4096)

namespace {
// initial value of frameId
const uint32_t kInitFrameId = 0;
}


Mind_Camera::Mind_Camera() {
    config_ = nullptr;
    frame_id_ = kInitFrameId;
    exit_flag_ = CAMERADATASETS_INIT;
    InitConfigParams();
}

Mind_Camera::~Mind_Camera() {
}

std::string Mind_Camera::CameraDatasetsConfig::ToString() const {
    stringstream log_info_stream("");
    log_info_stream << "fps:" << this->fps << ", camera:" << this->channel_id
                  << ", image_format:" << this->image_format
                  << ", resolution_width:" << this->resolution_width
                  << ", resolution_height:" << this->resolution_height;
    return log_info_stream.str();
}

HIAI_StatusT Mind_Camera::Init(
        const hiai::AIConfig& aiConfig,
        const std::vector<hiai::AIModelDescription>& modelDesc) {
    HIAI_ENGINE_LOG("[Mind_Camera] start init!");
    if (config_ == nullptr) {
            config_ = make_shared<CameraDatasetsConfig>();
    }

    for (int index = 0; index < aiConfig.items_size(); ++index) {
        const ::hiai::AIConfigItem& item = aiConfig.items(index);
        std::string name = item.name();
        std::string value = item.value();

        if (name == "fps") {
            config_->fps = atoi(value.data());
        } else if (name == "image_format") {
            config_->image_format = CommonParseParam(value);
        } else if (name == "data_source") {
            config_->channel_id = CommonParseParam(value);
        } else if (name == "image_size") {
            ParseImageSize(value, config_->resolution_width,
                           config_->resolution_height);
        } else {
            HIAI_ENGINE_LOG("unused config name: %s", name.c_str());
        }
    }

    HIAI_StatusT ret = HIAI_OK;
    bool failed_flag = (config_->image_format == PARSEPARAM_FAIL ||
                       config_->channel_id == PARSEPARAM_FAIL ||
                       config_->resolution_width <= 0 ||
                       config_->resolution_height <= 0);

    if (failed_flag) {
        std::string msg = config_->ToString();
        msg.append(" config_ data failed");
        HIAI_ENGINE_LOG(msg.data());
        ret = HIAI_ERROR;
    }

    HIAI_ENGINE_LOG("[Mind_Camera] end init!");
    return ret;
}

void Mind_Camera::InitConfigParams() {
    params_.insert(std::pair<std::string,std::string>
                   ("Channel-1", IntToString(CAMERAL_1)));
    params_.insert(std::pair<std::string, std::string>
                   ("Channel-2", IntToString(CAMERAL_2)));
    params_.insert(std::pair<std::string, std::string>
                   ("YUV420SP", IntToString(CAMERA_IMAGE_YUV420_SP)));
}

std::string Mind_Camera::IntToString(int value) {
    char msg[MAX_VALUESTRING_LENGTH] = {0};
    std::string ret;

    int retCode = sprintf_s(msg, MAX_VALUESTRING_LENGTH, "%d", value);

    // If an error occurred sprintf_s return -1
    if (retCode != -1)
    {
        ret = msg;
    }

    return ret;
}

int Mind_Camera::CommonParseParam(const std::string& val) const {
    std::map<std::string, std::string>::const_iterator iter = params_.find(val);
    if (iter != params_.end()) {
        return atoi((iter->second).c_str());
    }

    return PARSEPARAM_FAIL;
}

void Mind_Camera::SplitString(const std::string& source,
                                 std::vector<std::string>& tmp,
                                 const std::string& obj) {
    string::size_type pos1 = 0;
    string::size_type pos2 = source.find(obj);

    while (string::npos != pos2) {
    tmp.push_back(source.substr(pos1, pos2 - pos1));
    pos1 = pos2 + obj.size();
    pos2 = source.find(obj, pos1);
    }

    if (pos1 != source.length()) {
    tmp.push_back(source.substr(pos1));
    }
}

void Mind_Camera::ParseImageSize(const std::string& val, int& width,
                                    int& height) const {
    std::vector<std::string> tmp;
    SplitString(val, tmp, "x");

    //val is not a format of resolution ratio(*x*).
    if (tmp.size() != 2) {
        width = 0;
        height = 0;
    } else {
        width = atoi(tmp[0].c_str());
        height = atoi(tmp[1].c_str());
    }
}

Mind_Camera::CameraOperationCode Mind_Camera::PreCapProcess() {
    MediaLibInit();

    CameraStatus status = QueryCameraStatus(config_->channel_id);
    if (status != CAMERA_STATUS_CLOSED) {
        HIAI_ENGINE_LOG(
            "[Mind_Camera] PreCapProcess.QueryCameraStatus {status:%d} \
            failed.",status);
        return kCameraNotClosed;
    }

    //Open Camera
    int ret = OpenCamera(config_->channel_id);
    if (ret == 0) {
        HIAI_ENGINE_LOG(
                "[Mind_Camera] PreCapProcess OpenCamera {%d} failed.",
                config_->channel_id);
        return kCameraOpenFailed;
    }

    //set fps
    ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_FPS,
                            &(config_->fps));
    if (ret == 0) {
        HIAI_ENGINE_LOG(
                "[Mind_Camera] PreCapProcess set fps {fps:%d} failed.",
                config_->fps);
        return kCameraSetPropertyFailed;
    }

    // set image format
    ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_IMAGE_FORMAT,
                            &(config_->image_format));
    if (ret == 0) {
        HIAI_ENGINE_LOG(
                "[Mind_Camera] PreCapProcess set image_fromat {format:%d} \
                failed.",config_->image_format);
        return kCameraSetPropertyFailed;
    }

    // set image resolution.
    CameraResolution resolution;
    resolution.width = config_->resolution_width;
    resolution.height = config_->resolution_height;
    ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_RESOLUTION,
                            &resolution);
    if (ret == 0) {
        HIAI_ENGINE_LOG(
                "[Mind_Camera] PreCapProcess set resolution {width:%d, \
                height:%d } failed.",config_->resolution_width,
                                     config_->resolution_height);
        return kCameraSetPropertyFailed;
    }

    // set work mode
    CameraCapMode mode = CAMERA_CAP_ACTIVE;
    ret = SetCameraProperty(config_->channel_id, CAMERA_PROP_CAP_MODE, &mode);
    if (ret == 0) {
        HIAI_ENGINE_LOG(
            "[Mind_Camera] PreCapProcess set cap mode {mode:%d} failed.",
            mode);
        return kCameraSetPropertyFailed;
    }

    return kCameraOk;
}

std::shared_ptr<FaceRecognitionInfo>
    Mind_Camera::CreateBatchImageParaObj() {
    std::shared_ptr<FaceRecognitionInfo> pObj = make_shared<FaceRecognitionInfo>();

    // handle one image frame every time
    pObj->frame.channel_id = config_->channel_id;
    pObj->frame.frame_id = frame_id_++;
    pObj->frame.timestamp = time(nullptr);

    // channel begin from zero
    pObj->org_img.channel = 0;
    pObj->org_img.format = YUV420SP;
    pObj->org_img.width = config_->resolution_width;
    pObj->org_img.height = config_->resolution_height;
    // YUV size in memory is width*height*3/2
    pObj->org_img.size = config_->resolution_width * config_->resolution_height
        * 3 / 2;

    shared_ptr<uint8_t> data(new uint8_t[pObj->org_img.size],
                            default_delete<uint8_t[]>());
    pObj->org_img.data = data;
    return pObj;
}

bool Mind_Camera::DoCapProcess() {
    CameraOperationCode retCode = PreCapProcess();
    if (retCode == kCameraSetPropertyFailed) {
        CloseCamera(config_->channel_id);

        HIAI_ENGINE_LOG( "[Mind_Camera] DoCapProcess.PreCapProcess failed");
        return false;
    }

    // set procedure is running.
    SetExitFlag(CAMERADATASETS_RUN);

    HIAI_StatusT hiai_ret = HIAI_OK;
    int read_ret = 0;
    int read_size = 0;
    bool read_flag = false;
    while (GetExitFlag() == CAMERADATASETS_RUN) {
        std::shared_ptr<FaceRecognitionInfo> p_obj =
            CreateBatchImageParaObj();
        
        uint8_t* p_data = p_obj->org_img.data.get();
        read_size = (int) p_obj->org_img.size;

        // do read frame from camera, readSize maybe changed when called
        read_ret = ReadFrameFromCamera(config_->channel_id, (void*) p_data,
                                    &read_size);
        // indicates failure when readRet is 1
        read_flag = ((read_ret == 1) && (read_size == (int) p_obj->org_img.size));

        if (!read_flag) {
        HIAI_ENGINE_LOG("[CameraDatasets] readFrameFromCamera failed "
                        "{camera:%d, ret:%d, size:%d, expectsize:%d} ",
                        config_->channel_id, read_ret, read_size,
                        (int )p_obj->org_img.size);
        break;
        }

        hiai_ret = SendData(0, "FaceRecognitionInfo",
                            static_pointer_cast<void>(p_obj));

        if (hiai_ret != HIAI_OK) {
        HIAI_ENGINE_LOG("[CameraDatasets] senddata failed! {frameid:%d, "
                        "timestamp:%lu}",
                        p_obj->frame.frame_id, p_obj->frame.timestamp);
        break;
        }
    }

    // close camera
    CloseCamera(config_->channel_id);

    if (HIAI_OK != hiai_ret) {
        return false;
    }

    return true;
}

void Mind_Camera::SetExitFlag(int flag) {
    TLock lock(mutex_);
    exit_flag_ = flag;
}

int Mind_Camera::GetExitFlag() {
    TLock lock(mutex_);
    return exit_flag_;
}

HIAI_IMPL_ENGINE_PROCESS("Mind_Camera", Mind_Camera, INPUT_SIZE)
{
    HIAI_ENGINE_LOG("[Mind_Camera] start process!");
    DoCapProcess();
    HIAI_ENGINE_LOG("[Mind_Camera] end process!");
    return HIAI_OK;
}

