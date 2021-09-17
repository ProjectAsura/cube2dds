#include "cube_lut.h"
#include <iostream>
#include <sstream>


std::string CubeLUT::ReadLine(std::ifstream& stream, char line_separator)
{
    const char kCommentMarker = '#';
    std::string textLine("");

    while(textLine.size() == 0 || textLine[0] == kCommentMarker)
    {
        if (stream.eof())
        {
            status = LUTState::PrematureEndOfFile;
            break;
        }

        std::getline(stream, textLine, line_separator);

        if (stream.fail())
        {
            status = LUTState::ReadError;
            break;
        }
    }

    return textLine;
}

std::vector<float>  CubeLUT::ParseTableRow(const std::string& line_of_text)
{
    int N = 3;
    TableRow f(N);
    std::istringstream line(line_of_text);
 
    for(auto i=0; i<N; ++i)
    {
        line >> f[i];
        if (line.fail())
        {
            status = LUTState::CouldNotParseTableData;
            break;
        }
    }

    return f;
}

CubeLUT::LUTState CubeLUT::LoadCubeFile(const char* path)
{
    std::ifstream stream;

    stream.open(path, std::ios::in);
    if (!stream.is_open())
    {
        return LUTState::CouldNotParseTableData;
    }

    // Set defaults.
    status = LUTState::OK;
    title.clear();
    domainMin = TableRow(3, 0.0f);
    domainMax = TableRow(3, 1.0f);
    lut1D.clear();
    lut3D.clear();

    // Read file data by line.
    const char kNewlineCharacter = '\n';
    char lineSeparator = kNewlineCharacter;

    // sniff use of legacy lineSeparator
    const char kCarriageReturnCharacter = '\r';
    for(auto i=0; i<255; ++i)
    {
        char inc = stream.get();
        if (inc == kNewlineCharacter)
            break;

        if (inc == kCarriageReturnCharacter)
        {
            if (stream.get() == kNewlineCharacter)
                break;

            lineSeparator = kCarriageReturnCharacter;
            std::clog << "INFO : This file use non-compliant line separator \\r (0x0D)" << std::endl;
            break;
        }

        if (i > 250)
        {
            status = LUTState::LineError;
            break;
        }
    }

    stream.seekg(0);
    stream.clear();

    // read keywords
    int N, CntTitle, CntSize, CntMin, CntMax;

    // each keyword to occur zero or one time.
    N = CntTitle = CntSize = CntMin = CntMax = 0;

    while(status == LUTState::OK)
    {
        long linePos = long(stream.tellg());
        std::string lineOfText = ReadLine(stream, lineSeparator);

        if (!status == LUTState::OK)
            break;

        // Parse keywords and parameters
        std::istringstream line(lineOfText);
        std::string keyword;
        line >> keyword;

        if ("+" < keyword && keyword < ":")
        {
            // lines of table data come after keywords
            // restore stream pos to re-read line of data.
            stream.seekg(linePos);
            break;
        }
        else if (keyword == "TITLE" && CntTitle++ == 0)
        {
            const char kQUOTE = '"';
            char startOfTitle;
            line >> startOfTitle;
            if (startOfTitle != kQUOTE)
            {
                status = LUTState::TitleMissingQuote;
                break;
            }

            std::getline(line, title, kQUOTE);
        }
        else if (keyword == "DOMAIN_MIN" && CntMin++ == 0)
        {
            line >> domainMin[0] >> domainMin[1] >> domainMin[2];
        }
        else if (keyword == "DOMAIN_MAX" && CntMax++ == 0)
        {
            line >> domainMax[0] >> domainMax[1] >> domainMax[2];
        }
        else if (keyword == "LUT_1D_SIZE" && CntSize++ == 0)
        {
            line >> N;
            if (N < 2 || N > 65536)
            {
                status = LUTState::LUTSizeOutOfRange;
                break;
            }

            lut1D = Table1D(N, TableRow(3));
        }
        else if (keyword == "LUT_3D_SIZE" && CntSize++ == 0)
        {
            line >> N;
            if (N < 2 || N > 256)
            {
                status = LUTState::LUTSizeOutOfRange;
                break;
            }

            lut3D = Table3D(N, Table2D(N, Table1D(N, TableRow(3))));
        }
        else
        {
            status = LUTState::UnknownOrRepeatedKeyword;
            break;
        }
    }

    if (status == LUTState::OK && CntSize == 0)
        status = LUTState::LUTSizeOutOfRange;

    if (status == LUTState::OK 
     && (domainMin[0] >= domainMax[0])
     || (domainMin[1] >= domainMax[1])
     || (domainMin[2] >= domainMax[2]))
        status = LUTState::DomainBoundsReserved;

    // read lines of table data
    if (lut1D.size() > 0)
    {
        N = int(lut1D.size());
        for(auto i=0; i<N && status == LUTState::OK; ++i)
        {
            lut1D[i] = ParseTableRow(ReadLine(stream, lineSeparator));
        }
    }
    else
    {
        N = int(lut3D.size());
        // NOTE that r loops fastest
        for(auto b=0; b<N && status == LUTState::OK; ++b)
        {
            for(auto g=0; g<N && status == LUTState::OK; ++g)
            {
                for(auto r=0; r<N && status == LUTState::OK; ++r)
                {
                    lut3D[r][g][b] = ParseTableRow(ReadLine(stream, lineSeparator));
                }
            }
        }
    }

    return status;
}

CubeLUT::LUTState CubeLUT::SaveCubeFile(const char* path)
{
    std::ofstream stream;
    stream.open(path, std::ios::out);

    if (!stream.is_open())
    {
        return LUTState::CouldNotParseTableData;
    }

    if (!status == LUTState::OK)
    {
        return status;
    }

    // Write keywords
    const char kSPACE = ' ';
    const char kQUOTE = '"';

    if (title.size() > 0)
        stream << "TITLE" << kSPACE << kQUOTE << title << kQUOTE << std::endl;
    stream << "# Created by cube2dds" << std::endl;
    stream << "DOMAIN_MIN" << kSPACE << domainMin[0] << kSPACE << domainMin[1] << kSPACE << domainMin[2] << std::endl;
    stream << "DOMAIN_MAX" << kSPACE << domainMax[1] << kSPACE << domainMax[1] << kSPACE << domainMax[2] << std::endl;

    // Write LUT data
    if (lut1D.size() > 0)
    {
        int N = int(lut1D.size());
        stream << "LUT_1D_SIZE" << kSPACE << N << std::endl;
        for(auto i=0; i<N && stream.good(); ++i)
        {
            stream << lut1D[i][0] << kSPACE << lut1D[i][1] << kSPACE << lut1D[i][2] << std::endl;
        }
    }
    else
    {
        int N = int(lut3D.size());
        stream << "LUT_3D_SIZE" << kSPACE << N << std::endl;

        // NOTE that r loops fastest
        for(auto b=0; b<N && stream.good(); ++b)
        {
            for(auto g=0; g<N && stream.good(); ++g)
            {
                for(auto r=0; r<N && stream.good(); ++r)
                {
                    stream << lut3D[r][g][b][0] << kSPACE
                           << lut3D[r][g][b][1] << kSPACE
                           << lut3D[r][g][b][2] << kSPACE
                           << std::endl;
                }
            }
        }
    }

    stream.flush();

    auto ret = (stream.good()) ? LUTState::OK : LUTState::WriteError;
    stream.close();

    return ret;
}

#if 0
//void CubeLUT::CreateAsDefault(size_t N)
//{
//    status = LUTState::OK;
//    title.clear();
//    domainMin = TableRow(3, 0.0f);
//    domainMax = TableRow(3, 1.0f);
//    lut1D.clear();
//    lut3D.clear();
//
//    lut3D = Table3D(N, Table2D(N, Table1D(N, TableRow(3))));
//
//    for(auto b=0; b<N; ++b)
//    {
//        for(auto g=0; g<N; ++g)
//        {
//            for(auto r=0; r<N; ++r)
//            {
//                lut3D[r][g][b][0] = float(r) / float(N);
//                lut3D[r][g][b][1] = float(g) / float(N);
//                lut3D[r][g][b][2] = float(b) / float(N);
//            }
//        }
//    }
//}
#endif