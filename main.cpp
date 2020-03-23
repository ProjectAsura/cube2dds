
#include "cube_lut.h"

int main(int argc, char** argv)
{
    if (argc < 2 || 3 < argc)
    {
        return -1;
    }

    CubeLUT cube;
    auto ret = cube.LoadCubeFile(argv[1]);
    if (ret != CubeLUT::LUTState::OK)
    {
        return -1;
    }

    // TODO : DDSに保存.

    return 0;
}