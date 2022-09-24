//
//  mushWindowInterface.hpp
//  video-mush
//
//  Created by Josh McNamee on 24/02/2015.
//
//

#ifndef video_mush_mushWindowInterface_hpp
#define video_mush_mushWindowInterface_hpp

#include <azure/WindowInterface.hpp>
#include <scarlet/PlatformInterface.hpp>

//#include "ConfigStruct.hpp"
//#include "sfmlInterface.hpp"
#include "sdlInterface.hpp"
//#include <SFML/Graphics.hpp>

#include "mush-core-dll.hpp"

namespace mush {
	namespace gui {
		class MUSHEXPORTS_API window_interface : public SDLInterface {
		public:
			window_interface(const char * resourcesPath) : SDLInterface(), resourcesPath(resourcesPath) {

			}

			~window_interface() {

			}

			virtual boost::filesystem::path getRocketAssetsFolder() const {
				return getResourcesFolder() / boost::filesystem::path("assets");
			}

			virtual boost::filesystem::path getResourcesFolder() const {
				return boost::filesystem::path(resourcesPath);
			}

			virtual float getPixelsPerInch(azure::WindowHandle window, azure::ContextHandle context) const override {
#ifdef _WIN32
				return 72.0f;
#else
				return 96.0f;
#endif
			}

		private:
			const char * resourcesPath = nullptr;
		};
	}
}

#endif
