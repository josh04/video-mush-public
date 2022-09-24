
#include <string>
#include "mush-core-dll.hpp"
//extern "C" {
	namespace mush {
		MUSHEXPORTS_API std::string checkExists(std::string final, std::string original = "", unsigned int count = 1);
	}
//}
