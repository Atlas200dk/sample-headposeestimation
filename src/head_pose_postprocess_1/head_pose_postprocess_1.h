/*******
*
* Copyright(c)<2018>, <Huawei Technologies Co.,Ltd>
*
* @version 1.0
*
* @date 2018-5-19
*/
#ifndef head_pose_postprocess_1_ENGINE_H_
#define head_pose_postprocess_1_ENGINE_H_
#include "head_pose_estimate_params.h"

#include <iostream>
#include <string>
#include <dirent.h>
#include <memory>
#include <unistd.h>

#include <vector>
#include <stdint.h>

#include "hiaiengine/api.h"
#include "hiaiengine/ai_types.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/engine.h"
#include "ascenddk/presenter/agent/presenter_channel.h"
#define INPUT_SIZE 1
#define OUTPUT_SIZE 1

// face detection configuration
struct FaceDetectionPostConfig {
  float confidence;  // confidence
  std::string presenter_ip;  // presenter server IP
  int32_t presenter_port;  // presenter server port for agent
  std::string channel_name;  // channel name
};

class head_pose_postprocess_1 : public hiai::Engine {
public:
    head_pose_postprocess_1();
    ~head_pose_postprocess_1() = default;
    HIAI_StatusT Init(const hiai::AIConfig& config, const std::vector<hiai::AIModelDescription>& model_desc);
    /**
    * @ingroup hiaiengine
    * @brief HIAI_DEFINE_PROCESS : reload Engine Process
    * @[in]: define the number of input and output
    */
    HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE)
private:
    bool IsSupportFormat(hiai::IMAGEFORMAT format);

    HIAI_StatusT ConvertImage(hiai::ImageData<u_int8_t>& org_img);
    // configuration
    std::shared_ptr<FaceDetectionPostConfig> fd_post_process_config_;

    // presenter channel
    std::shared_ptr<ascend::presenter::Channel> presenter_channel_;

    /**
    * @brief: handle original image
    * @param [in]: FaceRecognitionInfo format data which inference engine send
    * @return: HIAI_StatusT
    */
    HIAI_StatusT HandleOriginalImage(
        const std::shared_ptr<FaceRecognitionInfo> &inference_res);

    /**
    * @brief: handle results
    * @param [in]: FaceRecognitionInfo format data which inference engine send
    * @return: HIAI_StatusT
    */
    HIAI_StatusT HandleResults(
        const std::shared_ptr<FaceRecognitionInfo> &inference_res);

    /**
    * @brief: validate IP address
    * @param [in]: IP address
    * @return: true: invalid
    *          false: valid
    */
    bool IsInValidIp(const std::string &ip);

    /**
    * @brief: validate port
    * @param [in]: port
    * @return: true: invalid
    *          false: valid
    */
    bool IsInValidPort(int32_t port);

    /**
    * @brief: validate channel name
    * @param [in]: channel name
    * @return: true: invalid
    *          false: valid
    */
    bool IsInValidChannelName(const std::string &channel__name);

    /**
    * @brief: validate confidence
    * @param [in]: confidence
    * @return: true: invalid
    *          false: valid
    */
    bool IsInvalidConfidence(float confidence);


    /**
    * @brief: validate results
    * @param [in]: attribute
    * @param [in]: score
    * @param [in]: left top anchor
    * @param [in]: right bottom anchor
    * @return: true: invalid
    *          false: valid
    */
    bool IsInvalidResults(float attr, float score,
                            const ascend::presenter::Point &point_lt,
                            const ascend::presenter::Point &point_rb);

    /**
    * @brief: convert YUV420SP to JPEG, and then send to presenter
    * @param [in]: image height
    * @param [in]: image width
    * @param [in]: image size
    * @param [in]: image data
    * @return: FD_FUN_FAILED or FD_FUN_SUCCESS
    */
    int32_t SendImage(uint32_t height, uint32_t width, uint32_t size,
                        u_int8_t *data, vector<ascend::presenter::DetectionResult>& detection_results);
};

#endif