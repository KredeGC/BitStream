#include "test.h"

#include <bitstream/bitstream.h>

int main(int argc, const char** argv)
{
    bitstream::test::unit::run_all_tests();

#ifdef _WIN32
        system("pause");
#endif
    
    return 0;
}