//
//  sphereGL.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/12/2016.
//
//

#ifndef sphereGL2_hpp
#define sphereGL2_hpp

#include <azure/glm/glm.hpp>
#include <azure/GLObject.hpp>
#include <azure/program.hpp>
#include "indexGLObject.hpp"
#include "mush-core-dll.hpp"

namespace azure {
    class Texture;
}

namespace mush {
    namespace raster {
        class MUSHEXPORTS_API sphere : public azure::indexGLObject {
        public:
            sphere(glm::vec3 location, float radius, unsigned int tex_width, unsigned int tex_height);
            ~sphere();
            
            void render() override;
            
            std::shared_ptr<azure::Texture> get_texture() const {
                return _texture;
            }
            
            unsigned int get_width() const {
                return _width;
            }
            
            unsigned int get_height() const {
                return _height;
            }
            
        private:
            std::shared_ptr<azure::Texture> _texture = nullptr;
            unsigned int _width = 0, _height = 0;
        };
    }
}
#endif /* sphereGL_hpp */
