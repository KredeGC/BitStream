#pragma once

#ifdef BS_DEBUG_BREAK
#if defined(_WIN32) // Windows
#define BS_BREAKPOINT() __debugbreak()
#elif defined(__linux__) // Linux
#include <csignal>
#define BS_BREAKPOINT() std::raise(SIGTRAP)
#else // Non-supported
#define BS_BREAKPOINT() throw
#endif

#define BS_ASSERT(x) if (!(x)) { BS_BREAKPOINT(); return false; }
#else // BS_DEBUG_BREAK
#define BS_ASSERT(x) if (!(x)) { return false; }

#define BS_BREAKPOINT() throw
#endif // BS_DEBUG_BREAK