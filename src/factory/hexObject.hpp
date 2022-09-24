//
//  hexObject.hpp
//  factory
//
//  Created by Josh McNamee on 12/07/2015.
//
//

#ifndef factory_hexObject_hpp
#define factory_hexObject_hpp

#include <azure/GLObject.hpp>

class hexObject : public azure::GLObject {
public:
    hexObject(std::shared_ptr<azure::Program> program) : GLObject(program) {
        _layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_FALSE));
        _layout.push_back(azure::GLObject::Layout("norm", 3, GL_FLOAT, GL_FALSE));
        _layout.push_back(azure::GLObject::Layout("tex", 2, GL_FLOAT, GL_FALSE));
        
        _count = 0;
        
        for (int i = -5; i < 5; ++i) {
            for (int j = -5; j < 5; ++j) {
                int p = j*2;
                int q = i % 2;
                if (q) {
                    p++;
                }
                
                addHex(i, p);
            }
        }
        
        buffer();
        attach();
    }
    
    ~hexObject() {
        
    }
    
private:
    
    void addHex(int tick_w = 0, int tick_h = 0) {
        _data.reserve(_data.size() + 8*4);
        
        const float root3_2 = 0.86602540378;
        
        const float width_add = 1.5f*(float)tick_w;
        const float height_add = (float)tick_h*root3_2;
        
        GLfloat temp[] = {
            -0.5f + width_add,          -root3_2 + height_add,	0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f,
            -1.0f + width_add,           0.0f + height_add,      0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f,
            -0.5f + width_add,            root3_2 + height_add,  0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 1.0f,
            
            -0.5f + width_add,          -root3_2 + height_add,   0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 1.0f,
            -0.5f + width_add,           root3_2 + height_add,	0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f,
            0.5f + width_add,            root3_2 + height_add,	0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f,
            
            0.5f + width_add,            root3_2 + height_add,   0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 1.0f,
            0.5f + width_add,           -root3_2 + height_add,	0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f,
            -0.5f + width_add,          -root3_2 + height_add,	0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f,
            
            0.5f + width_add,            -root3_2 + height_add, 0.0f,      0.0f, 0.0f, 1.0f,     1.0f, 0.0f,
            0.5f + width_add,             root3_2 + height_add,  0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 1.0f,
            1.0f + width_add,             0.0f + height_add,     0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f
        };
        
        _data.insert(_data.end(), temp, temp+96);
        
        _count = _count + 8*4;
    }
    
};

#endif
