/**
 * Copyright (c) 2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the MIT license (https://opensource.org/licenses/MIT)
**/
/**
 * @file ssd_post_process.hpp
 * @brief SSD post process
 *
 * Reference code: https://github.com/winfredsu/ssd_postprocessing/blob/master/ssd_postprocessing.py
 **/

#ifndef _HAILO_SSD_POST_PROCESS_HPP_
#define _HAILO_SSD_POST_PROCESS_HPP_

#include "net_flow/ops/nms_post_process.hpp"
#include "net_flow/ops/op_metadata.hpp"

namespace hailort
{
namespace net_flow
{

struct SSDPostProcessConfig
{
    // The image height.
    float32_t image_height = 0;

    // The image width.
    float32_t image_width = 0;

    uint32_t centers_scale_factor = 0;

    uint32_t bbox_dimensions_scale_factor = 0;

    uint32_t ty_index = 0;
    uint32_t tx_index = 0;
    uint32_t th_index = 0;
    uint32_t tw_index = 0;

    std::map<std::string, std::string> reg_to_cls_inputs;

    // A vector of anchors, each element in the vector represents the anchors for a specific layer
    // Each layer anchors vector is structured as {w,h} pairs.
    // Each anchor is mapped by 2 keys:
    //     1. reg input
    //     2. cls input
    std::map<std::string, std::vector<float32_t>> anchors;

    // Indicates whether boxes should be normalized (and clipped)
    bool normalize_boxes = false;
};

class SSDOpMetadata : public NmsOpMetadata
{
public:
    static Expected<std::shared_ptr<OpMetadata>> create(const std::unordered_map<std::string, BufferMetaData> &inputs_metadata,
                                                        const std::unordered_map<std::string, BufferMetaData> &outputs_metadata,
                                                        const NmsPostProcessConfig &nms_post_process_config,
                                                        const SSDPostProcessConfig &ssd_post_process_config,
                                                        const std::string &network_name);
    std::string get_op_description() override;
    hailo_status validate_format_info() override;
    SSDPostProcessConfig &ssd_config() { return m_ssd_config;};

private:
    SSDPostProcessConfig m_ssd_config;
    SSDOpMetadata(const std::unordered_map<std::string, BufferMetaData> &inputs_metadata,
                       const std::unordered_map<std::string, BufferMetaData> &outputs_metadata,
                       const NmsPostProcessConfig &nms_post_process_config,
                       const SSDPostProcessConfig &ssd_post_process_config,
                       const std::string &network_name)
        : NmsOpMetadata(inputs_metadata, outputs_metadata, nms_post_process_config, "SSD-Post-Process", network_name, OperationType::SSD)
        , m_ssd_config(ssd_post_process_config)
    {}

    hailo_status validate_params() override;
};

class SSDPostProcessOp : public NmsPostProcessOp
{

public:
    static Expected<std::shared_ptr<Op>> create(std::shared_ptr<SSDOpMetadata> metadata);

    hailo_status execute(const std::map<std::string, MemoryView> &inputs, std::map<std::string, MemoryView> &outputs) override;

    static const uint32_t DEFAULT_Y_OFFSET_IDX = 0;
    static const uint32_t DEFAULT_X_OFFSET_IDX = 1;
    static const uint32_t DEFAULT_H_OFFSET_IDX = 2;
    static const uint32_t DEFAULT_W_OFFSET_IDX = 3;

private:
    SSDPostProcessOp(std::shared_ptr<SSDOpMetadata> metadata)
        : NmsPostProcessOp(static_cast<std::shared_ptr<NmsOpMetadata>>(metadata))
        , m_metadata(metadata)
    {}
    std::shared_ptr<SSDOpMetadata> m_metadata;

    template<typename DstType = float32_t, typename SrcType>
    void extract_bbox_classes(const hailo_bbox_float32_t &dims_bbox, SrcType *cls_data, const BufferMetaData &cls_metadata, uint32_t cls_index,
        std::vector<DetectionBbox> &detections, std::vector<uint32_t> &classes_detections_count)
    {
        const auto &nms_config = m_metadata->nms_config();
        if (nms_config.cross_classes) {
            // Pre-NMS optimization. If NMS checks IoU over different classes, only the maximum class is relevant
            auto max_id_score_pair = get_max_class<DstType, SrcType>(cls_data, cls_index, 0, 1,
                cls_metadata.quant_info, cls_metadata.padded_shape.width);
            auto bbox = dims_bbox;
            bbox.score = max_id_score_pair.second;
            if (max_id_score_pair.second >= nms_config.nms_score_th) {
                detections.emplace_back(DetectionBbox(bbox, max_id_score_pair.first));
                classes_detections_count[max_id_score_pair.first]++;
            }
        } else {
            for (uint32_t class_index = 0; class_index < nms_config.number_of_classes; class_index++) {
                auto class_id = class_index;
                if (nms_config.background_removal) {
                    if (nms_config.background_removal_index == class_index) {
                        // Ignore if class_index is background_removal_index
                        continue;
                    }
                    else if (0 == nms_config.background_removal_index) {
                        // background_removal_index will always be the first or last index.
                        // If it is the first one we need to reduce all classes id's in 1.
                        // If it is the last one we just ignore it in the previous if case.
                        class_id--;
                    }
                }

                auto class_entry_idx = cls_index + (class_index * cls_metadata.padded_shape.width);
                auto class_score = Quantization::dequantize_output<DstType, SrcType>(cls_data[class_entry_idx],
                    cls_metadata.quant_info);
                if (class_score < nms_config.nms_score_th) {
                    continue;
                }
                auto bbox = dims_bbox;
                bbox.score = class_score;
                detections.emplace_back(bbox, class_id);
                classes_detections_count[class_id]++;
            }
        }
    }

    template<typename DstType = float32_t, typename SrcType>
    hailo_status extract_bbox_detections(const std::string &reg_input_name, const std::string &cls_input_name,
        const MemoryView &reg_buffer, const MemoryView &cls_buffer,
        uint64_t x_index, uint64_t y_index, uint64_t w_index, uint64_t h_index,
        uint32_t cls_index, float32_t wa, float32_t ha, float32_t xcenter_a, float32_t ycenter_a,
        std::vector<DetectionBbox> &detections, std::vector<uint32_t> &classes_detections_count)
    {
        const auto &inputs_metadata = m_metadata->inputs_metadata();
        const auto &ssd_config = m_metadata->ssd_config();
        assert(contains(inputs_metadata, reg_input_name));
        const auto &shape = inputs_metadata.at(reg_input_name).shape;
        const auto &reg_quant_info = inputs_metadata.at(reg_input_name).quant_info;
        SrcType *reg_data = (SrcType*)reg_buffer.data();
        auto *cls_data = cls_buffer.data();
        auto tx = Quantization::dequantize_output<DstType, SrcType>(reg_data[x_index], reg_quant_info);
        auto ty = Quantization::dequantize_output<DstType, SrcType>(reg_data[y_index], reg_quant_info);
        auto tw = Quantization::dequantize_output<DstType, SrcType>(reg_data[w_index], reg_quant_info);
        auto th = Quantization::dequantize_output<DstType, SrcType>(reg_data[h_index], reg_quant_info);
        tx /= static_cast<float32_t>(ssd_config.centers_scale_factor);
        ty /= static_cast<float32_t>(ssd_config.centers_scale_factor);
        tw /= static_cast<float32_t>(ssd_config.bbox_dimensions_scale_factor);
        th /= static_cast<float32_t>(ssd_config.bbox_dimensions_scale_factor);
        auto w = exp(tw) * wa;
        auto h = exp(th) * ha;
        auto x_center = tx * wa + xcenter_a;
        auto y_center = ty * ha + ycenter_a;
        auto x_min = (x_center - (w / 2.0f));
        auto y_min = (y_center - (h / 2.0f));
        auto x_max = (x_center + (w / 2.0f));
        auto y_max = (y_center + (h / 2.0f));

        // TODO: HRT-10033 - Fix support for clip_boxes and normalize_output
        // Currently `normalize_boxes` is always false
        if (ssd_config.normalize_boxes) {
            x_min = Quantization::clip(x_min, 0, static_cast<float32_t>(shape.width-1));
            y_min = Quantization::clip(y_min, 0, static_cast<float32_t>(shape.height-1));
            x_max = Quantization::clip(x_max, 0, static_cast<float32_t>(shape.width-1));
            y_max = Quantization::clip(y_max, 0, static_cast<float32_t>(shape.height-1));
        }
        hailo_bbox_float32_t dims_bbox{y_min, x_min, y_max, x_max, 0};
        assert(contains(inputs_metadata, cls_input_name));
        const auto &cls_metadata = inputs_metadata.at(cls_input_name);
        if (cls_metadata.format.type == HAILO_FORMAT_TYPE_UINT8) {
            extract_bbox_classes<DstType, uint8_t>(dims_bbox, (uint8_t*)cls_data, cls_metadata,
                cls_index, detections, classes_detections_count);
        } else if (cls_metadata.format.type == HAILO_FORMAT_TYPE_UINT16) {
            extract_bbox_classes<DstType, uint16_t>(dims_bbox, (uint16_t*)cls_data, cls_metadata,
                cls_index, detections, classes_detections_count);
        } else if (cls_metadata.format.type == HAILO_FORMAT_TYPE_FLOAT32) {
            extract_bbox_classes<DstType, float32_t>(dims_bbox, (float32_t*)cls_data, cls_metadata,
                cls_index, detections, classes_detections_count);
        } else {
            CHECK_SUCCESS(HAILO_INVALID_ARGUMENT, "SSD post-process received invalid cls input type: {}",
                cls_metadata.format.type);
        }
        return HAILO_SUCCESS;
    }

    /**
     * Extract bboxes with confidence level higher then @a confidence_threshold from @a buffer and add them to @a detections.
     *
     * @param[in] reg_input_name                Name of the regression input
     * @param[in] cls_input_name                Name of the classes input
     * @param[in] reg_buffer                    Buffer containing the boxes data after inference
     * @param[in] cls_buffer                    Buffer containing the classes ids after inference.
     * @param[inout] detections                 A vector of ::DetectionBbox objects, to add the detected bboxes to.
     * @param[inout] classes_detections_count   A vector of uint32_t, to add count of detections count per class to.
     *
     * @return Upon success, returns ::HAILO_SUCCESS. Otherwise, returns a ::hailo_status error.
    */
    hailo_status extract_detections(const std::string &reg_input_name, const std::string &cls_input_name,
        const MemoryView &reg_buffer, const MemoryView &cls_buffer,
        std::vector<DetectionBbox> &detections, std::vector<uint32_t> &classes_detections_count);
};

}

}

#endif // _HAILO_SSD_POST_PROCESSING_HPP_