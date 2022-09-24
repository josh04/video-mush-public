
#include <boost/filesystem.hpp>
#include <sstream>
#include "checkExists.hpp"


namespace mush {
	std::string checkExists(std::string final, std::string original, unsigned int count) {
		if (original == "") {
			original = final;
		}
		boost::filesystem::path pathT(final);
		if (boost::filesystem::exists(pathT)) {
			boost::filesystem::path pathTo(original);
			boost::filesystem::path extn = pathTo.extension();
			pathTo.replace_extension();
			std::stringstream strm;
			strm << " " << count;
			pathTo += strm.str();
			pathTo += extn;
			//pathTo.replace_extension(extn);
			return checkExists(pathTo.string(), original, ++count);
		} else {
			return final;
		}
	}

}