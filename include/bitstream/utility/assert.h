#pragma once

#if 1 //#ifdef BS_ENABLE_ASSERT
#define BS_ASSERT(x) if (!(x)) return false
#else
#define BS_ASSERT(x)
#endif