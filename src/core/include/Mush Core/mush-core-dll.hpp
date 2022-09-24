#ifndef MUSH_CORE_DLL_HPP
#define MUSH_CORE_DLL_HPP

#ifdef _WIN32
#ifdef MUSH_CORE_EXPORTS
#define MUSHEXPORTS_API __declspec(dllexport) 
#else
#define MUSHEXPORTS_API __declspec(dllimport) 
#endif
#else 
#define MUSHEXPORTS_API 
#endif

#endif