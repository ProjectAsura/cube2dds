//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>
#include <DirectXTex.h>
#include "cube_lut.h"


//-----------------------------------------------------------------------------
//      32bit 浮動小数から 16bit 浮動小数に変換します.
//-----------------------------------------------------------------------------
inline uint16_t ToHalf(float value)
{
    union FP32
    {
        uint32_t u;
        float    f;
    };

    uint16_t result;

    // ビット列を崩さないままuint32_t型に変換.
    FP32 fp32 = {};
    fp32.f = value;

    uint32_t bit = fp32.u;

    // float表現の符号bitを取り出し.
    uint32_t sign   = ( bit & 0x80000000U) >> 16U;

    // 符号部を削ぎ落す.
    bit = bit & 0x7FFFFFFFU;

    // halfとして表現する際に値がデカ過ぎる場合は，無限大にクランプ.
    if (bit > 0x47FFEFFFU)
    {
        result = 0x7FFFU;
    }
    else
    {
        // 正規化されたhalfとして表現するために小さすぎる値は正規化されていない値に変換.
        if ( bit < 0x38800000U)
        {
            uint32_t shift = 113U - ( bit >> 23U);
            bit = (0x800000U | ( bit & 0x7FFFFFU)) >> shift;
        }
        else
        {
            // 正規化されたhalfとして表現するために指数部に再度バイアスをかける
            bit += 0xC8000000U;
        }

        // half型表現にする.
        result = (( bit + 0x0FFFU + (( bit >> 13U) & 1U)) >> 13U) & 0x7FFFU;
    }

    // 符号部を付け足して返却.
    return static_cast<uint16_t>(result | sign);
}

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
    auto processed = false;

    // 1D LUT Only.
    if (!cube.lut1D.empty() && cube.lut3D.empty())
    {
        DirectX::ScratchImage image;

        auto N = cube.lut1D.size();
        image.Initialize1D(DXGI_FORMAT_R16G16B16A16_FLOAT, N, 1, 1);

        auto pixels = reinterpret_cast<uint16_t*>(image.GetPixels());
        auto idx = 0;
        for(size_t x=0; x<N; ++x) {

            auto r = cube.lut1D[x][0];
            auto g = cube.lut1D[x][1];
            auto b = cube.lut1D[x][2];

            pixels[idx + 0] = ToHalf(r);
            pixels[idx + 1] = ToHalf(g);
            pixels[idx + 2] = ToHalf(b);
            pixels[idx + 3] = ToHalf(1.0f);
            idx += 4;
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

        processed = true;
    }
    // 3D LUT Only.
    else if (!cube.lut3D.empty() && cube.lut1D.empty())
    {
        DirectX::ScratchImage image;

        auto N = cube.lut3D.size();
        image.Initialize3D(DXGI_FORMAT_R16G16B16A16_FLOAT, N, N, N, 1);

        auto pixels = reinterpret_cast<uint16_t*>(image.GetPixels());
        auto idx = 0;
        for(size_t z=0; z<N; ++z)
        {
            for(size_t y=0; y<N; ++y)
            {
                for(size_t x=0; x<N; ++x)
                {
                    auto r = cube.lut3D[x][y][z][0];
                    auto g = cube.lut3D[x][y][z][1];
                    auto b = cube.lut3D[x][y][z][2];

                    pixels[idx + 0] = ToHalf(r);
                    pixels[idx + 1] = ToHalf(g);
                    pixels[idx + 2] = ToHalf(b);
                    pixels[idx + 3] = ToHalf(1.0f);
                    idx += 4;
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

        processed = true;
    }
    // Shaper LUT + 3D LUT.
    else if (!cube.lut1D.empty() && !cube.lut3D.empty())
    {
        // TODO : Implementation.
    }

    if (!processed)
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