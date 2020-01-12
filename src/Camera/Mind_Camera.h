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

#ifndef CAMERADATASETS_ENGINE_H
#define CAMERADATASETS_ENGINE_H

#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <stdio.h>

#include "hiaiengine/engine.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/data_type_reg.h"
#include "head_pose_estimate_params.h"

#define CAMERAL_1 (0)
#define CAMERAL_2 (1)

#define INPUT_SIZE 1
#define OUTPUT_SIZE 1

#define CAMERADATASETS_INIT (0)
#define CAMERADATASETS_RUN  (1)
#define CAMERADATASETS_STOP (2)
#define CAMERADATASETS_EXIT (3)

#define PARSEPARAM_FAIL (-1)
#define MAX_VALUESTRING_LENGTH 25

/**
 * Mind_Camera used to capture image from camera
 */
class Mind_Camera : public hiai::Engine {
public:
    struct CameraDatasetsConfig {
        int fps;
        int channel_id;
        int image_format;
        int resolution_width;
        int resolution_height;
        std::string ToString() const;
    };

    enum CameraOperationCode {
        kCameraOk = 0,
        kCameraNotClosed = -1,
        kCameraOpenFailed = -2,
        kCameraSetPropertyFailed = -3,
    };

    /**
    * @brief   constructor
    */
    Mind_Camera();

    /**
    * @brief   destructor
    */
    ~Mind_Camera();

    /**
    * @brief  init config of Mind_Camera by aiConfig
    * @param [in]  initialized aiConfig
    * @param [in]  modelDesc
    * @return  success --> HIAI_OK ; fail --> HIAI_ERROR
    */
    HIAI_StatusT Init(const hiai::AIConfig& aiConfig,
                      const std::vector<hiai::AIModelDescription>& modelDesc) override;

    /**
    * @brief  Splite String s by c and store in v
    * @param [in] string s
    * @param [in] string c     value used to cut resolution ratio
    * @param [in] vector v     used to conserve the value of width and heigth
    * @param [out] vector v    value of width and heigth
    */
    static void SplitString(const std::string& source,
                            std::vector<std::string>& tmp,
                            const std::string& obj);

    /**
    * @brief   translate value to string
    * @param [in] value    channel id of camera
    * @return   string translate by value
    */
    static std::string IntToString(int value);

    /**
    * @brief  ingroup hiaiengine
    */
    HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE)

private:

    /**
    * @brief  create Image object
    * @return : shared_ptr of data frame
    */
    std::shared_ptr<FaceRecognitionInfo> CreateBatchImageParaObj();

    /**
    * @brief : init map params
    */
    void InitConfigParams();

    /**
    * @brief   preprocess for cap camera
    * @return  camera code
    */
    Mind_Camera::CameraOperationCode PreCapProcess();

    /**
    * @brief  cap camera
    * @return  success-->true ; fail-->false
    */
    bool DoCapProcess();

    /**
    * @brief  parse param
    * @return value of config
    */
    int CommonParseParam(const std::string& val) const;

    /**
    * @brief  get exit flag
    * @return the value of exit
    */
    int GetExitFlag();

    /**
    * @brief  set exit flag
    * @param [in]  which value want to set
    */
    void SetExitFlag(int flag = CAMERADATASETS_STOP);

    /**
    * @brief  get width and height from string val
    * @param [in]  val     resolution ratio of picture
    * @param [in]  width   used ot conserve width of picture
    * @param [in]  height  used ot conserve height of picture
    * @param [out] width   value of width
    * @param [out] height  value of height
    */
    void ParseImageSize(const std::string& val, int& width, int& height) const;

private:
    typedef std::unique_lock<std::mutex> TLock;
    std::shared_ptr<CameraDatasetsConfig> config_; //configure for camera
    std::map<std::string, std::string> params_; // all configure item for camera
    std::mutex mutex_; //thread variable to protect exit
    int exit_flag_; //ret of cameradataset
    uint32_t frame_id_;//frame id for image data

};

#endif
