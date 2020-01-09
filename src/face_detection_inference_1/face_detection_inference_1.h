/*******
*
* Copyright(c)<2018>, <Huawei Technologies Co.,Ltd>
*
* @version 1.0
*
* @date 2018-5-19
*/
#ifndef face_detection_inference_1_ENGINE_H_
#define face_detection_inference_1_ENGINE_H_
#include "head_pose_estimate_params.h"
#include "hiaiengine/api.h"
#include "hiaiengine/ai_model_manager.h"
#include "hiaiengine/ai_types.h"
#include "hiaiengine/data_type.h"
#include "hiaiengine/engine.h"
#include "hiaiengine/data_type_reg.h"
#include "hiaiengine/ai_tensor.h"

#define INPUT_SIZE 2
#define OUTPUT_SIZE 1

#define AI_MODEL_PROCESS_TIMEOUT 0

class face_detection_inference_1 : public hiai::Engine {
public:
    face_detection_inference_1();
    ~face_detection_inference_1() = default;
    HIAI_StatusT Init(const hiai::AIConfig& config, const std::vector<hiai::AIModelDescription>& model_desc);
    /**
    * @ingroup hiaiengine
    * @brief HIAI_DEFINE_PROCESS : reload Engine Process
    * @[in]: define the number of input and output
    */
    HIAI_DEFINE_PROCESS(INPUT_SIZE, OUTPUT_SIZE)
private:

    // cache AI model parameters
    std::shared_ptr<hiai::AIModelManager> ai_model_manager_;

    // confidence : used to check inference result
    float confidence_;

    /**
    * @brief: check confidence is valid or not
    * param [in]: confidence
    * @return: false:invalid, true: valid
    */
    bool IsValidConfidence(float confidence);

    /**
    * @brief: check inference results is valid or not
    * param [in]: attribute
    * param [in]: score
    * param [in]: face rectangle
    * @return: false:invalid, true: valid
    */
    bool IsValidResults(float attr, float score, const FaceRectangle &rectangle);

    /**
    * @brief: correction ratio
    * param [in]: coordinate ratio
    * @return: ratio in [0, 1]
    *          when ratio less than zero, then return zero
    *          when ratio more than one, then return one
    */
    float CorrectionRatio(float ratio);

    /**
    * @brief: pre-process
    * param [in]: image_handle: original image
    * param [out]: resized_image: ez_dvpp output image
    * @return: true: success; false: failed
    */
    bool PreProcess(const std::shared_ptr<FaceRecognitionInfo> &image_handle,
                    hiai::ImageData<u_int8_t> &resized_image);

    /**
    * @brief: inference
    * param [in]: resized_image: ez_dvpp output image
    * param [out]: output_data_vec: inference output
    * @return: true: success; false: failed
    */
    bool Inference(
        const hiai::ImageData<u_int8_t> &resized_image,
        std::vector<std::shared_ptr<hiai::IAITensor>> &output_data_vec);

    /**
    * @brief: post process
    * param [out]: image_handle: engine transform image
    * param [in]: output_data_vec: inference output
    * @return: true: success; false: failed
    */
    bool PostProcess(
        std::shared_ptr<FaceRecognitionInfo> &image_handle,
        const std::vector<std::shared_ptr<hiai::IAITensor>> &output_data_vec);

    /**
    * @brief: face detection
    * @param [out]: original information from front-engine
    * @return: HIAI_StatusT
    */
    HIAI_StatusT Detection(std::shared_ptr<FaceRecognitionInfo> &image_handle);

    /**
    * @brief: handle the error scene
    * param [in]: err_code: the error code
    * param [in]: err_msg: the error message
    * param [out]: image_handle: engine transform image
    */
    void HandleErrors(AppErrorCode err_code, const std::string &err_msg,
                        std::shared_ptr<FaceRecognitionInfo> &image_handle);

    /**
    * @brief: send result
    * param [out]: image_handle: engine transform image
    */
    void SendResult(const std::shared_ptr<FaceRecognitionInfo> &image_handle);
};

#endif