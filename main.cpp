#include "consts.h"

#include "fft.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#include <pthread.h>	/* POSIX Threads */

#ifdef __VISUAL__
#include "visual.h"
#endif /*__VISUAL*/

static char *_argv;
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

volatile int _num_bytes = BUFSIZE * HEIGHT *sizeof(char);

void read_samples(void *data);

int main(int argc, char*argv[]) {
	_argv = argv[0];

	/* The sample type to use */
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_U8,
		.rate = 44100,
		.channels = 1
	};

	pa_simple *s = NULL;
	int ret = 1;
	int error;

	uint8_t *fft_buf = allocate(BUFSIZE);

#ifdef __VISUAL__
	wflgInit();
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

#ifdef __VISUAL__
	while ( wflgRead() ) {
#else
	while (1) {
#endif /*__VISUAL__*/

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
		//four1(fft_buf, buf, BUFSIZE);
#ifdef __VISUAL__
		setPixels(buf);
		//int half = BUFSIZE / 2;
		//memset((fft_buf + half) , 0, half);
		//setPixels(fft_buf);
		draw();

		wflgPoll();
#endif /*__VISUAL__*/

#ifdef __PROFILE
		gettimeofday(&t1, 0);

		long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
		//printf("elapsed: %lu ms\n", elapsed);

		c_sec += elapsed; ++cnt;

		if(c_sec > 10e6) {
			printf("avg time spent per second per loop: %lu ms\n", c_sec / ((long)10e3 * cnt));// c_sec / (float)cnt);
			c_sec ^= ~c_sec;
			cnt ^= ~cnt;
		}
		//printf("avg time spent per second per loop: %lu ms\n", elapsed / 1000l);// c_sec / (float)cnt);
#endif /*__PROFILE*/
		//pa_simple_flush(s, NULL);
	}
	ret = 0;
  finish:

#ifdef __VISUAL__
	releaseVisual();
#endif /*__VISUAL__*/

	free(fft_buf);
	if (s)
		pa_simple_free(s);
	return ret;
}

void read_samples(void *data) {

	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_U8,
		.rate = 11050,
		.channels = 1
	};

	int error;
	pa_simple *s = NULL;

	if (!(s = pa_simple_new(NULL, _argv, PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		goto finish;
	}

	while(true) {

		pthread_mutex_lock(&_mutex);

		struct timeval t0/*, t1*/;
		gettimeofday(&t0, 0);

		/* Record some data ... */
		if (pa_simple_read(s, data, sizeof((uint8_t*)data), &error) < 0) {
			fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
			goto finish;
		}

		pthread_mutex_unlock(&_mutex);

		/*
		gettimeofday(&t1, 0);
		long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
		printf("2nd thread: reading in %d ms\n", elapsed);*/

	}

  finish:
	if (s)
		pa_simple_free(s);
}
