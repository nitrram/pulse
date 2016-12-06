/***
	This file is part of PulseAudio.
	PulseAudio is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation; either version 2.1 of the License,
	or (at your option) any later version.
	PulseAudio is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	General Public License for more details.
	You should have received a copy of the GNU Lesser General Public License
	along with PulseAudio; if not, see <http://www.gnu.org/licenses/>.
***/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#define BUFSIZE 512

//#define __PROFILE 0

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
#define WIDTH 1024
#define WINDOW_HEIGHT 768
#define HEIGHT 1 //768

static const int _num_bytes = BUFSIZE * HEIGHT *sizeof(char);
GLFWwindow* window;

#include "shader.h"

// use fft
// #include <stdio.h>
// #include <stdlib.h>
#include "fft.h"

#ifdef __VISUAL__

#ifdef __OPEN_CL__
static cl_mem _pixel_buf;
#else
static GLubyte *_pixel_buf = 0;
#endif /*__OPEN_CL__*/


#define VIDEO_SURFACE_NUM_PBOS 2

static GLuint pbos[VIDEO_SURFACE_NUM_PBOS];
static unsigned int read_dx;
static unsigned int write_dx;

static GLuint texId;
static GLuint tex;
static GLuint vao;
static GLuint vbo; //[2];

static GLuint prog;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
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

	prog = LoadShaders( vert_shader, frag_shader );

	texId  = glGetUniformLocation(prog, "myTextureSampler");
	glUniform1i(texId, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glUseProgram(0);


	glGenBuffers(VIDEO_SURFACE_NUM_PBOS, pbos);
	for(int i = 0; i < VIDEO_SURFACE_NUM_PBOS; ++i) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[i]);
		glBufferData((i % 2) ? GL_PIXEL_UNPACK_BUFFER : GL_PIXEL_PACK_BUFFER, _num_bytes, NULL, GL_STREAM_DRAW);

	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BUFSIZE, HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLfloat vertices[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f
	};

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
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
	if(!tex || WIDTH == 0 || HEIGHT == 0) {
		printf("WARNING: VideoSurface::setPixels(): cannot set, we're not initialized.\n");
		return;
	}

	read_dx = (read_dx + 1) % VIDEO_SURFACE_NUM_PBOS;
	write_dx = (read_dx + 1) % VIDEO_SURFACE_NUM_PBOS;

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[read_dx]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, BUFSIZE, HEIGHT, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, BUFSIZE, HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[write_dx]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, _num_bytes, NULL, GL_STREAM_DRAW);
	GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if(ptr) {
		memcpy(ptr, pixels, _num_bytes);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}


void draw(/*int x, int	y*/) {

	glEnableVertexAttribArray(0); // pos
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glUseProgram(prog);

	glBindTexture(GL_TEXTURE_2D, tex);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(0);
}

#endif /*__VISUAL*/

int main(int argc, char*argv[]) {
	/* The sample type to use */
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_U8,
		.rate = 24000,
		.channels = 1
	};
	pa_simple *s = NULL;
	int ret = 1;
	int error;

	uint8_t *fft_buf = allocate(BUFSIZE);

//	printf("%s\n", *argv);

#ifdef __VISUAL__

	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( WIDTH, WINDOW_HEIGHT, "pulse", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	setup_gl(*argv);

#endif /*__VISUAL__*/

#ifdef __PROFILE
	uint64_t c_sec = 0;
	uint32_t cnt = 0;
#endif /*__PROFILE*/
	/* Create the recording stream */
	if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		goto finish;
	}

	uint8_t buf[BUFSIZE];
	while (!glfwWindowShouldClose(window)) {

#ifdef __PROFILE
		struct timeval t0, t1;
		gettimeofday(&t0, 0);
#endif /*__PROFILE*/


		/* Record some data ... */
		if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
			fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
			goto finish;
		}

		//fft(buf, BUFSIZE, fft_buf);
		four1(fft_buf, buf, BUFSIZE);
#ifdef __VISUAL__

		setPixels(buf);
		//setPixels(fft_buf);
		draw();

		glfwSwapBuffers(window);
#endif /*__VISUAL__*/

#ifdef __PROFILE
		gettimeofday(&t1, 0);
		float check = t1.tv_usec - t0.tv_usec;

		c_sec += abs((t1.tv_usec - t0.tv_usec) / 1000 );
		++cnt;
		if(c_sec > 1000) {
			printf("avg time spent per second per loop: %ldms\n", c_sec / cnt);
			c_sec ^= ~c_sec;
			cnt ^= ~cnt;
		}
#endif /*__PROFILE*/
	}
	ret = 0;
  finish:

#ifdef __VISUAL__

	if(_pixel_buf) {
		free(_pixel_buf);
		_pixel_buf = 0;
	}

	if(texId) {
		glDeleteTextures(1, &texId);
		texId = 0;
	}

	if(tex) {
		glDeleteTextures(1, &tex);
		tex = 0;
	}

	if(vbo) {
		glDeleteBuffers(1, &vbo);
	}

#endif /*__VISUAL__*/


	free(fft_buf);
	if (s)
		pa_simple_free(s);
	return ret;
}
