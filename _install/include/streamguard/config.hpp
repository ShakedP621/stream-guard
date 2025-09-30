// Generated: do not edit directly. Edit config/streamguard_config.hpp.in instead.
#pragma once

// Version macros (from CMake project version)
#define STREAMGUARD_VERSION_MAJOR 0
#define STREAMGUARD_VERSION_MINOR 1
#define STREAMGUARD_VERSION_PATCH 0
#define STREAMGUARD_VERSION_STRING "0.1.0"

// Export/import macro
// If building as shared on Windows, use __declspec; otherwise these are no-ops.
#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(STREAMGUARD_BUILD_SHARED) && STREAMGUARD_BUILD_SHARED
    #if defined(STREAMGUARD_BUILDING_LIB)
      #define STREAMGUARD_API __declspec(dllexport)
    #else
      #define STREAMGUARD_API __declspec(dllimport)
    #endif
  #else
    #define STREAMGUARD_API
  #endif
#else
  // On non-Windows, rely on default visibility unless building shared with -fvisibility=hidden
  #define STREAMGUARD_API
#endif
