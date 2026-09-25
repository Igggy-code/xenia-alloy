/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/ui/metal/metal_util.h"

#include <cstring>

namespace xe::ui::metal::util {
MTL::Library* CreateLibrary(MTL::Device* device, const void* data, size_t size,
                            NS::Error** error) {
  if (size >= 4 && !std::memcmp(data, "MTLB", 4)) {
    dispatch_data_t binary = dispatch_data_create(
        data, size, nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    MTL::Library* library = device->newLibrary(binary, error);
    dispatch_release(binary);
    return library;
  }
  // Source blobs include their trailing NUL. Keep compile options consistent
  // with the offline XeSL compiler (including its default math behavior).
  auto* source = NS::String::string(static_cast<const char*>(data),
                                    NS::UTF8StringEncoding);
  return device->newLibrary(source, nullptr, error);
}
}  // namespace xe::ui::metal::util
