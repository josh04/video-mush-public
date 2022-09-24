//
//  CubeLUT.h
//  video-mush
//
//  Created by Josh McNamee on 02/03/2015.
//
//

#ifndef video_mush_CubeLUT_h
#define video_mush_CubeLUT_h

#include <string>
#include <vector>
#include <fstream>
#include <memory>

//#include <Mush Core/opencl.hpp>
namespace mush {
    class opencl;
}

namespace cl {
    class Image3D;
}

class CubeLUT {
public:
    typedef std::vector <float> tableRow;
    typedef std::vector <tableRow> table1D;
    typedef std::vector <table1D> table2D;
    typedef std::vector <table2D> table3D;
    
    enum LUTState { OK = 0, NotInitialized = 1,
        ReadError = 10, WriteError, PrematureEndOfFile, LineError,
        UnknownOrRepeatedKeyword = 20, TitleMissingQuote, DomainBoundsReversed,
        LUTSizeOutOfRange, CouldNotParseTableData };
    
    LUTState status;
    std::string title;
    tableRow domainMin;
    tableRow domainMax;
    table1D LUT1D;
    table3D LUT3D;
    
    float outputTable[32][32][32][4];
    
    CubeLUT ( void ) {
        status = NotInitialized;
    };
    
    LUTState LoadCubeFile ( std::ifstream & infile );
    LUTState SaveCubeFile ( std::ofstream & outfile );
    
    cl::Image3D * get(std::shared_ptr<mush::opencl> context, const char * resourcesDir);

    
private:
    std::string ReadLine ( std::ifstream & infile, char lineSeparator);
    tableRow ParseTableRow ( const std::string & lineOfText );
};

#endif
