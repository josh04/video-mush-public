//
//  hexGrid.hpp
//  factory
//
//  Created by Josh McNamee on 12/07/2015.
//
//

#ifndef factory_hexGrid_hpp
#define factory_hexGrid_hpp

#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#include <azure/glm/gtc/matrix_transform.hpp>
#include <azure/glm/gtx/transform.hpp>
#include <azure/glm/gtx/rotate_vector.hpp>
#else
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#endif

#include "hexObject.hpp"

class hexGrid {
public:
    hexGrid(unsigned int width, unsigned int height) {
        _width = width;
        _height = height;
        camera_location = {0.0, 0.0, 0.0};
    }
    
    ~hexGrid() {}
    
    void init() {
        
        const char * vert = "shaders/hexVert.vert";
        const char * frag = "shaders/hexFrag.frag";
        
#ifdef __APPLE__
        NSString * resStr = [[[NSBundle mainBundle] resourcePath] stringByAppendingString:@"/"];
        NSString * NSvertStr = [resStr stringByAppendingString:[NSString stringWithUTF8String:vert]];
        NSString * NSfragStr = [resStr stringByAppendingString:[NSString stringWithUTF8String:frag]];
        vert = [NSvertStr UTF8String];
        frag = [NSfragStr UTF8String];
#endif
        
        _program = std::make_shared<azure::Program>(vert, frag);
        _program->link();
        _program->use();
    
        _hexObj = std::make_shared<hexObject>(_program);
        //_hexObj->init();
        
        setCamera({0.0,0.0,0.0}, 0.0, 0.0, 90.0);
    }
    
    void draw() {
        
        glClearColor(0.5, 1.0f, 0.5, 1.0f);
        _glException();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        _glException();
        
        
        glEnable(GL_CULL_FACE);
        _glException();
        glCullFace(GL_BACK);
        _glException();
        
        static float look = 0.0;
        look = look + 1.0f;
        setCamera({0.0,0.0,-2.0}, 0, look, 90.0);
        
        _hexObj->attach();
        
        _hexObj->render();
    }
    
    void setCamera(const glm::vec3& new_location, const double newTheta, const double newPhi, const double fov) {

        
        camera_location = new_location;
        
        theta = newTheta;
        phi = newPhi;
        
        glm::vec3 gaze(1, 0, 0);
        gaze = glm::rotateZ(gaze, (float)newPhi);
        gaze = glm::rotateY(gaze,  (float)newTheta);
        //gaze.RotateZ(newPhi*(M_PI/180.0));
        //gaze.RotateY(newTheta*(M_PI/180.0));
        //gaze.Normalize();
        glm::vec3 up(0, 1, 0);
        up = glm::rotateZ(up,  (float)newPhi);
        up = glm::rotateY(up,  (float)newTheta);
        
        
        const glm::vec3 w = -gaze;
        const glm::vec3 u = glm::cross(up, w);
        const glm::vec3 tup = glm::cross(w, u);
        
        const float aspect = (double)_width / (double)_height; // get aspect ratio
        glViewport(0, 0, _width, _height);
        _glException();
        
        
        glm::mat4 Projection = glm::perspective((float)fov, aspect, 0.1f, 10000.0f);
        // Camera matrix
        //glm::vec3 center = {0.0, 0.0, 0.0};
        //glm::vec3 tup = {0.0, 1.0, 0.0};
        //glm::vec3 from = {0.0, 0.0, -2.0};
        
        glm::mat4 View       = glm::lookAt(
                                           glm::vec3(camera_location[0], camera_location[1], camera_location[2]),
                                           glm::vec3(gaze[0], gaze[1], gaze[2]),
                                           glm::vec3(tup[0], tup[1], tup[2])
                                           );
        
        glm::mat4 Model      = glm::mat4(1.0f);
        
        //    glm::mat4 MV         = View * Model;
        glm::mat4 MVP        = Projection * View * Model;
        
        _program->use();
        _program->uniformV("M", false, &Model[0][0]);
        _program->uniformV("V", false, &View[0][0]);
        //    _program->uniformV("MV", false, &MV[0][0]);
        _program->uniformV("MVP", false, &MVP[0][0]);
        
        GLfloat position[] = { camera_location[0], camera_location[1], camera_location[2], 1.0f };
        
        GLint location;
        if ((location = _program->uniform("light_position")) < 0) {return;}
        glUniform3fv(location, 1, position);
        _glException();
        
    }
    
    
private:
    std::shared_ptr<hexObject> _hexObj = nullptr;
    std::shared_ptr<azure::Program> _program = nullptr;
    unsigned int _width = 1280, _height = 720;
    
    double theta = 0.0, phi = 0.0, fov = 90.0;
    glm::vec3 camera_location;
};

#endif
