#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#ifndef NOMINMAX
#define NOMINMAX                        // Prevent windows.h min/max macros from breaking std headers
#endif
// Windows Header Files
#ifdef _WIN32
#include <windows.h>
#endif
