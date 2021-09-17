#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>
#include <vector>
#include <fstream>


///////////////////////////////////////////////////////////////////////////////
// CubeLUT class
///////////////////////////////////////////////////////////////////////////////
class CubeLUT
{
    //=========================================================================
    // list of friend classes and methods.
    //=========================================================================
    /* NOTHING */

public:
    using TableRow = std::vector<float>;
    using Table1D  = std::vector<TableRow>;
    using Table2D  = std::vector<Table1D>;
    using Table3D  = std::vector<Table2D>;

    enum LUTState
    {
        OK                          = 0,
        NotInitialized              = 1,
        ReadError                   = 10,
        WriteError                  = 11,
        PrematureEndOfFile          = 12,
        LineError                   = 13,
        UnknownOrRepeatedKeyword    = 20,
        TitleMissingQuote           = 21,
        DomainBoundsReserved        = 22,
        LUTSizeOutOfRange           = 23,
        CouldNotParseTableData      = 24,
    };

    //=========================================================================
    // public variables.
    //=========================================================================
    LUTState        status;
    std::string     title;
    TableRow        domainMin;
    TableRow        domainMax;
    Table1D         lut1D;
    Table3D         lut3D;

    //=========================================================================
    // public methods.
    //=========================================================================
    CubeLUT()
    : status(LUTState::NotInitialized)
    { /* DO_NOTHING */ }

    LUTState LoadCubeFile(const char* path);
    LUTState SaveCubeFile(const char* path);

#if 0
    //void CreateAsDefault(size_t N);
#endif

private:
    //=========================================================================
    // private variables.
    //=========================================================================
    /* NOTHING */

    //=========================================================================
    // private methods.
    //=========================================================================
    std::string ReadLine(std::ifstream& stream, char line_separator);
    TableRow ParseTableRow(const std::string& line_of_text);
};

