// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/cpu/tensor/unsqueeze.h"
#include "utils.h"
using namespace ::onnxruntime::common;

namespace onnxruntime {

ONNX_CPU_OPERATOR_KERNEL(
    Unsqueeze,
    1,
    KernelDefBuilder()
        .Alias(0, 0)
        .TypeConstraint("T", DataTypeImpl::AllTensorTypes()),
    Unsqueeze);

Status UnsqueezeBase::PrepareCompute(OpKernelContext* ctx, Prepare& p) const {
  const Tensor* X = ctx->Input<Tensor>(0);
  ORT_ENFORCE(X != nullptr);
  auto& input_tensor = *X;

  // New dimension count is the current dimensions + the number of entries in axes_
  // Initialize output_dims to 0 in each axis initially
  std::vector<int64_t> output_dims(axes_.size() + input_tensor.Shape().GetDims().size(), 0);

  // Set all axes_ indices to 1 in output_dims and check for duplicates
  for (size_t axis : axes_) {
    if (axis >= output_dims.size())
      return Status(ONNXRUNTIME, INVALID_ARGUMENT, "'axes' has an out of range axis");
    if (output_dims[axis] != 0)
      return Status(ONNXRUNTIME, INVALID_ARGUMENT, "'axes' has a duplicate axis");
    output_dims[axis] = 1;
  }

  // Now fill in the zero entries with the existing shape
  {
    auto begin = input_tensor.Shape().GetDims().cbegin();
    for (auto& axisSize : output_dims) {
      if (axisSize == 0)
        axisSize = *begin++;
    }
    assert(begin == input_tensor.Shape().GetDims().cend());
  }

  TensorShape output_shape(output_dims);
  p.output_tensor = ctx->Output(0, output_shape);
  p.input_tensor = &input_tensor;
  return Status::OK();
}

Status Unsqueeze::Compute(OpKernelContext* ctx) const {
  Prepare p;
  ORT_RETURN_IF_ERROR(PrepareCompute(ctx, p));

  CopyCpuTensor(p.input_tensor, p.output_tensor);

  return Status::OK();
}
}  // namespace onnxruntime
