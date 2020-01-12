/*******
*
* Copyright(c)<2018>, <Huawei Technologies Co.,Ltd>
*
* @version 1.0
*
* @date 2018-5-19
*/
#ifndef head_pose_inference_1_ENGINE_H_
#define head_pose_inference_1_ENGINE_H_
#include "hiaiengine/api.h"
#include "hiaiengine/ai_model_manager.h"
#include "hiaiengine/ai_types.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/engine.h"
#include "hiaiengine/multitype_queue.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/ai_tensor.h"
#include "head_pose_estimate_params.h"
#include <iostream>
#include <string>
#include <dirent.h>
#include <memory>
#include <unistd.h>
#include <vector>
#include <stdint.h>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/types_c.h"
#define INPUT_SIZE 2
#define OUTPUT_SIZE 1
#define DEFAULT_DATA_PORT 0
#define AI_MODEL_PROCESS_TIMEOUT 0

class head_pose_inference_1 : public hiai::Engine {
public:
    head_pose_inference_1() :
        input_que_(INPUT_SIZE) {}
    HIAI_StatusT Init(const hiai::AIConfig& config, const std::vector<hiai::AIModelDescription>& model_desc);
    /**
    * @ingroup hiaiengine
    * @brief HIAI_DEFINE_PROCESS : reload Engine Process
    * @[in]: define the number of input and output
    */
    HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE)
private:
    // Private implementation a member variable, which is used to cache the input queue
    hiai::MultiTypeQueue input_que_;

    // AI module manager
    std::shared_ptr<hiai::AIModelManager> ai_model_manager_;


    /*
    * Define the face feature position
    */
    enum FaceFeaturePos {
        kLeftEyeX,
        kLeftEyeY,
        kRightEyeX,
        kRightEyeY,
        kNoseX,
        kNoseY,
        kLeftMouthX,
        kLeftMouthY,
        kRightMouthX,
        kRightMouthY
    };

    /*
    * @brief: Init the AI model, load the graph.config and based
    *   on the model path,load the model
    * @param [in]: config: configuration in graph.config
    * @return: Whether init success
    */
    bool InitAiModel(const hiai::AIConfig &config);

    /*
    * @brief: Init the normlized mean and std value, the data source is from
    *   trainMean.png and trainSTD.png
    * @return: Whether init success
    */
    bool InitNormlizedData();

    /*
    * @brief: Crop the face from original image base on the face coordinate
    *   Invoke the ez_dvpp interface to do the crop action
    * @param [in]: face_recognition_info->frame Frame info
    * @param [in]: org_img The original image information
    * @param [in]: face_imgs->rectangle Face points based on the original image,
    *   face_imgs->images Face image data after cropped, NV12
    * @return: Whether init success
    */
    bool Crop(const std::shared_ptr<FaceRecognitionInfo> &face_recognition_info, const hiai::ImageData<u_int8_t> &org_img,
                std::vector<FaceImage> &face_imgs);

    /*
    * @brief: Resize the face from cropped image base on the face size 40*40
    *   Invoke the ez_dvpp interface to do the resize action
    * @param [in]: face_imgs->image Image data after cropped
    * @param [in]: resized_image Face info after resize
    * @return: Whether init success
    */
    bool Resize(const std::vector<FaceImage> &face_imgs,
                std::vector<hiai::ImageData<u_int8_t>> &resized_image);


    /*
    * @brief: Inference the data by the FWK's Process interface
    * @param [in]: normalized_image The data after normalization
    * @param [in]: face_imgs->feature_mask The inference result
    * @return: Whether init success
    */

    // 开启AIPP
    bool Inference(const std::vector<hiai::ImageData<u_int8_t>> resized_imgs, std::vector<FaceImage> &face_imgs);

    /*
    * @brief: Judge that whether the data is wrong, if the error is from last
    *   node, pass the data to next node directly
    * param [in]: face_detail_info->err_info
    * param [in]: flag for whether it is success
    * @return: Whether init success
    */
    bool IsDataHandleWrong(std::shared_ptr<FaceRecognitionInfo> &face_detail_info);

    /*
    * @brief: Handle failed when some step has the error
    * param [in]: error_log Error log info
    * param [in]: face_detail_info->err_info Error detail
    * @return: flag for whether it is success
    */
    HIAI_StatusT SendFailed(const std::string error_log,
                            std::shared_ptr<FaceRecognitionInfo> &face_recognition_info);

    /*
    * @brief: Handle failed when some step has the error
    * param [in]: face_detail_info->err_info Error detail
    * @return: flag for whether it is success
    */
    HIAI_StatusT SendSuccess(
        std::shared_ptr<FaceRecognitionInfo> &face_recognition_info);
};

#endif
