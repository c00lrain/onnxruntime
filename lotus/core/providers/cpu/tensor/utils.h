#pragma once

namespace Lotus {

struct TensorPitches : std::vector<int64_t> {
  TensorPitches(const Tensor &tensor)
      : std::vector<int64_t>(tensor.Shape().NumDimensions(), 0) {
    auto &dims = tensor.Shape().GetDims();

    // The pitches is the size of the next inner axis. Aka the amount to move by one of the next inner axis.
    // For a tensor with shape(2,3,4,5) the values would be: (3*4*5, 4*5, 5, 1)
    // Note that the outermost '2' is never used, as you never need to move by the entire size of the outermost axis

    back() = 1;  // The innermost axis is 1 (single values)
    for (size_t i = size() - 1; i-- > 0;) {
      operator[](i) = operator[](i + 1) * dims[i + 1];
    }
  }
};

// This class is to iterate through the axes of an arbitrarily shaped tensor
// For example, a tensor with shape (2,3,4) will be iterated in this order:
// (0,0,x) (0,1,x) (0,2,x) (1,0,x) (1,1,x) (1,2,x)
// Note: The innermost axis is not iterated over since it's always special cased
struct TensorAxisCounters {
  TensorAxisCounters(const Tensor &tensor) : tensor_(tensor) {
    indices_.resize(tensor_.Shape().NumDimensions() - 1, 0);
    axis_ = indices_.size();

    // If a tensor has a shape, but one of the axes is 0 in size, there are no elements, so nothing to iterate
    if (tensor_.Shape().Size() == 0)
      running_ = false;
  }

  // Returns true if there was a carry to the next axis
  bool Increment() {
    if (axis_ == 0) {
      running_ = false;
      return false;
    }

    if (++indices_[--axis_] != tensor_.Shape()[axis_]) {
      axis_ = indices_.size();
      return false;
    }

    indices_[axis_] = 0;  // Reset the counter for this axis
    return true;          // There was a carry
  }

  size_t Axis() const { return axis_; }
  operator bool() const { return running_; }

 private:
  const Tensor &tensor_;
  bool running_{true};
  size_t axis_;
  std::vector<int64_t> indices_;  // There is no index for innermost axis since it's a special case
};

// A std::vector that holds the number of entries to skip to go to the next axis start given an extent in each axis
// This is used by the SliceIterator to iterate over a slice of a tensor
struct SliceSkips : std::vector<int64_t> {
  SliceSkips(const Tensor &tensor, gsl::span<const int64_t> extents)
      : std::vector<int64_t>(tensor.Shape().NumDimensions(), 0) {
    auto &dims = tensor.Shape().GetDims();
    LOTUS_ENFORCE(static_cast<ptrdiff_t>(dims.size()) == extents.size());
    size_t pitch = dims.back();
    back() = pitch - extents[size() - 1];
    for (size_t i = size() - 1; i-- > 0;) {
      auto prevPitch = pitch;
      pitch *= dims[i];
      operator[](i) = pitch - prevPitch * extents[i];
    }
  }
};

// This provides easy sequential iteration over a subset of a tensor given a span of starts & etents
template <typename T>
struct SliceIterator {
  SliceIterator(const Tensor &tensor, gsl::span<const int64_t> starts, gsl::span<const int64_t> extents)
      : tensor_(tensor), extents_(extents), skips_(tensor, extents), indices_(extents.size(), 0) {
    auto &dims = tensor_.Shape().GetDims();
    LOTUS_ENFORCE(static_cast<ptrdiff_t>(dims.size()) == starts.size() && static_cast<ptrdiff_t>(dims.size()) == extents.size());

    size_t pitch = 1;
    // Initial skip, so that input_ points to the first element to copy
    for (size_t i = dims.size(); i-- > 0;) {
      input_ += pitch * starts[i];
      pitch *= dims[i];
    }

    inner_extent_ = extents_[dims.size() - 1];
  }

  void AdvanceOverInnerExtent() {
    size_t axis = skips_.size() - 1;
    input_ += skips_[axis];
    while (axis && ++indices_[--axis] == extents_[axis]) {
      indices_[axis] = 0;
      input_ += skips_[axis];
    }
  }

  const T *operator++(int) {
    const T *input = input_++;
    if (++inner_counter_ == inner_extent_) {
      inner_counter_ = 0;
      AdvanceOverInnerExtent();
    }
    return input;
  }

  T *CopyInnermostAxis(T *output) {
    for (size_t i = 0; i < inner_extent_; i++)
      *output++ = *input_++;
    AdvanceOverInnerExtent();
    return output;
  }

 private:
  const Tensor &tensor_;
  const T *input_{tensor_.Data<T>()};
  gsl::span<const int64_t> extents_;
  size_t inner_counter_{}, inner_extent_;
  SliceSkips skips_;
  std::vector<int64_t> indices_;  // There is no index for innermost axis since it's a special case
};

}  // namespace Lotus