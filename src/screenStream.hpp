#ifdef SCREENSTREAM_HPP // EDITED TO BLANK THIS
#define SCREENSTREAM_HPP

extern "C" void putLog(std::string s);

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp> // SFML MUST come first!
#include <SFML/Window/WindowStyle.hpp>

#include <memory>
#include <vector>
#include <fstream>
#include "ldrImageBuffer.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <GL/glew.h>
#endif
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/GL3.h>
#include <OpenGL/CGLCurrent.h>
#endif

using std::shared_ptr;

class screenStream {
public:
	screenStream(shared_ptr<ldrImageBuffer> outBuffer, unsigned int width, unsigned int height) : outBuffer(outBuffer), width(width), height(height) {
		GLfloat temp[] = {

//main

		-1.0,	-1.0f,	0.0f,	0.0f, 0.5f,
		-1.0,	1.0f,	0.0f,	0.0f, 0.0f,	
		0.0f,	1.0f,	0.0f,	1.0f, 0.0f,

		0.0f,	1.0f,	0.0f,	1.0f, 0.0f,
		0.0f,	-1.0f,	0.0f,	1.0f, 0.5f,
		-1.0,	-1.0f,	0.0f,	0.0f, 0.5f, 


		0.0,	-1.0f,	0.0f,	0.0f, 1.0f,
		0.0,	1.0f,	0.0f,	0.0f, 0.5f,	
		1.0f,	1.0f,	0.0f,	1.0f, 0.5f,

		1.0f,	1.0f,	0.0f,	1.0f, 0.5f,
		1.0f,	-1.0f,	0.0f,	1.0f, 1.0f,
		0.0,	-1.0f,	0.0f,	0.0f, 1.0f, 
		
		};

		vertexData.assign(temp, temp+60);
	}

	~screenStream() {}

	void go() {
#ifdef _WIN32
		wglMakeCurrent(glHDC2, glCtx);
#endif
#ifdef __APPLE__
		CGLSetCurrentContext(cgl_context);
#endif
        GLenum err = glGetError();
		window->setActive(true);
		while(frame = (unsigned char *)outBuffer->outLock()) {
			
			glUseProgram(maskProgramID);
			glActiveTexture(GL_TEXTURE0);
            err = glGetError();
			glBindTexture(GL_TEXTURE_2D, hdrTexID);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGB8,
				outputWidth,
				outputHeight,
				0, GL_BGR, 
				GL_UNSIGNED_BYTE, frame);
			glDrawArrays(GL_TRIANGLES, 0, 12);
			glFinish();
			window->display();
			glFinish();
			outBuffer->outUnlock();
		}
	}

	void init(const char * resourceDir, unsigned int owidth, unsigned int oheight) {
        outputWidth = owidth;
        outputHeight = oheight;
        this->resourceDir = resourceDir;
		sfmlInit(width, height);
#ifdef _WIN32
		glewInit();
#endif
		glInit();
		window->setActive(false);

#if defined(_WIN32)
		glCtx = wglGetCurrentContext();
		glHDC2 = wglGetCurrentDC();
#elif defined(__linux__)
			GLXContext glCtx = glXGetCurrentContext();
#elif defined(__APPLE__)
			cgl_context = CGLGetCurrentContext();
		CGLShareGroupObj sharegroup = CGLGetShareGroup(cgl_context);
		gcl_gl_set_sharegroup(sharegroup);

		CGLError err;
		err = CGLEnable(cgl_context, kCGLCEMPEngine);

		if (err != kCGLNoError) {
			abort();
		}
#endif

	}


	void sfmlInit(unsigned int width, unsigned int height) {
		sf::ContextSettings requestedContext;
		requestedContext.majorVersion = 3;
		requestedContext.minorVersion = 2;
		window = new sf::RenderWindow(sf::VideoMode(width, height), "argle", 0L, requestedContext);
		window->setPosition(sf::Vector2i(1680,0));
		#ifdef DEBUG
			sf::ContextSettings receivedContext = window->getSettings();
			std::stringstream strm;
			strm << "OpenGL " << receivedContext.majorVersion << "." << receivedContext.minorVersion;
			putLog(strm.str().c_str());
		#endif
	}


	void show_info_log(GLuint object, PFNGLGETSHADERIVPROC glGet__iv, PFNGLGETSHADERINFOLOGPROC glGet__InfoLog) {
		GLint log_length;
		char *log;

		glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
		log = (char *)malloc(log_length);
		glGet__InfoLog(object, log_length, NULL, log);
		putLog(std::string(log));
		free(log);
	}
	
	void glInit() {
		glClearColor(0.0, 1.0, 1.0, 1.0);

		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE10);
		glGenTextures(1, &hdrTexID);
		glBindTexture(GL_TEXTURE_2D, hdrTexID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, NULL);
		GLint status;

		std::ifstream vertFile(std::string(resourceDir) + "shaders/shader.vert"); std::string vertSource(std::istreambuf_iterator<char>(vertFile), (std::istreambuf_iterator<char>()));
		const GLchar * vertStr = (GLchar *)vertSource.c_str();
		const GLint vertLen = vertSource.size();
		GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertShader, 1, &vertStr, &vertLen);
		glCompileShader(vertShader);
		glGetShaderiv(vertShader, GL_COMPILE_STATUS, &status);
		if (!status) {
			putLog("Failed to compile Vertex Shader");
			show_info_log(vertShader, glGetShaderiv, glGetShaderInfoLog);
			return;
		}

		std::ifstream maskFile(std::string(resourceDir) + "shaders/mask.frag");
		std::string maskSource(std::istreambuf_iterator<char>(maskFile), (std::istreambuf_iterator<char>()));
		const GLchar * maskStr = (GLchar *)maskSource.c_str();
		const GLint maskLen = maskSource.size();
		GLuint maskShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(maskShader, 1, &maskStr, &maskLen);
		glCompileShader(maskShader);
		glGetShaderiv(maskShader, GL_COMPILE_STATUS, &status);
		if (!status) {
			putLog("Failed to compile Mask Shader");
			show_info_log(maskShader, glGetShaderiv, glGetShaderInfoLog);
			return;
		}

		maskProgramID = glCreateProgram();
		glAttachShader(maskProgramID, vertShader);
		glAttachShader(maskProgramID, maskShader);

		glLinkProgram(maskProgramID);
		glGetProgramiv(maskProgramID, GL_LINK_STATUS, &status);

		uMaskTex = glGetUniformLocation(maskProgramID, "tex");
		aMaskVert = glGetAttribLocation(maskProgramID, "vert");
		aMaskVertTexCoord = glGetAttribLocation(maskProgramID, "vertTexCoord");

		glUseProgram(maskProgramID);

		glGenVertexArrays(1, &gVAO);
		glGenBuffers(1, &gVBO);

		glClear(GL_COLOR_BUFFER_BIT);

		window->display();
		
		glBindVertexArray(gVAO);
		glBindBuffer(GL_ARRAY_BUFFER, gVBO);

		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertexData.size(), &vertexData[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(aMaskVert);
		glVertexAttribPointer(aMaskVert, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, NULL);

		glEnableVertexAttribArray(aMaskVertTexCoord);
		glVertexAttribPointer(aMaskVertTexCoord, 2, GL_FLOAT, GL_TRUE,  sizeof(GLfloat) * 5, (const GLvoid *)(sizeof(GLfloat) * 3));

		glClear(GL_COLOR_BUFFER_BIT);
	
		glDrawArrays(GL_TRIANGLES, 0, 6);

		window->display();
	}

private:
	unsigned char * frame;
	
	GLuint gVAO;
	GLuint gVBO;

	std::vector<GLfloat> vertexData;

	GLuint hdrTexID;

	GLuint maskProgramID;

	GLint uMaskTex;
	GLint aMaskVert;
	GLint aMaskVertTexCoord;

#ifdef _WIN32
	HGLRC glCtx;
	HDC glHDC2;
#endif
#ifdef __APPLE__
	CGLContextObj cgl_context;
#endif

	sf::RenderWindow * window;
	shared_ptr<ldrImageBuffer> outBuffer;
	unsigned int width, height;
    
    const char * resourceDir;
    unsigned int outputWidth = 1280, outputHeight = 1440;
};

#endif
