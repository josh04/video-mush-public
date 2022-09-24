//
//  profile.hpp
//  compressor
//
//  Created by Jonathan Hatchett on 25/07/2012.
//  Copyright (c) 2012. All rights reserved.
//

#ifndef PROFILE_HPP
#define PROFILE_HPP

#include <stdio.h>
#include <iostream>
#include <boost/timer/timer.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <Mush Core/metricReporter.hpp>

#include <Mush Core/mushLog.hpp>

#ifdef _PROFILE
#undef _PROFILE
#endif
#define _PROFILE 3

class Profile {
public:

#if _PROFILE > 0
	boost::timer::cpu_timer profileTotal;
	unsigned int frames;
	boost::timer::nanosecond_type total;
#endif
#if _PROFILE > 2
	boost::timer::cpu_timer profileReadFromPipe, 
							 profileWriteToPipe, 
							 profileWriteToGPU,
							 profileReadFromGPU, 
							 profileExecute,
							 profileWait;
#endif	
#if _PROFILE > 3
	boost::timer::cpu_timer profileRgba, 
							 profileLuma,	
							 profileBilateral, 
							 profileHmean, 
							 profileCopyHmean, 
							 profileTonemap,
							 profileChroma; 
#endif
	
	inline void init() {
		std::stringstream strm;
#if _PROFILE > 0
		frames = 0;
		total = 0;
#endif
#if _PROFILE > 1
		strm << boost::str(boost::format("%8s %7s   |") % "Frame:" %"Time:");
//		fprintf(stdout, "%12s %12s ", "Frame No:", "Frame Time: |");
#endif
#if _PROFILE > 2
		strm << boost::str(boost::format(" %7s %10s %9s %9s %9s %9s  |") % "Read:" %"Write GPU:" %"Execute:" %"Read GPU:" %"Write:" %"Wait:");
//		fprintf(stdout, "%12s %12s %12s %12s %12s %12s |", "Read:", "Write GPU:", "Execute", "Read GPU:", "Write:", "Wait");
#endif
#if _PROFILE > 3
		strm << boost::str(boost::format("%12s %12s %12s %12s %12s %12s %12s ") %"RGB->RGBA:" %"Luma:" %"Bilateral:" %"Hmean Copy:" %"Hmean Exe:" %"Tonemap:" %"Chroma:");
//		fprintf(stdout, "%12s %12s %12s %12s %12s %12s %12s ", "RGB->RGBA:", "Luma:", "Bilateral:", "Hmean Copy:", "Hmean Exe:", "Tonemap:", "Chroma:");
#endif
#if _PROFILE > 1
		putLog(strm.str().c_str());
//		fprintf(stdout, "\n");
#endif
	}
	
	void report() {
		std::stringstream strm;
#if _PROFILE > 1
		strm << boost::str(boost::format("%8i %7fs |") %frames %(profileTotal.elapsed().wall / 1000000000.0));
#endif
#if _PROFILE > 2
		strm << boost::str(boost::format(" %7fs %7fs %7fs %7fs %7fs %7fs |") % (profileReadFromPipe.elapsed().wall / 1000000000.0) \
			%(profileWriteToGPU.elapsed().wall / 1000000000.0) %(profileExecute.elapsed().wall / 1000000000.0) \
			%(profileReadFromGPU.elapsed().wall / 1000000000.0)	%(profileWriteToPipe.elapsed().wall / 1000000000.0) \
			%(profileWait.elapsed().wall / 1000000000.0));
#endif
#if _PROFILE > 3
/*		fprintf(stdout, "%11fs %11fs %11fs %11fs %11fs %11fs %11fs ",
			   	profileRgba.elapsed().wall / 1000000000.0,
			   	profileLuma.elapsed().wall / 1000000000.0,
			   	profileBilateral.elapsed().wall / 1000000000.0,
			   	profileCopyHmean.elapsed().wall / 1000000000.0,
			   	profileHmean.elapsed().wall / 1000000000.0,
			   	profileTonemap.elapsed().wall / 1000000000.0,
				profileChroma.elapsed().wall / 1000000000.0);*/
#endif
#if _PROFILE > 1
        if (frames > 5000) {
            if (frames % 1000 == 0) {
                putLog(strm.str().c_str());
            }
        } else if (frames > 1000) {
            if (frames % 100 == 0) {
                putLog(strm.str().c_str());
            }
        } else if (frames > 100) {
            if (frames % 10 == 0) {
                putLog(strm.str().c_str());
            }
        } else {
            putLog(strm.str().c_str());
        }
//		fprintf(stdout, "\n");
#endif
	}
	
    void finalReport() {
#if _PROFILE > 0
		putLog("");
		putLog("Total frame time: " + boost::str(boost::format("%.2fs") % (total / 1000000000.0)));
		putLog("Average frame time: " + boost::str(boost::format("%.2fs") % ((total / (double)frames) / 1000000000.0)));
		putLog("Average FPS: " + boost::str(boost::format("%.2fs") % (1.0 / ((total / (double)frames) / 1000000000.0))));

//		std::cout << std::endl;
//		std::cout << "Total frame time: " << (total / 1000000000.0) << "s" << std::endl;
//		std::cout << "Average frame time: " << ((total / (double)frames) / 1000000000.0) << "s" << std::endl;
//		std::cout << "Average FPS: " << (1.0/((total / (double)frames) / 1000000000.0)) << std::endl;
#endif
	}
	
	inline void start() {
#if _PROFILE > 0
		profileTotal.start();
#endif
	}
	
	inline void stop() {
#if _PROFILE > 0
		profileTotal.stop();
		total += profileTotal.elapsed().wall;
		frames++;
#endif
	}
	
	inline void inReadStart() {
#if _PROFILE > 2
		profileReadFromPipe.start();
#endif
	}
	
	inline void inReadStop() {
#if _PROFILE > 2
		profileReadFromPipe.stop();
#endif
	}
	
	inline void executionStart() {
#if _PROFILE > 2
		profileExecute.start();
#endif
	}
	
	inline void executionStop() {
#if _PROFILE > 2
		profileExecute.stop();
#endif
	}
	
	inline void writeToGPUStart() {
#if _PROFILE > 2
		profileWriteToGPU.start();
#endif
	}
	
	inline void writeToGPUStop() {
#if _PROFILE > 2
		profileWriteToGPU.stop();
#endif
	}
	
	inline void waitStart() {
#if _PROFILE > 2
		profileWait.start();
#endif
	}
	
	inline void waitStop() {
#if _PROFILE > 2
		profileWait.stop();
#endif
	}

	inline void rgbaStart() {
#if _PROFILE > 3
		profileRgba.start();
#endif
	}
	
	inline void rgbaStop() {
#if _PROFILE > 3
		profileRgba.stop();
#endif
	}
	inline void lumaStart() {
#if _PROFILE > 3
		profileLuma.start();
#endif
	}
	
	inline void lumaStop() {
#if _PROFILE > 3
		profileLuma.stop();
#endif
	}
	
	inline void bilateralStart() {
#if _PROFILE > 3
		profileBilateral.start();
#endif
	}
	
	inline void bilateralStop() {
#if _PROFILE > 3
		profileBilateral.stop();
#endif
	}
	
	inline void copyHmeanStart() {
#if _PROFILE > 3
		profileCopyHmean.start();
#endif
	}
	
	inline void copyHmeanStop() {
#if _PROFILE > 3
		profileCopyHmean.stop();
#endif
	}
	
	inline void hmeanStart() {
#if _PROFILE > 3
		profileHmean.start();
#endif
	}
	
	inline void hmeanStop() {
#if _PROFILE > 3
		profileHmean.stop();
#endif
	}
	
	inline void tonemapStart() {
#if _PROFILE > 3
		profileTonemap.start();
#endif
	}
	
	inline void tonemapStop() {
#if _PROFILE > 3
		profileTonemap.stop();
#endif
	}
	
	inline void readFromGPUStart() {
#if _PROFILE > 2
		profileReadFromGPU.start();
#endif		
	}
	
	inline void readFromGPUStop() {
#if _PROFILE > 2
		profileReadFromGPU.stop();
#endif		
	}
	
	inline void chromaStart() {
#if _PROFILE > 3
		profileChroma.start();
#endif		
	}
	
	inline void chromaStop() {
#if _PROFILE > 3
		profileChroma.stop();
#endif		
	}
	
	inline void writeStart() {
#if _PROFILE > 2
		profileWriteToPipe.start();
#endif		
	}
	
	inline void writeStop() {
#if _PROFILE > 2
		profileWriteToPipe.stop();
#endif		
	}
};

#endif
