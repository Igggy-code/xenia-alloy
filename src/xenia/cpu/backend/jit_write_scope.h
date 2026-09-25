/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Xenia contributors. All rights reserved.                    *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */
#ifndef XENIA_CPU_BACKEND_JIT_WRITE_SCOPE_H_
#define XENIA_CPU_BACKEND_JIT_WRITE_SCOPE_H_

#include "xenia/base/platform.h"
#if XE_PLATFORM_APPLE && XE_ARCH_ARM64
#include <pthread.h>
#endif

namespace xe::cpu::backend {
// All executable storage on Apple Silicon belongs to one MAP_JIT mapping.
// Write permission is thread-local; never execute guest code within this scope.
// Nesting preserves write permission until the outermost writer has finished.
class JitWriteScope {
 public:
  JitWriteScope() {
#if XE_PLATFORM_APPLE && XE_ARCH_ARM64
    if (depth_++ == 0) {
      pthread_jit_write_protect_np(0);
    }
#endif
  }
  ~JitWriteScope() {
#if XE_PLATFORM_APPLE && XE_ARCH_ARM64
    if (--depth_ == 0) {
      pthread_jit_write_protect_np(1);
    }
#endif
  }
  JitWriteScope(const JitWriteScope&) = delete;
  JitWriteScope& operator=(const JitWriteScope&) = delete;
 private:
#if XE_PLATFORM_APPLE && XE_ARCH_ARM64
  inline static thread_local unsigned int depth_ = 0;
#endif
};
}  // namespace xe::cpu::backend

#endif  // XENIA_CPU_BACKEND_JIT_WRITE_SCOPE_H_
