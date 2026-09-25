/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_UI_METAL_METAL_UTIL_H_
#define XENIA_UI_METAL_METAL_UTIL_H_

#include <utility>

#include "xenia/ui/metal/metal_provider.h"

namespace xe {
namespace ui {
namespace metal {
namespace util {

// Accepts either a binary metallib or a NUL-terminated source blob generated
// from the current XeSL files. The returned library is owned by the caller.
MTL::Library* CreateLibrary(MTL::Device* device, const void* data, size_t size,
                            NS::Error** error);


const MTL::ResourceOptions kStorageModePrivate =
    MTL::ResourceStorageModePrivate;

}
}  // namespace metal
}  // namespace ui
}  // namespace xe

#endif
