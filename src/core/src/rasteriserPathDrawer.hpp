//
//  rasteriserPathDrawer.hpp
//  mush-core
//
//  Created by Josh McNamee on 15/04/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#ifndef rasteriserPathDrawer_hpp
#define rasteriserPathDrawer_hpp

#include <azure/globject.hpp>
#include <array>
#include <string>

#include "camera_path_io.hpp"

namespace mush {
    namespace raster {
        class path_drawer : public azure::GLObject {
        public:
            path_drawer();
            ~path_drawer();
            
            void init(const std::vector<camera::camera_path_node>& path);
            void render() override;
            private:
            
        };
    }
}

#endif /* rasteriserPathDrawer_hpp */
