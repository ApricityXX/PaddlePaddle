// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "paddle/phi/kernels/unpool_kernel.h"

#include <algorithm>
#include <vector>

#include "paddle/phi/backends/cpu/cpu_context.h"
#include "paddle/phi/core/kernel_registry.h"
#include "paddle/phi/kernels/funcs/math_function.h"

namespace phi {

template <typename T, typename IndT, typename Context>
void Unpool(const Context& dev_ctx,
            const DenseTensor& x,
            const DenseTensor& indices,
            DenseTensor* out) {
  T* output_data = dev_ctx.template Alloc<T>(out);
  if (output_data) {
    phi::funcs::SetConstant<Context, T> set_zero;
    set_zero(dev_ctx, out, static_cast<T>(0));
  }
  const int batch_size = static_cast<int>(x.dims()[0]);
  const int input_height = static_cast<int>(x.dims()[2]);
  const int input_width = static_cast<int>(x.dims()[3]);
  const int output_channels = static_cast<int>(out->dims()[1]);
  const int output_height = static_cast<int>(out->dims()[2]);
  const int output_width = static_cast<int>(out->dims()[3]);
  int input_feasize = input_height * input_width;
  int output_feasize = output_height * output_width;
  const T* input_data = x.data<T>();
  const IndT* indices_data = indices.data<IndT>();
  for (int b = 0; b < batch_size; ++b) {
    for (int c = 0; c < output_channels; ++c) {
      for (int i = 0; i < input_feasize; ++i) {
        IndT index = indices_data[i];
        PADDLE_ENFORCE_LT(
            index,
            output_feasize,
            common::errors::InvalidArgument(
                "index should less than output tensor height * output tensor "
                "width. Expected %ld < %ld, but got "
                "%ld >= %ld. Please check input value.",
                index,
                output_feasize,
                index,
                output_feasize));
        output_data[index] = input_data[i];
      }
      input_data += input_feasize;
      indices_data += input_feasize;
      output_data += output_feasize;
    }
  }
}

template <typename T, typename Context>
void UnpoolKernel(const Context& dev_ctx,
                  const DenseTensor& x,
                  const DenseTensor& indices,
                  const std::vector<int>& ksize UNUSED,
                  const std::vector<int>& strides UNUSED,
                  const std::vector<int>& paddings UNUSED,
                  const IntArray& output_size UNUSED,
                  const std::string& data_format UNUSED,
                  DenseTensor* out) {
  const auto& indices_type = indices.dtype();
  if (indices_type == phi::DataType::INT32) {
    Unpool<T, int, Context>(dev_ctx, x, indices, out);
  } else {
    Unpool<T, int64_t, Context>(dev_ctx, x, indices, out);
  }
}

template <typename T, typename IndT, typename Context>
void Unpool3d(const Context& dev_ctx,
              const DenseTensor& x,
              const DenseTensor& indices,
              DenseTensor* out) {
  T* output_data = dev_ctx.template Alloc<T>(out);
  if (output_data) {
    phi::funcs::SetConstant<Context, T> set_zero;
    set_zero(dev_ctx, out, static_cast<T>(0));
  }
  const int batch_size = static_cast<int>(x.dims()[0]);
  const int input_depth = static_cast<int>(x.dims()[2]);
  const int input_height = static_cast<int>(x.dims()[3]);
  const int input_width = static_cast<int>(x.dims()[4]);
  const int output_channels = static_cast<int>(out->dims()[1]);
  const int output_depth = static_cast<int>(out->dims()[2]);
  const int output_height = static_cast<int>(out->dims()[3]);
  const int output_width = static_cast<int>(out->dims()[4]);
  int input_feasize = input_depth * input_height * input_width;
  int output_feasize = output_depth * output_height * output_width;
  const T* input_data = x.data<T>();
  const IndT* indices_data = indices.data<IndT>();
  for (int b = 0; b < batch_size; ++b) {
    for (int c = 0; c < output_channels; ++c) {
      for (int i = 0; i < input_feasize; ++i) {
        IndT index = indices_data[i];
        PADDLE_ENFORCE_LT(
            index,
            output_feasize,
            common::errors::InvalidArgument(
                "index should less than output tensor depth * output tensor "
                "height "
                "* output tensor width. Expected %ld < %ld, but got "
                "%ld >= %ld. Please check input value.",
                index,
                output_feasize,
                index,
                output_feasize));
        output_data[index] = input_data[i];
      }
      input_data += input_feasize;
      indices_data += input_feasize;
      output_data += output_feasize;
    }
  }
}

template <typename T, typename Context>
void Unpool3dKernel(const Context& dev_ctx,
                    const DenseTensor& x,
                    const DenseTensor& indices,
                    const std::vector<int>& ksize UNUSED,
                    const std::vector<int>& strides UNUSED,
                    const std::vector<int>& paddings UNUSED,
                    const std::vector<int>& output_size UNUSED,
                    const std::string& data_format UNUSED,
                    DenseTensor* out) {
  const auto& indices_type = indices.dtype();
  if (indices_type == phi::DataType::INT32) {
    Unpool3d<T, int, Context>(dev_ctx, x, indices, out);
  } else {
    Unpool3d<T, int64_t, Context>(dev_ctx, x, indices, out);
  }
}

}  // namespace phi

PD_REGISTER_KERNEL(
    unpool, CPU, ALL_LAYOUT, phi::UnpoolKernel, float, double, int64_t) {}

PD_REGISTER_KERNEL(
    unpool3d, CPU, ALL_LAYOUT, phi::Unpool3dKernel, float, double, int64_t) {}
