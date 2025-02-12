/**
 * Copyright (c) 2020-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the MIT license (https://opensource.org/licenses/MIT)
 **/
/**
 * @file handler.hpp
 * @brief Handlers base class for HailoRT tracer mechanism
 **/

#ifndef _HAILO_HANDLER_HPP_
#define _HAILO_HANDLER_HPP_

#include "hailo/hailort.h"
#include "hailo/stream.hpp"

#include "vdevice/scheduler/scheduler_base.hpp"

namespace hailort
{

struct Trace
{
    Trace(const std::string &name)
        : name(name)
    {}

    virtual ~Trace() = default;

    uint64_t timestamp = 0;
    std::string name;
};

struct InitTrace : Trace
{
    InitTrace() : Trace("init") {}
};

struct InitProfilerProtoTrace : Trace
{
    InitProfilerProtoTrace () : Trace("init_profiler_proto") {}
};

struct CoreOpIdleTrace : Trace
{
    CoreOpIdleTrace(const device_id_t &device_id, scheduler_core_op_handle_t core_op_handle)
        : Trace("core_op_idle"), device_id(device_id), core_op_handle(core_op_handle)
    {}

    device_id_t device_id;
    scheduler_core_op_handle_t core_op_handle;
};

struct AddDeviceTrace : Trace
{
    AddDeviceTrace(const device_id_t &device_id, const std::string &device_arch)
        : Trace("add_device_trace"), device_id(device_id), device_arch(device_arch)
    {}

    device_id_t device_id;
    std::string device_arch;
};

struct MonitorStartTrace : Trace
{
    MonitorStartTrace(uint32_t device_count)
        : Trace("scheduler_start"), device_count(device_count)
    {}

    uint32_t device_count = 0;
};

struct AddCoreOpTrace : Trace
{
    AddCoreOpTrace(const device_id_t &device_id, const std::string &core_op_name, uint64_t timeout, uint32_t threshold,
        scheduler_core_op_handle_t handle, bool is_nms, int batch_size)
        : Trace("add_core_op"), device_id(device_id), core_op_name(core_op_name), timeout(timeout), threshold(threshold),
            core_op_handle(handle), is_nms(is_nms), batch_size(batch_size)
    {}

    device_id_t device_id;
    std::string core_op_name;
    uint64_t timeout = 0;
    uint32_t threshold = 0;
    scheduler_core_op_handle_t core_op_handle = INVALID_CORE_OP_HANDLE;
    bool is_nms = false;
    int batch_size = 0;
};

struct CreateCoreOpInputStreamsTrace : Trace
{
    CreateCoreOpInputStreamsTrace(const device_id_t &device_id, const std::string &core_op_name, const std::string &stream_name, uint32_t queue_size,
        scheduler_core_op_handle_t core_op_handle)
        : Trace("create_input_stream"), device_id(device_id), core_op_name(core_op_name), stream_name(stream_name), queue_size(queue_size),
        core_op_handle(core_op_handle)
    {}

    device_id_t device_id;
    std::string core_op_name;
    std::string stream_name;
    uint32_t queue_size;
    scheduler_core_op_handle_t core_op_handle;
};

struct CreateCoreOpOutputStreamsTrace : Trace
{
    CreateCoreOpOutputStreamsTrace(const device_id_t &device_id, const std::string &core_op_name, const std::string &stream_name, uint32_t queue_size,
        scheduler_core_op_handle_t core_op_handle)
        : Trace("create_output_stream"), device_id(device_id), core_op_name(core_op_name), stream_name(stream_name), queue_size(queue_size),
        core_op_handle(core_op_handle)
    {}

    device_id_t device_id;
    std::string core_op_name;
    std::string stream_name;
    uint32_t queue_size;
    scheduler_core_op_handle_t core_op_handle;
};

struct WriteFrameTrace : Trace
{
    WriteFrameTrace(scheduler_core_op_handle_t core_op_handle, const std::string &queue_name)
        : Trace("write_frame"), core_op_handle(core_op_handle), queue_name(queue_name)
    {}

    scheduler_core_op_handle_t core_op_handle;
    std::string queue_name;
};

struct InputVdmaDequeueTrace : Trace
{
    InputVdmaDequeueTrace(const device_id_t &device_id, scheduler_core_op_handle_t core_op_handle, const std::string &queue_name)
        : Trace("input_vdma_dequeue"), device_id(device_id), core_op_handle(core_op_handle), queue_name(queue_name)
    {}

    device_id_t device_id;
    scheduler_core_op_handle_t core_op_handle;
    std::string queue_name;
};

struct ReadFrameTrace : Trace
{
    ReadFrameTrace(scheduler_core_op_handle_t core_op_handle, const std::string &queue_name)
        : Trace("read_frame"), core_op_handle(core_op_handle), queue_name(queue_name)
    {}

    scheduler_core_op_handle_t core_op_handle;
    std::string queue_name;
};

struct OutputVdmaEnqueueTrace : Trace
{
    OutputVdmaEnqueueTrace(const device_id_t &device_id, scheduler_core_op_handle_t core_op_handle, const std::string &queue_name)
        : Trace("output_vdma_enqueue"), device_id(device_id), core_op_handle(core_op_handle), queue_name(queue_name)
    {}

    device_id_t device_id;
    scheduler_core_op_handle_t core_op_handle;
    std::string queue_name;
};

struct SwitchCoreOpTrace : Trace
{
    SwitchCoreOpTrace(const device_id_t &device_id, scheduler_core_op_handle_t handle)
        : Trace("switch_core_op"), device_id(device_id), core_op_handle(handle)
    {}

    device_id_t device_id;
    scheduler_core_op_handle_t core_op_handle;
};

struct SetCoreOpTimeoutTrace : Trace
{
    SetCoreOpTimeoutTrace(vdevice_core_op_handle_t handle, const std::chrono::milliseconds timeout)
        : Trace("set_timeout"), core_op_handle(handle), timeout(timeout)
    {}

    vdevice_core_op_handle_t core_op_handle;
    std::chrono::milliseconds timeout;
};

struct SetCoreOpThresholdTrace : Trace
{
    SetCoreOpThresholdTrace(vdevice_core_op_handle_t handle, uint32_t threshold)
        : Trace("set_threshold"), core_op_handle(handle), threshold(threshold)
    {}

    vdevice_core_op_handle_t core_op_handle;
    uint32_t threshold;
};

struct SetCoreOpPriorityTrace : Trace
{
    SetCoreOpPriorityTrace(vdevice_core_op_handle_t handle, uint8_t priority)
        : Trace("set_priority"), core_op_handle(handle), priority(priority)
    {}

    vdevice_core_op_handle_t core_op_handle;
    uint8_t priority;
};

struct OracleDecisionTrace : Trace
{
    OracleDecisionTrace(bool reason_idle, device_id_t device_id, vdevice_core_op_handle_t handle, bool over_threshold,
        bool over_timeout)
        : Trace("switch_core_op_decision"), reason_idle(reason_idle), device_id(device_id), core_op_handle(handle),
        over_threshold(over_threshold), over_timeout(over_timeout)
    {}

    bool reason_idle;
    device_id_t device_id;
    vdevice_core_op_handle_t core_op_handle;
    bool over_threshold;
    bool over_timeout;
};

struct DumpProfilerState : Trace
{
    DumpProfilerState() : Trace("dump_profiler_state") {}
};

class Handler
{
public:
    virtual ~Handler() = default;

    virtual void handle_trace(const InitTrace&) {};
    virtual void handle_trace(const AddCoreOpTrace&) {};
    virtual void handle_trace(const CreateCoreOpInputStreamsTrace&) {};
    virtual void handle_trace(const CreateCoreOpOutputStreamsTrace&) {};
    virtual void handle_trace(const WriteFrameTrace&) {};
    virtual void handle_trace(const InputVdmaDequeueTrace&) {};
    virtual void handle_trace(const ReadFrameTrace&) {};
    virtual void handle_trace(const OutputVdmaEnqueueTrace&) {};
    virtual void handle_trace(const SwitchCoreOpTrace&) {};
    virtual void handle_trace(const MonitorStartTrace&) {};
    virtual void handle_trace(const CoreOpIdleTrace&) {};
    virtual void handle_trace(const AddDeviceTrace&) {};
    virtual void handle_trace(const SetCoreOpTimeoutTrace&) {};
    virtual void handle_trace(const SetCoreOpThresholdTrace&) {};
    virtual void handle_trace(const SetCoreOpPriorityTrace&) {};
    virtual void handle_trace(const OracleDecisionTrace&) {};
    virtual void handle_trace(const DumpProfilerState&) {};
    virtual void handle_trace(const InitProfilerProtoTrace&) {};

};

struct JSON;

}

#endif /* _HAILO_HANDLER_HPP */