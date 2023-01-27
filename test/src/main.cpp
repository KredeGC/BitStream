#include "test.h"

int main(int argc, const char** argv)
{
    bitstream::test::unit::run_all_tests();

#ifdef _WIN32
        system("pause");
#endif
    
    return 0;
}