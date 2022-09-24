//
//  CubeLUT.cpp
//  video-mush
//
//  Created by Josh McNamee on 02/03/2015.
//
//

// CubeLUT.cpp
#include "CubeLUT.h"
#include <Mush Core/opencl.hpp>
#include <iostream>
#include <sstream>

extern "C" void putLog(std::string s);

namespace {
    using std::string;
    using std::vector;
    using std::istringstream;
    using std::ifstream;
    using std::ofstream;
    using std::clog;
    using std::endl;
    using std::cout;
}

string CubeLUT:: ReadLine ( ifstream & infile, char lineSeparator ) {
    // Skip empty lines and comments
    const char CommentMarker = '#';
    string textLine("");
    while ( textLine.size() == 0 || textLine[0] == CommentMarker ) {
        if ( infile.eof() ) { status = PrematureEndOfFile; break; }
        getline ( infile, textLine, lineSeparator );
        if ( infile.fail() ) { status = ReadError; break; }
    }
    return textLine;
}

vector <float> CubeLUT:: ParseTableRow ( const string & lineOfText ) {
    int N = 3;
    tableRow f ( N );
    istringstream line ( lineOfText );
    for (int i = 0; i < N; i++) {
        line >> f[i];
        if ( line.fail() ) { status = CouldNotParseTableData; break; };
    }
    return f;
}

CubeLUT:: LUTState CubeLUT:: LoadCubeFile ( ifstream & infile ) {
    // Set defaults
    status = OK;
    title.clear();
    domainMin = tableRow ( 3, 0.0 );
    domainMax = tableRow ( 3, 1.0 );
    LUT1D.clear();
    LUT3D.clear();
    // Read file data line by line
    const char NewlineCharacter = '\n';
    char lineSeparator = NewlineCharacter;
    // sniff use of legacy lineSeparator
    const char CarriageReturnCharacter = '\r';
    for (int i = 0; i < 255; i++) {
        char inc = infile.get();
        if ( inc == NewlineCharacter ) break;
        if ( inc == CarriageReturnCharacter ) {
            if ( infile.get() == NewlineCharacter ) break;
            lineSeparator = CarriageReturnCharacter;
            clog << "INFO: This file uses non-compliant line separator \\r (0x0D)" << endl;
            break;
        }
        if ( i > 250 ) { status = LineError; break; }
    }
    infile.seekg ( 0 );
    infile.clear();
    // read keywords
    int N, CntTitle, CntSize, CntMin, CntMax;
    // each keyword to occur zero or one time
    N = CntTitle = CntSize = CntMin = CntMax = 0;
    while ( status == OK ) {
        long linePos = infile.tellg();
        string lineOfText = ReadLine ( infile, lineSeparator );
        if ( ! status == OK ) break;
        // Parse keywords and parameters
        istringstream line ( lineOfText );
        string keyword;
        line >> keyword;
        if ( "+" < keyword && keyword < ":" ) {
            // lines of table data come after keywords
            // restore stream pos to re-read line of data
            infile.seekg ( linePos );
            break;
        } else if ( keyword == "TITLE" && CntTitle++ == 0 ) {
            const char QUOTE = '"';
            char startOfTitle;
            line >> startOfTitle;
            if ( startOfTitle != QUOTE ) { status = TitleMissingQuote; break; }
            getline ( line, title, QUOTE ); // read to "
        } else if ( keyword == "DOMAIN_MIN" && CntMin++ == 0 ) {
            line >> domainMin[0] >> domainMin[1] >> domainMin[2];
        } else if ( keyword == "DOMAIN_MAX" && CntMax++ == 0 ) {
            line >> domainMax[0] >> domainMax[1] >> domainMax[2];
        } else if ( keyword == "LUT_1D_SIZE" && CntSize++ == 0 ) {
            line >> N;
            if ( N < 2 || N > 65536 ) { status = LUTSizeOutOfRange; break; }
            LUT1D = table1D ( N, tableRow ( 3 ) );
        } else if ( keyword == "LUT_3D_SIZE" && CntSize++ == 0 ) {
            line >> N;
            if ( N < 2 || N > 256 ) { status = LUTSizeOutOfRange; break; }
            LUT3D = table3D ( N, table2D ( N, table1D ( N, tableRow ( 3 ) ) ) );
        } else { status = UnknownOrRepeatedKeyword; break; }
        if ( line.fail() ) { status = ReadError; break; }
    } // read keywords
    if ( status == OK && CntSize == 0 ) status = LUTSizeOutOfRange;
    if ( status == OK && ( domainMin[0] >= domainMax[0] || domainMin[1] >= domainMax[1]
                          || domainMin[2] >= domainMax[2] ) )
        status = DomainBoundsReversed;
    // read lines of table data
    if ( LUT1D.size() > 0 ) {
        N = LUT1D.size();
        for ( int i = 0; i < N && status == OK; i++ ) {
            LUT1D [i] = ParseTableRow ( ReadLine ( infile, lineSeparator ) );
        }
    } else {
        N = LUT3D.size();
        // NOTE that r loops fastest
        for ( int b = 0; b < N && status == OK; b++ ) {
            for ( int g = 0; g < N && status == OK; g++ ) {
                for ( int r = 0; r < N && status == OK; r++ ) {
                    LUT3D[r][g][b] = ParseTableRow
                    ( ReadLine ( infile, lineSeparator ) );
                }
            }
        }
    } // read 3D LUT

    return status;
}


cl::Image3D * CubeLUT::get(std::shared_ptr<mush::opencl> context, const char * resourcesDir) {
    
    enum { OK = 0, ErrorOpenInFile = 100, ErrorOpenOutFile };
    
    // Load a Cube
    ifstream infile ( std::string(resourcesDir) + "./resources/cube.lut" );
    
    int ret = ErrorOpenInFile;
    
    if ( !infile.good() ) {
        std::stringstream strm;
        strm << "Could not open input file " << resourcesDir << "./resources/cube.lut" << endl;
        throw std::runtime_error(strm.str().c_str());
    } else {
        ret = LoadCubeFile ( infile );
        infile.close();
    }

    if ( ret != OK ) {
		throw std::runtime_error("Could not parse the cube info in the input file.");
    } else {
        putLog("Great job, well done!");
    }
    
    unsigned int dim = LUT3D.size();
    
    for (int k = 0; k < 32; ++k) {
        for (int j = 0; j < 32; ++j) {
            for (int i = 0; i < 32; ++i) {
                outputTable[k][j][i][3] = 1.0f;
                for (int d = 0; d < 3; ++d) {
                    outputTable[k][j][i][d] = LUT3D[k][j][i][d];
                }
            }
        }
    }
    
    cl::size_t<3> origin;
    cl::size_t<3> region;
    origin[0] = 0; origin[1] = 0; origin[2] = 0;
    region[0] = dim; region[1] = dim; region[2] = dim;
    
    cl::Image3D * upCube = context->cubeImage(dim, dim, dim);
    auto queue = context->getQueue();
    cl::Event event;
    queue->enqueueWriteImage(*upCube, CL_TRUE, origin, region, 0, 0, outputTable, NULL, &event);
    event.wait();
    return upCube;
}

