#ifndef SDLBITS_MUSH_H
#define SDLBITS_MUSH_H

#if defined(_WIN32)
#	include <SDL2/SDL.h>
#elif defined(__APPLE__)
#	include <SDL2/SDL.h>
#elif defined(__linux__)
#	include <SDL2/SDL.h>
#else
#	error 'unsupported platform'
#endif

#endif

