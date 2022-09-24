//
//  indexedGLObject.cpp
//  video-mush
//
//  Created by Josh McNamee on 08/12/2016.
//
//
#include <azure/glexception.hpp>
#include <azure/program.hpp>
#include "indexGLObject.hpp"

namespace azure {
    indexGLObject::indexGLObject(std::shared_ptr<azure::Program> program) : azure::GLObject(program) {
        index_alloc();
        //index_buffer();
        //index_attach();
    }
    
    indexGLObject::~indexGLObject() {
        
    }
    
    void indexGLObject::index_alloc() {
        glGenBuffers(1, &indices_vbo);
        _glException();
        
    }
    
    void indexGLObject::index_buffer() {
        glBindVertexArray(_vao);
        _glException();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);
        _glException();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        _glException();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        _glException();
        glBindVertexArray(0);
        _glException();
    }
    
    void indexGLObject::index_buffer(const std::vector<unsigned int>& indices_alt) {
        glBindVertexArray(_vao);
        _glException();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);
        _glException();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_alt.size() * sizeof(unsigned int), &indices_alt[0], GL_STATIC_DRAW);
        _glException();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        _glException();
        glBindVertexArray(0);
        _glException();
    }
    
    void indexGLObject::index_attach() {
        _program->use();
        glBindVertexArray(_vao);
        _glException();
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        _glException();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo);
        _glException();

        attach();
    }
    
    void indexGLObject::render() {
        render(0, indices.size());
    }
    
    void indexGLObject::render(const size_t offset, const size_t indices_count) {
        _program->use();
        glBindVertexArray(_vao);
        _glException();
        glDrawElements(_mode, indices_count, GL_UNSIGNED_INT, (GLuint *)(offset * sizeof(GLuint)));
        _glException();
        glBindVertexArray(0);
        _glException();
        _program->discard();
    }
    
    
}
