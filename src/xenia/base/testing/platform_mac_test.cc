/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/base/platform.h"

#if XE_PLATFORM_MAC

#include <atomic>
#include <string>
#include <unistd.h>

#include "third_party/catch/include/catch.hpp"
#include "xenia/base/exception_handler.h"
#include "xenia/base/memory.h"
#include "xenia/base/threading.h"

namespace xe::base::test {
namespace {

struct TestAllocation {
  explicit TestAllocation(size_t length)
      : length(length),
        data(static_cast<uint8_t*>(memory::AllocFixed(
            nullptr, length, memory::AllocationType::kReserveCommit,
            memory::PageAccess::kReadWrite))) {}
  ~TestAllocation() {
    if (data) {
      memory::DeallocFixed(data, length, memory::DeallocationType::kRelease);
    }
  }
  size_t length;
  uint8_t* data;
};

}  // namespace

TEST_CASE("Darwin protects guest subpages without replacing their contents",
          "[mac][memory]") {
  const size_t page = memory::page_size();
  TestAllocation allocation(page * 2);
  REQUIRE(allocation.data != nullptr);
  allocation.data[0] = 0x42;
  allocation.data[page + 1] = 0x37;

  // An Xbox page can be smaller than the host page on Apple Silicon.
  const size_t offset = page > 4096 ? 4096 : 0;
  memory::PageAccess previous;
  REQUIRE(memory::Protect(allocation.data + offset, 4096,
                          memory::PageAccess::kReadOnly, &previous));
  REQUIRE(previous == memory::PageAccess::kReadWrite);
  size_t length = 0;
  memory::PageAccess access;
  REQUIRE(memory::QueryProtect(allocation.data, length, access));
  REQUIRE(access == memory::PageAccess::kReadOnly);
  REQUIRE(memory::AllocFixed(allocation.data + offset, 4096,
                             memory::AllocationType::kCommit,
                             memory::PageAccess::kReadWrite) ==
          allocation.data + offset);
  REQUIRE(allocation.data[0] == 0x42);
  REQUIRE(allocation.data[page + 1] == 0x37);
}

TEST_CASE("Darwin QueryProtect rejects a hole before an existing mapping",
          "[mac][memory]") {
  const size_t page = memory::page_size();
  TestAllocation allocation(page * 3);
  REQUIRE(allocation.data != nullptr);
  REQUIRE(memory::DeallocFixed(allocation.data + page, page,
                               memory::DeallocationType::kRelease));
  size_t length = 0;
  memory::PageAccess access;
  REQUIRE_FALSE(memory::QueryProtect(allocation.data + page, length, access));
  REQUIRE(memory::QueryProtect(allocation.data + page * 2, length, access));
  REQUIRE(access == memory::PageAccess::kReadWrite);
}

TEST_CASE("Darwin shared views retain aliasing after guest commit",
          "[mac][memory]") {
  const size_t page = memory::page_size();
  const std::string name = "xenia-mac-alias-test-with-long-name-" +
                           std::to_string(getpid());
  const auto handle = memory::CreateFileMappingHandle(
      name, page, memory::PageAccess::kReadWrite, true);
  REQUIRE(handle != memory::kFileMappingHandleInvalid);
  auto* first = static_cast<uint8_t*>(memory::MapFileView(
      handle, nullptr, page, memory::PageAccess::kReadWrite, 0));
  auto* second = static_cast<uint8_t*>(memory::MapFileView(
      handle, nullptr, page, memory::PageAccess::kReadWrite, 0));
  struct Cleanup {
    memory::FileMappingHandle handle;
    std::string name;
    uint8_t* first;
    uint8_t* second;
    size_t page;
    ~Cleanup() {
      if (first) memory::UnmapFileView(handle, first, page);
      if (second) memory::UnmapFileView(handle, second, page);
      memory::CloseFileMappingHandle(handle, name);
    }
  } cleanup{handle, name, first, second, page};
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  first[7] = 0x51;
  REQUIRE(second[7] == 0x51);
  REQUIRE(memory::Protect(second, page, memory::PageAccess::kNoAccess));
  REQUIRE(memory::AllocFixed(second, page, memory::AllocationType::kCommit,
                             memory::PageAccess::kReadWrite) == second);
  REQUIRE(second[7] == 0x51);
  second[7] = 0x93;
  REQUIRE(first[7] == 0x93);
}

TEST_CASE("Darwin dispatches queued callbacks at an alertable wait",
          "[mac][thread]") {
  using namespace std::chrono_literals;
  using namespace threading;
  auto event = Event::CreateAutoResetEvent(false);
  std::atomic<int> callbacks = 0;
  WaitResult result = WaitResult::kFailed;
  Thread::CreationParameters parameters{};
  parameters.create_suspended = true;
  auto thread = Thread::Create(parameters, [&] {
    result = Wait(event.get(), true, 1s);
  });
  REQUIRE(thread != nullptr);
  thread->QueueUserCallback([&] { callbacks.fetch_add(1); });
  thread->QueueUserCallback([&] { callbacks.fetch_add(1); });
  REQUIRE(thread->Resume());
  REQUIRE(Wait(thread.get(), false, 2s) == WaitResult::kSuccess);
  REQUIRE(result == WaitResult::kUserCallback);
  REQUIRE(callbacks == 2);
  // A repeated wait must not join an already joined pthread.
  REQUIRE(Wait(thread.get(), false, 0ms) == WaitResult::kSuccess);
}

TEST_CASE("Darwin event queries preserve signal state", "[mac][event]") {
  using namespace std::chrono_literals;
  using namespace threading;
  auto manual = Event::CreateManualResetEvent(true);
  REQUIRE(manual->Query().type == 0);
  REQUIRE(manual->Query().state == 1);
  REQUIRE(Wait(manual.get(), false, 0ms) == WaitResult::kSuccess);
  REQUIRE(manual->Query().state == 1);
  manual->Reset();
  REQUIRE(manual->Query().state == 0);
  auto automatic = Event::CreateAutoResetEvent(true);
  REQUIRE(automatic->Query().type == 1);
  REQUIRE(automatic->Query().state == 1);
  REQUIRE(Wait(automatic.get(), false, 0ms) == WaitResult::kSuccess);
  REQUIRE(automatic->Query().state == 0);
}

TEST_CASE("Darwin write faults can resume after protection repair",
          "[mac][memory][exception]") {
  TestAllocation allocation(memory::page_size());
  REQUIRE(allocation.data != nullptr);
  struct FaultState {
    TestAllocation* allocation;
    bool handled = false;
  } state{&allocation};
  const auto handler = +[](Exception* exception, void* context) {
    auto& state = *static_cast<FaultState*>(context);
    if (exception->code() != Exception::Code::kAccessViolation ||
        exception->fault_address() !=
            reinterpret_cast<uintptr_t>(state.allocation->data) ||
        exception->access_violation_operation() !=
            Exception::AccessViolationOperation::kWrite) {
      return false;
    }
    state.handled = memory::Protect(state.allocation->data,
                                    state.allocation->length,
                                    memory::PageAccess::kReadWrite);
    return state.handled;
  };
  ExceptionHandler::Install(handler, &state);
  struct Cleanup {
    ExceptionHandler::Handler handler;
    FaultState* state;
    ~Cleanup() { ExceptionHandler::Uninstall(handler, state); }
  } cleanup{handler, &state};
  REQUIRE(memory::Protect(allocation.data, allocation.length,
                          memory::PageAccess::kReadOnly));
  *static_cast<volatile uint8_t*>(allocation.data) = 0x64;
  REQUIRE(state.handled);
  REQUIRE(allocation.data[0] == 0x64);
}

TEST_CASE("Darwin running threads outlive closed public handles",
          "[mac][thread]") {
  using namespace std::chrono_literals;
  using namespace threading;
  for (bool explicit_exit : {false, true}) {
    auto start = Event::CreateManualResetEvent(false);
    auto destroyed = Event::CreateManualResetEvent(false);
    struct CallbackLifetime {
      Event* destroyed;
      ~CallbackLifetime() { destroyed->Set(); }
    };
    auto lifetime = std::make_shared<CallbackLifetime>();
    lifetime->destroyed = destroyed.get();
    Thread::CreationParameters parameters{};
    std::unique_ptr<Thread> thread;
    thread = Thread::Create(parameters, [&, lifetime, explicit_exit] {
      Wait(start.get(), false);
      // Mirrors XThread::Exit releasing its final handle before Thread::Exit.
      thread.reset();
      if (explicit_exit) Thread::Exit(0);
    });
    REQUIRE(thread != nullptr);
    lifetime.reset();
    start->Set();
    REQUIRE(Wait(destroyed.get(), false, 2s) == WaitResult::kSuccess);
    REQUIRE(thread == nullptr);
  }
}

}  // namespace xe::base::test

#endif  // XE_PLATFORM_MAC
