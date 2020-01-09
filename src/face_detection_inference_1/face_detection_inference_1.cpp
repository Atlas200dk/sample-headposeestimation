/*******
*
* Copyright(c)<2018>, <Huawei Technologies Co.,Ltd>
*
* @version 1.0
*
* @date 2018-5-19
*/
#include "face_detection_inference_1.h"
#include <vector>
#include <unistd.h>
#include <sstream>

#include "hiaiengine/log.h"
#include "ascenddk/ascend_ezdvpp/dvpp_process.h"

using hiai::Engine;
using hiai::ImageData;
using namespace std;
using namespace ascend::utils;

namespace {

// output port (engine port begin with 0)
const uint32_t kSendDataPort = 0;

// model need resized image to 300 * 300
const float kResizeWidth = 300.0;
const float kResizeHeight = 300.0;

const float kMoveLeft = -0.15;
const float kMoveRight = 1.15;
const float kMoveTop = -0.10;
const float kMoveDown = 1.25;

// confidence parameter key in graph.config
const string kConfidenceParamKey = "confidence";

// valid confidence range (0.0, 1.0]
const float kConfidenceMin = 0.0;
const float kConfidenceMax = 1.0;

// results
// inference output result index
const int32_t kResultIndex = 0;
// each result size (7 float)
const int32_t kEachResultSize = 7;
// attribute index
const int32_t kAttributeIndex = 1;
// score index
const int32_t kScoreIndex = 2;
// left top X-axis coordinate point
const int32_t kLeftTopXaxisIndex = 3;
// left top Y-axis coordinate point
const int32_t kLeftTopYaxisIndex = 4;
// right bottom X-axis coordinate point
const int32_t kRightBottomXaxisIndex = 5;
// right bottom Y-axis coordinate point
const int32_t kRightBottomYaxisIndex = 6;

// face attribute
const float kAttributeFaceLabelValue = 1.0;
const float kAttributeFaceDeviation = 0.00001;

// ratio
const float kMinRatio = 0.0;
const float kMaxRatio = 1.0;

// image source from register
const uint32_t kRegisterSrc = 1;

// sleep interval when queue full (unit:microseconds)
const __useconds_t kSleepInterval = 200000;
}

// register custom data type
HIAI_REGISTER_DATA_TYPE("FaceRecognitionInfo", FaceRecognitionInfo);
HIAI_REGISTER_DATA_TYPE("FaceRectangle", FaceRectangle);
HIAI_REGISTER_DATA_TYPE("FaceImage", FaceImage);

face_detection_inference_1::face_detection_inference_1() {
  ai_model_manager_ = nullptr;
  confidence_ = -1.0;  // initialized as invalid value
}
/**
* @ingroup hiaiengine
* @brief HIAI_DEFINE_PROCESS : implementaion of the engine
* @[in]: engine name and the number of input
*/
HIAI_StatusT face_detection_inference_1::Init(const hiai::AIConfig &config,
    const std::vector<hiai::AIModelDescription> &model_desc)
{
    HIAI_ENGINE_LOG("Start initialize!");
    // initialize aiModelManager
    if (ai_model_manager_ == nullptr) {
    ai_model_manager_ = std::make_shared<hiai::AIModelManager>();
    }

    // get parameters from graph.config
    // set model path and passcode to AI model description
    hiai::AIModelDescription fd_model_desc;
    for (int index = 0; index < config.items_size(); index++) {
    const ::hiai::AIConfigItem& item = config.items(index);
    // get model path
    if (item.name() == "model_path") {
        const char* model_path = item.value().data();
        fd_model_desc.set_path(model_path);
        } else if (item.name() == kConfidenceParamKey) {  // get confidence
          stringstream ss(item.value());
          ss >> confidence_;
        }
    }

    // initialize model manager
    std::vector<hiai::AIModelDescription> model_desc_vec;
    model_desc_vec.push_back(fd_model_desc);
    hiai::AIStatus ret = ai_model_manager_->Init(config, model_desc_vec);
    // initialize AI model manager failed
    if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE, "initialize AI model failed");
    return HIAI_ERROR;
    }

    HIAI_ENGINE_LOG("End initialize!");
    return HIAI_OK;
}

bool face_detection_inference_1::IsValidConfidence(float confidence) {
  return (confidence > kConfidenceMin) && (confidence <= kConfidenceMax);
}

bool face_detection_inference_1::IsValidResults(float attr, float score,
                                   const FaceRectangle &rectangle) {
  // attribute is not face (background)
  if (abs(attr - kAttributeFaceLabelValue) > kAttributeFaceDeviation) {
    return false;
  }

  // confidence check
  if ((score < confidence_) || !IsValidConfidence(score)) {
    return false;
  }

  // position check : lt == rb invalid
  if ((rectangle.lt.x == rectangle.rb.x)
      && (rectangle.lt.y == rectangle.rb.y)) {
    return false;
  }
  return true;
}

float face_detection_inference_1::CorrectionRatio(float ratio) {
  float tmp = (ratio < kMinRatio) ? kMinRatio : ratio;
  return (tmp > kMaxRatio) ? kMaxRatio : tmp;
}

//前处理：此处只做了resize->300*300
bool face_detection_inference_1::PreProcess(
    const shared_ptr<FaceRecognitionInfo> &image_handle,
    ImageData<u_int8_t> &resized_image) {
    // input size is less than zero, return failed
    int32_t img_size = image_handle->org_img.size;
    if (img_size <= 0) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "original image size less than or equal zero, size=%d",
                        img_size);
        return false;
    }

    // call ez_dvpp to resize image
    DvppBasicVpcPara resize_para;
    resize_para.input_image_type = image_handle->frame.org_img_format;

    // get original image size and set to resize parameter
    int32_t width = image_handle->org_img.width;
    int32_t height = image_handle->org_img.height;

    // set source resolution ratio
    resize_para.src_resolution.width = width;
    resize_para.src_resolution.height = height;

    // crop parameters, only resize, no need crop, so set original image size
    // set crop left-top point (need even number)
    resize_para.crop_left = 0;
    resize_para.crop_up = 0;
    // set crop right-bottom point (need odd number)
    uint32_t crop_right = ((width >> 1) << 1) - 1;
    uint32_t crop_down = ((height >> 1) << 1) - 1;
    resize_para.crop_right = crop_right;
    resize_para.crop_down = crop_down;

    // set destination resolution ratio (need even number)
    resize_para.dest_resolution.width = kResizeWidth;
    resize_para.dest_resolution.height = kResizeHeight;

    // set input image align or not
    resize_para.is_input_align = image_handle->frame.img_aligned;
    resize_para.is_output_align = false;

    // call
    DvppProcess dvpp_resize_img(resize_para);
    DvppVpcOutput dvpp_output;
    int ret = dvpp_resize_img.DvppBasicVpcProc(image_handle->org_img.data.get(),
                                                img_size, &dvpp_output);
    if (ret != kDvppOperationOk) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "call ez_dvpp failed, failed to resize image.");
        return false;
    }

    // call success, set data and size
    resized_image.data.reset(dvpp_output.buffer, default_delete<u_int8_t[]>());
    resized_image.size = dvpp_output.size;
    return true;
}

bool face_detection_inference_1::Inference(
  const ImageData<u_int8_t> &resized_image,
  vector<shared_ptr<hiai::IAITensor>> &output_data_vec) {
  // neural buffer
  shared_ptr<hiai::AINeuralNetworkBuffer> neural_buf = shared_ptr <
      hiai::AINeuralNetworkBuffer > (
        new hiai::AINeuralNetworkBuffer(),
        default_delete<hiai::AINeuralNetworkBuffer>());
  neural_buf->SetBuffer((void*) resized_image.data.get(), resized_image.size);

  // input data
  shared_ptr<hiai::IAITensor> input_data = static_pointer_cast<hiai::IAITensor>(
        neural_buf);
  vector<shared_ptr<hiai::IAITensor>> input_data_vec;
  input_data_vec.push_back(input_data);

  // Call Process
  // 1. create output tensor
  hiai::AIContext ai_context;
  hiai::AIStatus ret = ai_model_manager_->CreateOutputTensor(input_data_vec,
                       output_data_vec);
  // create failed
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "call CreateOutputTensor failed");
    return false;
  }

  // 2. process
  HIAI_ENGINE_LOG("aiModelManager->Process start!");
  ret = ai_model_manager_->Process(ai_context, input_data_vec, output_data_vec,
                                   AI_MODEL_PROCESS_TIMEOUT);
  // process failed, also need to send data to post process
  if (ret != hiai::SUCCESS) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, "call Process failed");
    return false;
  }
  HIAI_ENGINE_LOG("aiModelManager->Process end!");
  return true;
}

bool face_detection_inference_1::PostProcess(
  shared_ptr<FaceRecognitionInfo> &image_handle,
  const vector<shared_ptr<hiai::IAITensor>> &output_data_vec) {
    // inference result vector only need get first result
    // because batch is fixed as 1
    shared_ptr<hiai::AISimpleTensor> result_tensor = static_pointer_cast <
        hiai::AISimpleTensor > (output_data_vec[kResultIndex]);

    // copy data to float array
    int32_t size = result_tensor->GetSize() / sizeof(float);
    if(size <= 0){
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "the result tensor's size is not correct, size is %d", size);
        return false;
    }
    float result[size];
    errno_t mem_ret = memcpy_s(result, sizeof(result), result_tensor->GetBuffer(),
                                result_tensor->GetSize());
    if (mem_ret != EOK) {
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "post process call memcpy_s() error=%d", mem_ret);
        return false;
    }

    uint32_t width = image_handle->org_img.width;
    uint32_t height = image_handle->org_img.height;

    // every inference result needs 8 float
    // loop the result for every result
    float *ptr = result;
    for (int32_t i = 0; i < size - kEachResultSize; i += kEachResultSize) {
        ptr = result + i;
        // attribute
        float attr = ptr[kAttributeIndex];
        // confidence
        float score = ptr[kScoreIndex];

        // position 矩形框的角标位置点
        FaceRectangle rectangle;
        rectangle.lt.x = CorrectionRatio(ptr[kLeftTopXaxisIndex]) * width;
        rectangle.lt.y = CorrectionRatio(ptr[kLeftTopYaxisIndex]) * height;
        rectangle.rb.x = CorrectionRatio(ptr[kRightBottomXaxisIndex]) * width;
        rectangle.rb.y = CorrectionRatio(ptr[kRightBottomYaxisIndex]) * height;

	
        // 头部姿势识别，需要将角标放大一些
        float rect_width = rectangle.rb.x - rectangle.lt.x;
        float rect_height = rectangle.rb.y - rectangle.lt.y;
        rectangle.lt.x = rectangle.lt.x + kMoveLeft * rect_width;
        rectangle.rb.x = rectangle.rb.x + (kMoveRight - 1) * rect_width;
        rectangle.lt.y = rectangle.lt.y + kMoveTop * rect_height;
        rectangle.rb.y = rectangle.rb.y + (kMoveDown - 1) * rect_height;
        if (rectangle.lt.x < 0)
          rectangle.lt.x = 0;
        if (rectangle.lt.y < 0)
          rectangle.lt.y = 0;
        if (rectangle.rb.x < 0)
          rectangle.rb.x = 0;
        if (rectangle.rb.y < 0)
          rectangle.rb.y = 0;
        if (rectangle.lt.x > image_handle->org_img.width - 2)
          rectangle.lt.x = image_handle->org_img.width - 2;
        if (rectangle.lt.y > image_handle->org_img.height - 2)
          rectangle.lt.y = image_handle->org_img.height - 2;
        if (rectangle.rb.x > image_handle->org_img.width - 2)
          rectangle.rb.x = image_handle->org_img.width - 2;
        if (rectangle.rb.y > image_handle->org_img.height - 2)
          rectangle.rb.y = image_handle->org_img.height - 2;
        

        // check results is invalid, skip it
        if (!IsValidResults(attr, score, rectangle)) {
          continue;
        }

        HIAI_ENGINE_LOG("attr=%f, score=%f, lt.x=%d, lt.y=%d, rb.x=%d, rb.y=%d",
                        attr, score, rectangle.lt.x, rectangle.lt.y, rectangle.rb.x,
                        rectangle.rb.y);

        // push back to image_handle
        FaceImage faceImage;
        faceImage.rectangle = rectangle;
        faceImage.score = score;
        image_handle->face_imgs.emplace_back(faceImage);
    }
    return true;
}

void face_detection_inference_1::HandleErrors(
  AppErrorCode err_code, const string &err_msg,
  shared_ptr<FaceRecognitionInfo> &image_handle) {
  // write error log
  HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT, err_msg.c_str());

  // set error information
  image_handle->err_info.err_code = err_code;
  image_handle->err_info.err_msg = err_msg;

  // send data
  SendResult(image_handle);
}

void face_detection_inference_1::SendResult(
  const shared_ptr<FaceRecognitionInfo> &image_handle) {

  // when register face, can not discard when queue full
  HIAI_StatusT hiai_ret;
  do {
    hiai_ret = SendData(kSendDataPort, "FaceRecognitionInfo",
                        static_pointer_cast<void>(image_handle));
    // when queue full, sleep
    if (hiai_ret == HIAI_QUEUE_FULL) {
      HIAI_ENGINE_LOG("queue full, sleep 200ms");
      usleep(kSleepInterval);
    }
  } while (hiai_ret == HIAI_QUEUE_FULL
           && image_handle->frame.image_source == kRegisterSrc);

  // send failed
  if (hiai_ret != HIAI_OK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "call SendData failed, err_code=%d", hiai_ret);
  }
}

HIAI_StatusT face_detection_inference_1::Detection(
  shared_ptr<FaceRecognitionInfo> &image_handle) {
  string err_msg = "";
  if (image_handle->err_info.err_code != AppErrorCode::kNone) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "front engine dealing failed, err_code=%d, err_msg=%s",
                    image_handle->err_info.err_code,
                    image_handle->err_info.err_msg.c_str());
    SendResult(image_handle);
    return HIAI_ERROR;
  }

  // resize image
  ImageData<u_int8_t> resized_image;
  if (!PreProcess(image_handle, resized_image)) {
    err_msg = "face_detection call ez_dvpp to resize image failed.";
    HandleErrors(AppErrorCode::kDetection, err_msg, image_handle);
    return HIAI_ERROR;
  }

  // inference
  vector<shared_ptr<hiai::IAITensor>> output_data;
  if (!Inference(resized_image, output_data)) {
    err_msg = "face_detection inference failed.";
    HandleErrors(AppErrorCode::kDetection, err_msg, image_handle);
    return HIAI_ERROR;
  }

  // post process
  if (!PostProcess(image_handle, output_data)) {
    err_msg = "face_detection deal result failed.";
    HandleErrors(AppErrorCode::kDetection, err_msg, image_handle);
    return HIAI_ERROR;
  }

  // send result
  SendResult(image_handle);
  return HIAI_OK;
}

HIAI_IMPL_ENGINE_PROCESS("face_detection_inference_1", face_detection_inference_1, INPUT_SIZE)
{
    HIAI_StatusT ret = HIAI_OK;

  // deal arg0 (camera input)
  if (arg0 != nullptr) {
    HIAI_ENGINE_LOG("camera input will be dealing!");
    shared_ptr<FaceRecognitionInfo> camera_img = static_pointer_cast <
        FaceRecognitionInfo > (arg0);
    ret = Detection(camera_img);
  }
    return ret;
}
