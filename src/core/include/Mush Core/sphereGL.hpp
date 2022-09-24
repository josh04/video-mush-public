//
//  sphereGL.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/12/2016.
//
//

#ifndef sphereGL_hpp
#define sphereGL_hpp

#include <azure/glm/glm.hpp>
#include <azure/GLObject.hpp>
#include <azure/program.hpp>
#include "indexGLObject.hpp"
#include "mush-core-dll.hpp"


class MUSHEXPORTS_API sphereGL : public azure::indexGLObject {
public:
    sphereGL(std::shared_ptr<azure::Program> program, glm::vec3 location, float radius);
    ~sphereGL();
    
private:
    
};

#endif /* sphereGL_hpp */
