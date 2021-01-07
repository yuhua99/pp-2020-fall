#include <stdio.h>
#include <stdlib.h>
#include "hostFE.h"
#include "helper.h"

void hostFE(int filterWidth, float *filter, int imageHeight, int imageWidth,
            float *inputImage, float *outputImage, cl_device_id *device,
            cl_context *context, cl_program *program)
{
    cl_int status;
    cl_uint plat_num;
    int filterSize = filterWidth * filterWidth;
    int imageSize = imageWidth * imageHeight;
    // create command queue
    cl_command_queue cmd_que = clCreateCommandQueue(*context, *device, 0, &status);

    cl_image_format flt_fmt;
    flt_fmt.image_channel_order = CL_R; //CL_R: only one channel, CL_A: rgba
    flt_fmt.image_channel_data_type = CL_FLOAT;
    // create buffer
    cl_mem in_img = clCreateBuffer(*context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   imageSize * sizeof(float), inputImage, &status);
    cl_mem out_img = clCreateBuffer(*context, CL_MEM_WRITE_ONLY,
                                    imageSize * sizeof(float), NULL, &status);

    cl_mem in_flt = clCreateImage2D(
        *context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
        &flt_fmt, filterWidth, filterWidth, 0, filter, &status);

    // call kernel
    cl_kernel conv = clCreateKernel(*program, "convolution", &status);
    // set arguments
    status = clSetKernelArg(conv, 0, sizeof(cl_mem), (void *)&in_img);
    status = clSetKernelArg(conv, 1, sizeof(cl_mem), (void *)&in_flt);
    status = clSetKernelArg(conv, 2, sizeof(cl_mem), (void *)&out_img);
    status = clSetKernelArg(conv, 3, sizeof(cl_int), (void *)&imageWidth);
    status = clSetKernelArg(conv, 4, sizeof(cl_int), (void *)&imageHeight);
    // set local and global workgroup size
    size_t localws[2] = {16, 16};
    size_t globalws[2] = {16 * (imageWidth / 16 + (imageWidth % 16 == 0 ? 0 : 1)), 16 * (imageHeight / 16 + (imageHeight % 16 == 0 ? 0 : 1))};

    // execute kernel
    // (que, kernel, work-dim, gws offset, gws,...)
    status = clEnqueueNDRangeKernel(cmd_que, conv, 2, NULL, globalws, localws, 0, NULL, NULL);
    CHECK(status, "enq");

    // copy result to host
    status = clEnqueueReadBuffer(cmd_que, out_img, CL_TRUE, 0,
                                 imageSize * sizeof(float), outputImage,
                                 NULL, NULL, NULL);

    // release mem
    clReleaseMemObject(in_img);
    clReleaseMemObject(in_flt);
    clReleaseMemObject(out_img);
    clReleaseKernel(conv);
    clReleaseCommandQueue(cmd_que);
}