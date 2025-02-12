/**
 * Copyright (c) 2020-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the MIT license (https://opensource.org/licenses/MIT)
 **/
/**
 * @file nms_stream.hpp
 * @brief Wraps some stream object that reads bbox/bursts into a stream object that reads nms frames.
 **/

#ifndef _NMS_STREAM_HPP_
#define _NMS_STREAM_HPP_


#include "common/utils.hpp"
#include "hailo/hailort_common.hpp"

#include "stream_common/stream_internal.hpp"
#include "stream_common/async_stream_base.hpp"

namespace hailort
{

static const uint64_t NMS_DELIMITER = 0xFFFFFFFFFFFFFFFF;
static const uint64_t NMS_IMAGE_DELIMITER = 0xFFFFFFFFFFFFFFFE;
static const uint64_t NMS_H15_PADDING = 0xFFFFFFFFFFFFFFFD;

enum class NMSBurstState {
    NMS_BURST_STATE_WAITING_FOR_DELIMETER = 0,
    NMS_BURST_STATE_WAITING_FOR_IMAGE_DELIMETER = 1,
    NMS_BURST_STATE_WAITING_FOR_PADDING = 2,
};

// static class that helps receives and reads the nms ouput stream according to the differnet burst mode, type and size.
// For explanation on the different burst modes and types and state machine and logic of the class please check out the cpp.
class NMSStreamReader {
public:
    static hailo_status read_nms(OutputStreamBase &stream, void *buffer, size_t offset, size_t size);
private:
    static hailo_status read_nms_bbox_mode(OutputStreamBase &stream, void *buffer, size_t offset);
    static hailo_status read_nms_burst_mode(OutputStreamBase &stream, void *buffer, size_t offset, size_t buffer_size);
    static hailo_status advance_state_machine(NMSBurstState *burst_state, const uint64_t current_bbox,
        const hailo_nms_burst_type_t burst_type, const uint32_t num_classes, size_t *num_delimeters_received,
        bool *can_stop_reading_burst, const size_t burst_offset, const size_t burst_size, size_t *burst_index);
};

class NmsReaderThread final {
public:

    NmsReaderThread(std::shared_ptr<OutputStreamBase> base_stream, size_t max_queue_size);
    ~NmsReaderThread();

    NmsReaderThread(const NmsReaderThread &) = delete;
    NmsReaderThread &operator=(const NmsReaderThread &) = delete;

    hailo_status launch_transfer(TransferRequest &&transfer_request);

    size_t get_max_ongoing_transfers() const;

private:

    void signal_thread_quit();
    void process_transfer_requests();

    std::shared_ptr<OutputStreamBase> m_base_stream;
    const size_t m_queue_max_size;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cond;
    // TODO: use SpscQueue (HRT-10554)
    std::queue<TransferRequest> m_queue;
    // m_should_quit is used to quit the thread (called on destruction)
    bool m_should_quit;
    std::thread m_worker_thread;
};

// NMS requires multiple reads from the device + parsing the output. Hence, a background thread is needed.
// This class opens a worker thread that processes nms transfers, signalling the user's callback upon completion.
// read_async adds transfer requests to a producer-consumer queue
class NmsOutputStream : public AsyncOutputStreamBase {
public:
    static Expected<std::shared_ptr<NmsOutputStream>> create(std::shared_ptr<OutputStreamBase> base_stream,
        const LayerInfo &edge_layer, size_t max_queue_size, EventPtr core_op_activated_event);

    virtual hailo_stream_interface_t get_interface() const override;

    NmsOutputStream(std::shared_ptr<OutputStreamBase> base_stream, const LayerInfo &edge_layer, size_t max_queue_size,
        EventPtr core_op_activated_event, hailo_status &status) :
            AsyncOutputStreamBase(edge_layer, base_stream->get_interface(), std::move(core_op_activated_event), status),
            m_base_stream(base_stream),
            m_reader_thread(base_stream, max_queue_size)
    {}

protected:
    virtual Expected<std::unique_ptr<StreamBufferPool>> allocate_buffer_pool() override;
    virtual size_t get_max_ongoing_transfers() const override;
    virtual hailo_status read_async_impl(TransferRequest &&transfer_request) override;
    virtual hailo_status activate_stream_impl() override;
    virtual hailo_status deactivate_stream_impl() override;

    std::shared_ptr<OutputStreamBase> m_base_stream;

    NmsReaderThread m_reader_thread;
};

} /* namespace hailort */

#endif /* _NMS_STREAM_HPP_ */