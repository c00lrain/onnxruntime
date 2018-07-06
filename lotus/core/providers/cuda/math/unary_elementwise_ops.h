#pragma once
#include "core/providers/cuda/cuda_common.h"

namespace Lotus {
namespace Cuda {

struct UnaryElementwisePreparation {
  const Tensor* input_tensor = nullptr;
  Tensor* output_tensor = nullptr;
};

class UnaryElementwise : public CudaKernel {
 protected:
  UnaryElementwise(const OpKernelInfo& info) : CudaKernel(info) {}
  Status Compute(OpKernelContext*) const override {
    return Status(LOTUS, FAIL);  // should not reach here
  }
  Status Prepare(OpKernelContext* context, UnaryElementwisePreparation* p) const;
};

template <typename T>
class Abs final : public UnaryElementwise {
 public:
  Abs(const OpKernelInfo& info) : UnaryElementwise(info) {}
  Status Compute(OpKernelContext* context) const override;
};

template <typename T>
class Neg final : public UnaryElementwise {
 public:
  Neg(const OpKernelInfo& info) : UnaryElementwise(info) {}
  Status Compute(OpKernelContext* context) const override;
};

template <typename T>
class Floor final : public UnaryElementwise {
 public:
  Floor(const OpKernelInfo& info) : UnaryElementwise(info) {}
  Status Compute(OpKernelContext* context) const override;
};

template <typename T>
class Ceil final : public UnaryElementwise {
 public:
  Ceil(const OpKernelInfo& info) : UnaryElementwise(info) {}
  Status Compute(OpKernelContext* context) const override;
};

template <typename T>
class Reciprocal final : public UnaryElementwise {
 public:
  Reciprocal(const OpKernelInfo& info) : UnaryElementwise(info) {}
  Status Compute(OpKernelContext* context) const override;
};

template <typename T>
class Sqrt final : public UnaryElementwise {
 public:
  Sqrt(const OpKernelInfo& info) : UnaryElementwise(info) {}
  Status Compute(OpKernelContext* context) const override;
};

template <typename T>
class Log final : public UnaryElementwise {
 public:
  Log(const OpKernelInfo& info) : UnaryElementwise(info) {}
  Status Compute(OpKernelContext* context) const override;
};

template <typename T>
class Exp final : public UnaryElementwise {
 public:
  Exp(const OpKernelInfo& info) : UnaryElementwise(info) {}
  Status Compute(OpKernelContext* context) const override;
};

}  // namespace Cuda
}  // namespace Lotus
