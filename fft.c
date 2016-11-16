#include "fft.h"

#include <math.h>
#include <stdio.h>

#ifdef __OPEN_CL__
static cl_mem _out_buf;
static cl_kernel _fft_kernel;

static cl_device_id _device;
static cl_context _context;
static cl_program _program;
static cl_command_queue _queue;


#define DEVICE_TYPE CL_DEVICE_TYPE_CPU
#define PROGRAM_SOURCE "fft_cl.cl"

void CL_CALLBACK err_callback(const char *errinfo, const void *prvt_info, size_t cb, void *user_data) {
    printf("Error in creating context: %s\n", errinfo);
}

cl_program build_program(cl_context context,
                         cl_device_id device,
                         const char *path) {

    FILE* program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;
    cl_int err;

    program_handle = fopen(path, "r");
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);


    cl_program program;
    program = clCreateProgramWithSource(context,
                                        1,
                                        (const char **)&program_buffer,
                                        &program_size,
                                        &err);
    if(err < 0) {
        fprintf(stderr, "error creating program from source: %d", err);
    }

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if(err < 0) {
        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char*) malloc(log_size + 1);
        program_log[log_size] = '\0';

        printf("log_size %lu\n", log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
    }

    free(program_buffer);
    return program;
}

#endif /* __OPEN_CL__ */

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#define SHIFT_LEFT(_index_, _shift_pos_, _mask_left_)	  \
    br[0] = (_index_ << _shift_pos_) & _mask_left_; \
    br[1] = (((_index_)+1) << _shift_pos_) & _mask_left_; \
    br[2] = (((_index_)+2) << _shift_pos_) & _mask_left_; \
    br[3] = (((_index_)+3) << _shift_pos_) & _mask_left_;


#define SHIFT_RIGHT(_index_, _shift_pos_, _mask_right_)	  \
    br[0] = (_index_ >> _shift_pos_) & _mask_right_; \
    br[1] = (((_index_)+1) >> _shift_pos_) & _mask_right_; \
    br[2] = (((_index_)+2) >> _shift_pos_) & _mask_right_; \
    br[3] = (((_index_)+3) >> _shift_pos_) & _mask_right_;


inline void swap(uint8_t *nr, uint8_t *ni) {
    uint8_t tmp = *nr;
    *nr = *ni;
    *ni = tmp;
}

#define MOD(n, N) ((n<0)? N+n : n)

void conv(const uint8_t *func, size_t func_size,
          const uint8_t *model, size_t model_size,
          uint8_t *out) {

    for(size_t n=0; n < func_size + model_size - 1; n++)
    {
        out[n] = 0;
        for(size_t m=0; m < model_size; m++)
        {
            out[n] = func[m] * model[MOD(n-m, model_size)];
        }
    }
}

void four1(uint8_t *data, size_t size) {
    unsigned long n, mmax, m, j, istep, i;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;


    double t1 = now_ms();

    // reverse-binary reindexing
    n = size<<1;
    j=1;
    for (i=1; i<n; i+=2) {
        if (j>i) {
            swap(&data[j-1], &data[i-1]);
            swap(&data[j], &data[i]);
        }
        m = size;
        while (m>=2 && j>m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    };

    // here begins the Danielson-Lanczos section
    mmax=2;
    while (n>mmax) {
        istep = mmax<<1;
        theta = -(2*M_PI/mmax);
        wtemp = sin(0.5*theta);
        wpr = -2.0*wtemp*wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m=1; m < mmax; m += 2) {
            for (i=m; i <= n; i += istep) {
                j=i+mmax;
                tempr = wr*data[j-1] - wi*data[j];
                tempi = wr * data[j] + wi*data[j-1];

                data[j-1] = data[i-1] - tempr;
                data[j] = data[i] - tempi;
                data[i-1] += tempr;
                data[i] += tempi;
            }
            wtemp=wr;
            wr += wr*wpr - wi*wpi;
            wi += wi*wpr + wtemp*wpi;
        }
        mmax=istep;
    }


    printf("%.2fms\n", now_ms() - t1);
}

void fft(const uint8_t *buf, size_t size, uint8_t *out) {

    const uint8_t *tmp = buf;

    unsigned int index, mask_left, mask_right, shift_pos;
    //uint8_t x1, x2, x3, x4; //, sum12, diff12, sum34, diff34;

    //bit reversal
    unsigned int br[4];

    double t_now = now_ms();

    index = 0;

    //init
    while(index <= size) {

        mask_left = size/2;
        mask_right = 1;
        //size is of power 2 => positive sign
        shift_pos = (unsigned int)log2(size) -1;

        SHIFT_LEFT(index, shift_pos, mask_left);

        SHIFT_RIGHT(index, shift_pos, mask_right);

        while(shift_pos > 1) {
            shift_pos -= 2;
            mask_left >>= 1;
            mask_right <<= 1;
        }

        //x1 = buf;


        index += 4;
        ++buf;
    }

    printf("%2.4fms\n", now_ms() - t_now);

    buf = tmp;
}


uint8_t *allocate(size_t bufsize) {

    uint8_t *result = malloc(sizeof(uint8_t)*bufsize);

#ifdef __OPEN_CL__
    cl_int err;
    cl_uint num_platforms;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Cannot get the number of OpenCL platforms available.\n");
        exit(EXIT_FAILURE);
    }

    cl_platform_id platforms[num_platforms];
    err = clGetPlatformIDs(num_platforms, platforms, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Cannot get	 platform ids.\n");
        exit(EXIT_FAILURE);
    }

    cl_platform_id pt = platforms[1];
    err = clGetDeviceIDs(pt, DEVICE_TYPE, 1, &_device, NULL);
    if(err != CL_SUCCESS) {
        fprintf(stderr, "Cannot find any CPU device.\n");
        goto finish;
    }

    /* INFO */
    char devname[256];
    err = clGetDeviceInfo(_device, CL_DEVICE_NAME, 256, devname, 0);
    if(err < 0) {
        fprintf(stderr, "Error getting device info (NAME).\n");
        goto finish;
    }
    printf("CL_DEVICE = %s\n", devname);

    _context = clCreateContext(NULL,
                               1,
                               &_device,
                               err_callback,
                               NULL,
                               &err);
    if( err < 0 ) {
        fprintf(stderr, "Could not create context\n");
        goto finish;
    }

    _program = build_program(_context, _device, PROGRAM_SOURCE);

    _out_buf = clCreateBuffer(_context,
                              CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                              bufsize,
                              result,
                              &err);
    if( err < 0 ) {
        fprintf(stderr, "Could not create buffer of the tmp_buf\n");
        goto finish;
    }

    _fft_kernel = clCreateKernel(_program, "fft", &err);
    if( err < 0 ) {
        fprintf(stderr, "Could not create kernel dim1\n");
    }

    clSetKernelArg(_fft_kernel, 0, sizeof(cl_mem), &_out_buf);


    _queue = clCreateCommandQueue(_context, _device, 0, &err);

#endif /* __OPEN_CL__ */

#pragma GCC diagnostic ignored "-Wunused-label"
  finish:
    return result;
}
