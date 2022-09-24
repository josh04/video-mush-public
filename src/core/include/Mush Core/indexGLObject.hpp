//
//  indexedGLObject.hpp
//  video-mush
//
//  Created by Josh McNamee on 08/12/2016.
//
//

#ifndef indexedGLObject_hpp
#define indexedGLObject_hpp

#include <azure/globject.hpp>
#include "mush-core-dll.hpp"

namespace azure {
    class Program;
    
    class MUSHEXPORTS_API indexGLObject : public azure::GLObject {
    public:
        indexGLObject(std::shared_ptr<azure::Program> program);
        ~indexGLObject();
    
        void index_alloc();
        void index_buffer();
        void index_buffer(const std::vector<unsigned int>& indices_alt);
        void index_attach();
        
        void render() override;
        void render(const size_t offset, const size_t indices_count);
        
    protected:
        GLuint indices_vbo;
        std::vector<unsigned int> indices;
    private:
    };
    
}

#endif /* indexedGLObject_hpp */
