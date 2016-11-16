#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#ifdef __OPEN_CL__
#include <CL/cl.h>

typedef uint8_t cmpx_t[2];

typedef struct {
    cl_device_id device;
    cl_platform_id platform;
} device_platform_t;

static cl_program build_program(cl_context context,
                                cl_device_id device,
                                const char *path);

#endif

static double now_ms(void)
{
    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return 1000.0*res.tv_sec + (double)res.tv_nsec/1e6;
}

#ifdef __cplusplus
extern "C" {
#endif

    void fft(const uint8_t *buf, size_t size, uint8_t *out);

    void four1(uint8_t *buf, size_t size);

    uint8_t *allocate(size_t bufsize);

#ifdef __cplusplus
}
#endif
