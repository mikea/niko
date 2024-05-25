#pragma once

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif

#ifndef COMPILE_TIME
#define COMPILE_TIME "unknown"
#endif

#ifndef BUILD_TYPE
#define BUILD_TYPE "unknown"
#endif

#define VERSION_STRING "niko " GIT_DESCRIBE " " BUILD_TYPE " (" COMPILE_TIME ") http://github.com/mikea/niko"
