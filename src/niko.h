#pragma once

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif

#ifndef BUILD_TYPE
#define BUILD_TYPE "unknown"
#endif

// Include generated timestamp header if available
#ifdef __has_include
#if __has_include("timestamp.h")
#include "timestamp.h"
#endif
#endif

#ifndef COMPILE_TIME
#define COMPILE_TIME "unknown"
#endif

#define VERSION_STRING "niko " GIT_DESCRIBE " " BUILD_TYPE " (" COMPILE_TIME ") http://github.com/mikea/niko"
