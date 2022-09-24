#ifndef SCARLET_DEBUG_HPP
#define SCARLET_DEBUG_HPP

#include <cassert>

#include <iostream>
#include <sstream>

#define ASSERT(condition) assert(condition)

#ifndef NDEBUG
#define ASSERTMSG(condition, message) \
	do { \
		if (!(condition)) { \
			std::ostringstream ostr; ostr << message; \
			std::cerr << "Assertion failed: " << #condition << ", function " << __FUNCTION__ << ", file " << __FILE__ << ", line " << __LINE__ << ". " << std::endl << ostr.str() << std::endl; \
			std::abort(); \
		} \
	} while (false);
#else
#define ASSERTMSG(condition, message) do {} while (false);
#endif

#ifndef NDEBUG
#define dout std::cout << __FILE__ << ":" << __LINE__ << ": " // FIXME: Remove this and replace with real logging.
#else
#define dout if (0) std::cout
#endif

#endif

