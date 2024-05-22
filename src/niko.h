#pragma once

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif
#ifndef COMPILE_TIME
#define COMPILE_TIME "unknown"
#endif

#define VERSION_STRING "niko " GIT_DESCRIBE " (" COMPILE_TIME ")"
