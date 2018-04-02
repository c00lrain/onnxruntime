#pragma once

#include "core/common/exceptions.h"
#include "core/common/status.h"
#include "core/common/logging/logging.h"
#include "core/framework/execution_frame.h"
#include "core/framework/kernel_def_builder.h"
#include "core/framework/ml_value.h"
#include "core/framework/tensor.h"
#include "core/graph/constants.h"
#include "core/graph/graph.h"
#include "core/graph/op.h"

namespace Lotus {
class OpKernelContext;

// A very light-weight class, which works as an aggregated
// view of all data needed for constructing a Kernel instance.
// NOTE: it does not own/hold any objects.
class OpKernelInfo {
 public:
  explicit OpKernelInfo(const LotusIR::Node& node,
                        const AllocatorInfo& allocator_info,
                        const KernelDef& kernel_def)
      : node_(node),
        allocator_info_(allocator_info),
        kernel_def_(kernel_def) {}

  //Get a single attribute
  template <typename T>
  Status GetAttr(const std::string& name, T* value) const;

  //Get repeated attributes
  template <typename T>
  Status GetAttrs(const std::string& name, std::vector<T>& values) const;

  const LotusIR::Node& node() const {
    return node_;
  }

  const AllocatorInfo& GetAllocatorInfo() const {
    return allocator_info_;
  }

  const KernelDef& GetKernelDef() const {
    return kernel_def_;
  }

 private:
  const LotusIR::Node& node_;
  const AllocatorInfo& allocator_info_;
  const KernelDef& kernel_def_;
};

class OpKernel {
 public:
  typedef std::function<void()> DoneCallback;

  explicit OpKernel(const OpKernelInfo& info)
      : op_kernel_info_(info) {
    // TODO: enable this
    // LOTUS_ENFORCE(nullptr != kernel_def, "kernel_def should be not nullptr.")
  }

  const LotusIR::Node& Node() const {
    return op_kernel_info_.node();
  }

  const Lotus::KernelDef& KernelDef() const {
    return op_kernel_info_.GetKernelDef();
  }

  virtual Status Compute(OpKernelContext* context) const = 0;

  virtual Status ComputeAsync(OpKernelContext* context,
                              DoneCallback done) const {
    UNUSED_PARAMETER(context);
    UNUSED_PARAMETER(done);
    LOTUS_NOT_IMPLEMENTED;
  }

  const AllocatorInfo& Allocator() const {
    return op_kernel_info_.GetAllocatorInfo();
  }

 protected:
  OpKernelInfo op_kernel_info_;
};

class OpKernelContext {
 public:
  typedef std::unordered_map<std::string, size_t> ArgMap;

  explicit OpKernelContext(ExecutionFrame* frame, const OpKernel* kernel, const Logging::Logger& logger);

  ~OpKernelContext(){};

  template <typename T>
  const T* Input(int index) const {
    return execution_frame_->GetValue<T>(arg_start_index_ + index);
  }

  // Fetch output (non-tensor) with specified index.
  template <typename T>
  T* Output(int index) {
    auto output_arg_index = arg_start_index_ + static_cast<int>(kernel_->Node().InputDefs().size()) + index;
    return execution_frame_->GetMutableValue<T>(output_arg_index);
  }

  // In the case that memory allocation has not been done for an output tensor,
  // The memory allocation will be done on-the-fly with given tensor shape.
  Tensor* Output(int index, const TensorShape& shape);

  const Logging::Logger& Logger() const {
    return *logger_;
  }

 private:
  ExecutionFrame* execution_frame_ = nullptr;
  const OpKernel* kernel_ = nullptr;
  const Logging::Logger* logger_ = nullptr;

  // The argument starting index in ExecutionFrame.
  int arg_start_index_ = -1;
};

typedef OpKernel* (*KernelCreateFn)(const OpKernelInfo&);

class KernelRegistry {
 public:
  // Register a kernel with kernel definition and function to create the kernel.
  Status Register(KernelDefBuilder& kernel_def_builder, KernelCreateFn kernel_creator);

  // Mainly for provide debug info
  std::vector<std::string> GetAllRegisteredOpNames() const;

  // factory functions should always return a unique_ptr for maximum flexibility
  // for its clients unless the factory is managing the lifecycle of the pointer
  // itself.
  // TODO(Task:132) Make usage of unique_ptr/shared_ptr as out param consistent
  Status CreateKernel(const ProviderType& provider_type,
                      const LotusIR::Node& node,
                      const AllocatorInfo& allocator_info,
                      std::unique_ptr<OpKernel>* op_kernel) const;

  static KernelRegistry& Instance() {
    static KernelRegistry kernel_registry;
    return kernel_registry;
  }

 private:
  KernelRegistry() = default;

  struct KernelCreateInfo {
    unique_ptr<KernelDef> kernel_def;  // Owned and stored in the global kernel registry.
    KernelCreateFn kernel_create_func;

    KernelCreateInfo(unique_ptr<KernelDef> definition,
                     KernelCreateFn create_func)
        : kernel_def(std::move(definition)),
          kernel_create_func(create_func) {}

    KernelCreateInfo(KernelCreateInfo&& other) : kernel_def(std::move(other.kernel_def)),
                                                 kernel_create_func(other.kernel_create_func) {
    }
  };

  // Check if the node's input/output/attributes are compatible with this
  // kernel_def, If so, the kernel defined by the kernel_def is used to
  // execute this node.
  static bool VerifyKernelDef(const LotusIR::Node& node,
                              const KernelDef& kernel_def);

  // Kernel create function map from op name to kernel creation info.
  std::multimap<std::string, KernelCreateInfo> kernel_creator_fn_map_;
};

#define REGISTER_KERNEL(kernel_def_builder, ...) \
  REGISTER_KERNEL_UNIQ_HELPER(__COUNTER__, kernel_def_builder, __VA_ARGS__)

#define REGISTER_KERNEL_UNIQ_HELPER(counter, kernel_def_builder, ...) \
  REGISTER_KERNEL_UNIQ(counter, kernel_def_builder, __VA_ARGS__)

#define REGISTER_KERNEL_UNIQ(counter, kernel_def_builder, ...)         \
  static Lotus::Common::Status kernel_def_builder_##counter##_status = \
      KernelRegistry::Instance().Register(                             \
          kernel_def_builder,                                          \
          [](const OpKernelInfo& info) -> OpKernel* { return new __VA_ARGS__(info); })

}  // namespace Lotus
