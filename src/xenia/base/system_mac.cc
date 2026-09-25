/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <spawn.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <cerrno>

extern char** environ;

#include "xenia/base/assert.h"
#include "xenia/base/logging.h"
#include "xenia/base/platform.h"
#include "xenia/base/string.h"
#include "xenia/base/system.h"

#if !XE_PLATFORM_IOS
#include <cstring>

// Use headers in third party to not depend on system sdl headers for building
#include "third_party/SDL2/include/SDL.h"
#endif  // !XE_PLATFORM_IOS

namespace xe {
namespace {
void OpenWithWorkspace(const std::string& target) {
  // Pass the path or URL as one argument: shell parsing corrupts spaces and
  // treats characters in game filenames as commands.
  char* argv[] = {const_cast<char*>("open"), const_cast<char*>("--"),
                  const_cast<char*>(target.c_str()), nullptr};
  pid_t process;
  const int result = posix_spawn(&process, "/usr/bin/open", nullptr, nullptr,
                                 argv, environ);
  if (result != 0) {
    XELOGE("Unable to open {}: {}", target, std::strerror(result));
    return;
  }
  int status;
  while (waitpid(process, &status, 0) == -1 && errno == EINTR) {
  }
}
}  // namespace

void LaunchWebBrowser(const std::string_view url) {
#if XE_PLATFORM_IOS
  // TODO(wmarti): Implement via UIApplication openURL.
  XELOGW("LaunchWebBrowser not yet implemented on iOS: {}", url);
#else
  OpenWithWorkspace(std::string(url));
#endif
}

void LaunchFileExplorer(const std::filesystem::path& path) {
#if XE_PLATFORM_IOS
  XELOGW("LaunchFileExplorer not supported on iOS: {}", path.string());
#else
  OpenWithWorkspace(path.string());
#endif
}

void ShowSimpleMessageBox(SimpleMessageBoxType type, std::string_view message) {
  Uint32 flags;
  const char* title;
  switch (type) {
    case SimpleMessageBoxType::Help:
      title = "Xenia Help";
      flags = SDL_MESSAGEBOX_INFORMATION;
      break;
    case SimpleMessageBoxType::Warning:
      title = "Xenia Warning";
      flags = SDL_MESSAGEBOX_WARNING;
      break;
    default:
    case SimpleMessageBoxType::Error:
      title = "Xenia Error";
      flags = SDL_MESSAGEBOX_ERROR;
      break;
  }
  const std::string message_copy(message);
  if (SDL_ShowSimpleMessageBox(flags, title, message_copy.c_str(), nullptr) != 0) {
    XELOGE("Unable to display message box: {} ({})", message, SDL_GetError());
  }
}

bool SetProcessPriorityClass(const uint32_t priority_class) {
  int nice_value = 0;
  switch (priority_class) {
    case 0:
      nice_value = 0;
      break;
    case 1:
      nice_value = -5;
      break;
    case 2:
      nice_value = -10;
      break;
    case 3:
      nice_value = -20;
      break;
    default:
      return false;
  }

  return setpriority(PRIO_PROCESS, 0, nice_value) == 0;
}

bool IsUseNexusForGameBarEnabled() { return false; }

}  // namespace xe
