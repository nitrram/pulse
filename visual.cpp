#include "visual.h"
#include "shader.h"
#include "consts.h"

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define VIDEO_SURFACE_NUM_PBOS 4

#define WIDTH 1024
#define WINDOW_HEIGHT 768

#define PULSE_VISUAL_WIDTH 1024
#define PULSE_VISUAL_HEIGHT 768
#define PULSE_MULTIPLICATOR 1 //768

#ifdef __OPEN_CL__
static cl_mem _pixel_buf;
#else
static GLubyte *_pixel_buf = 0;
#endif /*__OPEN_CL__*/


static GLuint _pbos[VIDEO_SURFACE_NUM_PBOS];
static unsigned int _read_dx;
static unsigned int _write_dx;
static GLuint _texId;
static GLuint _tex;
static GLuint _vao;
static GLuint _vbo; //[2];
static GLuint _prog;


extern volatile unsigned  int _num_bytes;

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

static GLFWwindow* _window;

void key_callback(
	GLFWwindow *window,
	int key,
	int scancode,
	int action,
	int mods
) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(_window, GLFW_TRUE);
}

void setup_gl(char *zero_argv) {

	// relativize the path
	char * argv = strrchr(zero_argv, (int)'/');
	*(argv+1) = '\0';

	char vert_shader[1024];
	char frag_shader[1024];

	memset(vert_shader, 0 ,1024);
	memset(frag_shader, 0, 1024);

	strcpy(vert_shader, zero_argv);
	strcat(vert_shader, "rectangle.vert");

	strcpy(frag_shader, zero_argv);
	strcat(frag_shader, "texture.frag");

	_prog = LoadShaders( vert_shader, frag_shader );

	_texId	= glGetUniformLocation(_prog, "myTextureSampler");
	glUniform1i(_texId, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glUseProgram(0);


	glGenBuffers(VIDEO_SURFACE_NUM_PBOS, _pbos);
	for(int i = 0; i < VIDEO_SURFACE_NUM_PBOS; ++i) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[i]);
		glBufferData((i % 2) ? GL_PIXEL_UNPACK_BUFFER : GL_PIXEL_PACK_BUFFER, _num_bytes, NULL, GL_STREAM_DRAW);

	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glGenTextures(1, &_tex);
	glBindTexture(GL_TEXTURE_2D, _tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BUFSIZE, HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	GLfloat vertices[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f
	};

	glGenBuffers(1, &_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER,
				 8*sizeof(GLfloat),
				 vertices,
				 GL_STATIC_DRAW);

	_pixel_buf = (GLubyte*)malloc(_num_bytes);
}

/*set pixels*/
void setPixels(unsigned char* pixels) {
	if(!pixels) {
		printf("WARNING: VideoSurface::setPixels(), given pixels is NULL.\n");
		return;
	}
	if(!_tex || PULSE_VISUAL_WIDTH == 0 || PULSE_VISUAL_HEIGHT == 0) {
		printf("WARNING: VideoSurface::setPixels(): cannot set, we're not initialized.\n");
		return;
	}

	_read_dx = (_read_dx + 1) % VIDEO_SURFACE_NUM_PBOS;
	_write_dx = (_read_dx + 1) % VIDEO_SURFACE_NUM_PBOS;

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[_read_dx]);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BUFSIZE, HEIGHT, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, BUFSIZE, HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[_write_dx]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, _num_bytes, NULL, GL_STREAM_DRAW);
	GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

	pthread_mutex_lock(&_mutex);

	if(ptr) {
		memcpy(ptr, pixels, _num_bytes);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}

	pthread_mutex_unlock(&_mutex);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void releaseVisual() {
	if(_pixel_buf) {
		free(_pixel_buf);
		_pixel_buf = 0;
	}

	if(_texId) {
		glDeleteTextures(1, &_texId);
		_texId = 0;
	}

	if(_tex) {
		glDeleteTextures(1, &_tex);
		_tex = 0;
	}

	if(_vbo) {
		glDeleteBuffers(1, &_vbo);
	}
}

void draw(/*int x, int	y*/) {

	glEnableVertexAttribArray(0); // pos
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glUseProgram(_prog);

	glBindTexture(GL_TEXTURE_2D, _tex);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(0);
}


bool wflgRead() {
	return ((glfwGetKey(_window, GLFW_KEY_ESCAPE ) != GLFW_PRESS) &&
			(glfwGetKey(_window, GLFW_KEY_Q) != GLFW_PRESS)) &&
		glfwWindowShouldClose(_window) == 0;
}

void wflgPoll() {
	glfwSwapBuffers(_window);
	glfwPollEvents();
}


void wflgInit() {
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	_window = glfwCreateWindow( WIDTH, WINDOW_HEIGHT, "pulse", NULL, NULL);
	if( _window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		glfwTerminate();
		return;
	}
	glfwMakeContextCurrent(_window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(_window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(_window, key_callback);
}
