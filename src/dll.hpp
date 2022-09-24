#ifndef MUSH_DLL_HPP
#define MUSH_DLL_HPP

#ifdef _WIN32
#ifdef VIDEO_MUSH_EXPORTS
#define VIDEOMUSH_EXPORTS __declspec(dllexport) 
#else
#define VIDEOMUSH_EXPORTS __declspec(dllimport) 
#endif
#else 
#define VIDEOMUSH_EXPORTS 
#endif

#endif