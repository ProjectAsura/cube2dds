//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>
#include <DirectXTex.h>
#include "cube_lut.h"


//-----------------------------------------------------------------------------
//      拡張子を取り除いたファイルパスを取得します.
//-----------------------------------------------------------------------------
std::string GetPathWithoutExtA( const char* filePath )
{
    std::string path = filePath;
    auto idx = path.find_last_of( "." );
    if ( idx != std::string::npos )
    {
        return path.substr( 0, idx );
    }

    return path;
}

//-----------------------------------------------------------------------------
//      ワイド文字列に変換します.
//-----------------------------------------------------------------------------
std::wstring ToStringW( const std::string& value )
{
    auto length = MultiByteToWideChar(CP_ACP, 0, value.c_str(), int(value.size() + 1), nullptr, 0 );
    auto buffer = new wchar_t[length];

    MultiByteToWideChar(CP_ACP, 0, value.c_str(), int(value.size() + 1),  buffer, length );

    std::wstring result( buffer );
    delete[] buffer;

    return result;
}

//-----------------------------------------------------------------------------
//      DDSファイルに変換します.
//-----------------------------------------------------------------------------
bool ConvertToDDS(const char* path, const CubeLUT& cube)
{
    auto N = cube.lut3D.size();
    if (N == 0)
    {
        return false;
    }

    DirectX::ScratchImage image;
    image.Initialize3D(DXGI_FORMAT_R16G16B16A16_FLOAT, N, N, N, 1);

    for(auto z=0; z<N; ++z)
    {
        for(auto y=0; y<N; ++y)
        {
            for(auto x=0; x<N; ++x)
            {
                auto r = cube.lut3D[x][y][z][0];
                auto g = cube.lut3D[x][y][z][1];
                auto b = cube.lut3D[x][y][z][2];

                // TODO : ピクセルに格納.
            }
        }
    }


    // 出力パス.
    auto outputPath = ToStringW(path);

    // DDSファイルに保存.
    auto hr = DirectX::SaveToDDSFile(
        image.GetImages(),
        image.GetImageCount(),
        image.GetMetadata(),
        DirectX::DDS_FLAGS_NONE,
        outputPath.c_str());
    if (FAILED(hr))
    {
        return false;
    }

    // 正常終了.
    return true;
}

//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
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

    auto outpath = GetPathWithoutExtA(argv[1]) + ".dds";
    if (!ConvertToDDS(outpath.c_str(), cube))
    {
        return -1;
    }

    return 0;
}