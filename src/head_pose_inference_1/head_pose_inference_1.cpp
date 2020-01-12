/*******
*
* Copyright(c)<2018>, <Huawei Technologies Co.,Ltd>
*
* @version 1.0
*
* @date 2018-5-19
*/
#include "head_pose_inference_1.h"
#include <opencv2/imgproc/types_c.h>
#include "hiaiengine/log.h"
#include "hiaiengine/data_type_reg.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"
#include <memory>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>

using namespace ascend::utils;
using hiai::Engine;
using namespace std;
using namespace hiai;
using namespace cv;

namespace {
// The image's width need to be resized
const int32_t kResizedImgWidth = 224;

// The image's height need to be resized
const int32_t kResizedImgHeight = 224;

const int32_t kEachResult1Size = 68;//特征点结果数
const int32_t kEachResult2Size = 3;//姿态结果数
// inference output result index
const int32_t kResult1Index = 0;
const int32_t kResult2Index = 1;

// The rgb image's channel number
const int32_t kRgbChannel = 3;

// For each input, the result should be one tensor
const int32_t kEachResultTensorNum = 10;

// The center's size for the inference result
const float kNormalizedCenterData = 0.5;

const int32_t kSendDataIntervalMiss = 20;
}
shared_ptr<FaceRecognitionInfo> face_recognition_info2;

/**
* @ingroup hiaiengine
* @brief HIAI_DEFINE_PROCESS : implementaion of the engine
* @[in]: engine name and the number of input
*/
HIAI_StatusT head_pose_inference_1::Init(const hiai::AIConfig &config,
    const std::vector<hiai::AIModelDescription> &model_desc)
{
    if (!InitAiModel(config)) {
        return HIAI_ERROR;
    }
    return HIAI_OK;
}

bool head_pose_inference_1::InitAiModel(const AIConfig &config) {
    AIStatus ret = SUCCESS;

    // Define the initialization value for the ai_model_manager_
    if (ai_model_manager_ == nullptr) {
        ai_model_manager_ = make_shared<AIModelManager>();
    }

    vector<AIModelDescription> model_desc_vec;
    AIModelDescription model_desc;

    // Get the model information from the file graph.config
    for (int index = 0; index < config.items_size(); ++index) {
        const AIConfigItem &item = config.items(index);
        if (item.name() == kModelPathParamKey) {
        const char *model_path = item.value().data();
        model_desc.set_path(model_path);
        } 
        else {
        continue;
        }
    }

    //Invoke the framework's interface to init the information
    model_desc_vec.push_back(model_desc);
    ret = ai_model_manager_->Init(config, model_desc_vec);

    if (ret != SUCCESS) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "AI model init failed!");
        return false;
    }
    return true;
}


// 参数1. 记录人脸数据 2. 记录原始图片数据 3. 一张图片可以识别出的人脸的信息
bool head_pose_inference_1::Crop(const shared_ptr<FaceRecognitionInfo> &face_recognition_info, const ImageData<u_int8_t> &org_img,
                                  vector<FaceImage> &face_imgs) 
{
  HIAI_ENGINE_LOG("Begin to crop the face, face number is %d",
                  face_imgs.size());
  int32_t img_size = org_img.size;
  for (vector<FaceImage>::iterator face_img_iter = face_imgs.begin();
       face_img_iter != face_imgs.end(); ++face_img_iter) {
    // call ez_dvpp to crop image
    DvppBasicVpcPara crop_para;
    crop_para.input_image_type = face_recognition_info->frame.org_img_format;

    // Change the left top coordinate to even numver 将左上角坐标改变为偶数
    u_int32_t lt_horz = ((face_img_iter->rectangle.lt.x) >> 1) << 1;
    u_int32_t lt_vert = ((face_img_iter->rectangle.lt.y) >> 1) << 1;
    // Change the left top coordinate to odd numver 将右下角坐标改变为奇数
    u_int32_t rb_horz = (((face_img_iter->rectangle.rb.x) >> 1) << 1) - 1;
    u_int32_t rb_vert = (((face_img_iter->rectangle.rb.y) >> 1) << 1) - 1;
    HIAI_ENGINE_LOG("The crop is from left-top(%d,%d) to right-bottom(%d,%d)",
                    lt_horz, lt_vert, rb_horz, rb_vert);
    // 偶数减去奇数再加1，还是偶数
    u_int32_t cropped_width = rb_horz - lt_horz + 1;
    u_int32_t cropped_height = rb_vert - lt_vert + 1;

    crop_para.src_resolution.width = org_img.width;
    crop_para.src_resolution.height = org_img.height;
    crop_para.dest_resolution.width = cropped_width;
    crop_para.dest_resolution.height = cropped_height;
    crop_para.crop_left = lt_horz;
    crop_para.crop_right = rb_horz;
    crop_para.crop_up = lt_vert;
    crop_para.crop_down = rb_vert;

    crop_para.is_input_align = face_recognition_info->frame.img_aligned;
    crop_para.is_output_align = false;
    DvppProcess dvpp_crop_img(crop_para);
    DvppVpcOutput dvpp_output;
    int ret = dvpp_crop_img.DvppBasicVpcProc(
                org_img.data.get(), img_size, &dvpp_output);
    if (ret != kDvppOperationOk) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Call ez_dvpp failed, failed to crop image.");
      return false;
    }

    // 最终给faceimgs结构填充图像内容
    face_img_iter->image.data.reset(dvpp_output.buffer, default_delete<u_int8_t[]>());
    face_img_iter->image.size = dvpp_output.size;
    face_img_iter->image.width = cropped_width;
    face_img_iter->image.height = cropped_height;
  }
  return true;
}

bool head_pose_inference_1::Resize(const vector<FaceImage> &face_imgs,
                                    vector<ImageData<u_int8_t>> &resized_imgs) {
  // Begin to resize all the resize image
  for (vector<FaceImage>::const_iterator face_img_iter = face_imgs.begin();
       face_img_iter != face_imgs.end(); ++face_img_iter) {

    int32_t img_size = face_img_iter->image.size;
    if (img_size <= 0) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "image size less than or equal zero, size=%d", img_size);
      return false;
    }

    // 人脸图像的真实宽、高
    int32_t origin_width = face_img_iter->image.width;
    int32_t origin_height = face_img_iter->image.height;
    ImageData<u_int8_t> resized_image;
    DvppBasicVpcPara resize_para;

    //resize_para.input_image_type = face_recognition_info2->frame.org_img_format;//INPUT_YUV420_SEMI_PLANNER_UV;
    resize_para.input_image_type = INPUT_YUV420_SEMI_PLANNER_UV;
    resize_para.crop_left = 0;
    resize_para.crop_up = 0;
    resize_para.crop_right = origin_width - 1;
    resize_para.crop_down = origin_height - 1;
    resize_para.src_resolution.width = origin_width;
    resize_para.src_resolution.height = origin_height;

    resize_para.dest_resolution.width = kResizedImgWidth;
    resize_para.dest_resolution.height = kResizedImgHeight;
    //resize_para.is_input_align = true;
    resize_para.is_output_align = false;
    //resize_para.is_input_align = face_recognition_info2->frame.img_aligned;
    resize_para.is_input_align = false;
    DvppProcess dvpp_resize_img(resize_para);

    // Invoke EZ_DVPP interface to resize image
    DvppVpcOutput dvpp_output;
    int ret = dvpp_resize_img.DvppBasicVpcProc(
                face_img_iter->image.data.get(), img_size, &dvpp_output);
    if (ret != kDvppOperationOk) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Call ez_dvpp failed, failed to resize image.");
      return false;
    }

    // call success, set data and size
    resized_image.data.reset(dvpp_output.buffer, default_delete<u_int8_t[]>());
    resized_image.size = dvpp_output.size;
    resized_image.width = kResizedImgWidth;
    resized_image.height = kResizedImgHeight;
    resized_imgs.push_back(resized_image);
  }
  return true;
}

bool head_pose_inference_1::Inference(const vector<ImageData<u_int8_t>> resized_imgs,
                                       vector<FaceImage> &face_imgs) {
  // Define the ai model's data
  AIContext ai_context;

  //一张图片上对应的人脸个数
  int nresized_imgs_size = resized_imgs.size();

  for (int i = 0; i < nresized_imgs_size; i++) 
  {
    int start_index =  i;

    vector<shared_ptr<IAITensor>> input_data_vec;
    shared_ptr<AINeuralNetworkBuffer> neural_buffer = shared_ptr <
        AINeuralNetworkBuffer > (new(std::nothrow) AINeuralNetworkBuffer());
    shared_ptr<IAITensor> input_data = static_pointer_cast <
                                       IAITensor > (neural_buffer);
    //一张图片的buffer送入模型的输入Tensor
    neural_buffer->SetBuffer((void *)resized_imgs[start_index].data.get(), resized_imgs[start_index].size);
    input_data_vec.push_back(input_data);

    vector<shared_ptr<IAITensor>> output_data_vec;
    AIStatus ret = ai_model_manager_->CreateOutputTensor(input_data_vec, output_data_vec);
    if (ret != SUCCESS) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Fail to create output tensor");
      return false;
    }

    ret = ai_model_manager_->Process(ai_context, input_data_vec, output_data_vec, 0);

    if (ret != SUCCESS) {
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Fail to process the data in FWK");
      return false;
    }

    HIAI_ENGINE_LOG("Inference successed!");

    // Get the inference result.
    
    shared_ptr<AISimpleTensor> result_tensor = static_pointer_cast <
        AISimpleTensor > (output_data_vec[kResult1Index]);//第0路输出 136个float


    shared_ptr<hiai::AISimpleTensor> result_tensor1 = static_pointer_cast <
        AISimpleTensor > (output_data_vec[kResult2Index]);//第1路输出 3个float


    int32_t size = result_tensor->GetSize() / sizeof(float);
    int32_t size1 = result_tensor1->GetSize() / sizeof(float);
        

    // 为模型的第一路输出申请内存空间 
    float result[size];
    errno_t mem_ret = memcpy_s(result, sizeof(result), result_tensor->GetBuffer(),
                                result_tensor->GetSize());
    if (mem_ret != EOK) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "post process call memcpy_s() error=%d", mem_ret);
        return false;
    }

    // 基于原图的人脸框宽度
    float box_width = face_imgs[i].rectangle.rb.x - face_imgs[i].rectangle.lt.x;
    // 基于原图的人脸框高度
    float box_height = face_imgs[i].rectangle.rb.y - face_imgs[i].rectangle.lt.y;
    int temp_size = 0;
    // 人脸特征点计算方式:
    // 基于原图x的特征点坐标 = 基于原图的人脸框左上角x的坐标+(1+模型坐标点x的输出值)/2 * 基于原图人脸框的宽度
    // 基于原图y的特征点坐标 = 基于原图的人脸框左上角y的坐标+(1+模型坐标点y的输出值)/2 * 基于原图人脸框的高度
    while(temp_size < size)
    {
      if(temp_size % 2 == 0)
        // x特征点的坐标
        // 基于原图x的特征点坐标 = 基于原图人脸框左上角x的坐标+(1+模型坐标点x的输出值)/2 * 基于原图人脸框的宽度
        result[temp_size] = ((result[temp_size] + 1.0) / 2) * box_width + face_imgs[i].rectangle.lt.x;
      else 
        // y特征点的坐标
        // 基于原图y的特征点坐标 = 基于原图人脸框左上角y的坐标+(1+模型坐标点y的输出值)/2 * 基于原图人脸框的高度
        result[temp_size] = ((result[temp_size] + 1.0) / 2) * box_height + face_imgs[i].rectangle.lt.y;
      temp_size++;
    }
  
    //将模型输出的68个点的坐标写入到结构体中，传送给后处理引擎
    int32_t k_points = 0;
    while(k_points < kEachResult1Size)
    {
      face_imgs[i].infe_res.face_points[k_points].x = result[k_points * 2];
      face_imgs[i].infe_res.face_points[k_points].y = result[(k_points * 2) + 1];
      k_points += 1;
    }

    float result1[size1];
    //为第二路输出申请内存空间
    errno_t mem_ret1 = memcpy_s(result1, sizeof(result1), result_tensor1->GetBuffer(),
                                result_tensor1->GetSize());
    if (mem_ret1 != EOK) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "post process call memcpy_s() error=%d", mem_ret1);
        return false;
    }

    int32_t k_pose = 0;
    while(k_pose < kEachResult2Size)
    {
      face_imgs[i].infe_res.head_pose[k_pose] = result1[k_pose] * 50;
      k_pose += 1;
    }
    // 循环输入推理
    input_data_vec.clear();
    output_data_vec.clear();
  }
  return true;
}



bool head_pose_inference_1::IsDataHandleWrong(shared_ptr<FaceRecognitionInfo> &face_detail_info) {
  ErrorInfo err_info = face_detail_info->err_info;
  if (err_info.err_code != AppErrorCode::kNone) {
    return false;
  }
  return true;
}

HIAI_StatusT head_pose_inference_1::SendFailed(const string error_log,
    shared_ptr<FaceRecognitionInfo> &face_recognition_info) {

  HIAI_ENGINE_LOG("VCNN network run error");
  ErrorInfo err_info = face_recognition_info->err_info;
  err_info.err_code = AppErrorCode::kFeatureMask;
  err_info.err_msg = error_log;
  HIAI_StatusT ret = HIAI_OK;
  do {
    ret = SendData(DEFAULT_DATA_PORT, "FaceRecognitionInfo",
                   static_pointer_cast<void>(face_recognition_info));
    if (ret == HIAI_QUEUE_FULL) {
      HIAI_ENGINE_LOG("Queue is full, sleep %d ms", HIAI_QUEUE_FULL);
      usleep(kSendDataIntervalMiss);
    }
  } while (ret == HIAI_QUEUE_FULL);

  return ret;
}

HIAI_StatusT head_pose_inference_1::SendSuccess(
  shared_ptr<FaceRecognitionInfo> &face_recognition_info) {

  HIAI_ENGINE_LOG("VCNN network run success, the total face is %d .",
                  face_recognition_info->face_imgs.size());
  HIAI_StatusT ret = HIAI_OK;
  do {
    ret = SendData(DEFAULT_DATA_PORT, "FaceRecognitionInfo",
                   static_pointer_cast<void>(face_recognition_info));
    if (ret == HIAI_QUEUE_FULL) {
      HIAI_ENGINE_LOG("Queue is full, sleep 200ms");
      usleep(kSendDataIntervalMiss);
    }
  } while (ret == HIAI_QUEUE_FULL);

  return ret;
}

HIAI_IMPL_ENGINE_PROCESS("head_pose_inference_1", head_pose_inference_1, INPUT_SIZE)
{
    // args is null, arg0 is image info, arg1 is model info
    if (nullptr == arg0) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "Fail to process invalid message, is null.");
        return HIAI_ERROR;
    }

    // Get the data from last stdEngine
    // If not correct, Send the message to next node directly
    shared_ptr<FaceRecognitionInfo> face_recognition_info = static_pointer_cast <
        FaceRecognitionInfo > (arg0);
    if (!IsDataHandleWrong(face_recognition_info)) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "The message status is not normal");
        SendData(DEFAULT_DATA_PORT, "FaceRecognitionInfo",
                static_pointer_cast<void>(face_recognition_info));
        return HIAI_ERROR;
    }
    // 没有检测到合格的人脸，所以不需要推理，直接发送结果给post process
    if (face_recognition_info->face_imgs.size() == 0) {
        HIAI_ENGINE_LOG("No face image need to be handled.");
        return SendSuccess(face_recognition_info);
    }
    
    // 根据face_imgs中存储的的人脸坐标，将人脸图像数据从原图像中扣出来放到face_imgs中的image中

    // 参数1. 记录人脸数据 2. 记录原始图片数据 3. 一张图片可以识别出的人脸的信息
    if (!Crop(face_recognition_info, face_recognition_info->org_img, face_recognition_info->face_imgs)) {
        return SendFailed("Crop all the data failed, all the data failed",
                        face_recognition_info);
    }
    
    // 将face_imgs的image图像resize到模型需要的大小，存放在resized_imgs中
    vector<ImageData<u_int8_t>> resized_imgs;
    if (!Resize(face_recognition_info->face_imgs, resized_imgs)) {
        return SendFailed("Resize all the data failed, all the data failed",
                        face_recognition_info);
    }

    bool inference_flag = Inference(resized_imgs, face_recognition_info->face_imgs);
    if (!inference_flag) {
        return SendFailed("Inference the data failed",
                        face_recognition_info);
    }

    return SendSuccess(face_recognition_info);
    return HIAI_OK;
}
