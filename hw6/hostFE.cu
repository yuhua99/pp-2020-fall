#include <stdio.h>
#include <stdlib.h>
#include "hostFE.h"
// for cuda
#include <cuda.h>

#define BlockSize 8

__constant__ float flt[1024];

__global__ void conv(float *in_img, float *out_img,
                     int img_w, int img_h, int flt_s, int pitch)
{
    int x = threadIdx.x + blockIdx.x * BlockSize;
    int y = threadIdx.y + blockIdx.y * BlockSize;
    if (x >= img_w || y >= img_h)
        return;

    int shift = flt_s / 2;
    float sum = 0;
    for (int i = 0; i < flt_s; i++)
    {
        for (int j = 0; j < flt_s; j++)
        {
            int nx = x - shift + j;
            int ny = y - shift + i;
            if ((nx < 0) || (nx >= img_w) || (ny < 0) || (ny >= img_h))
            {
                continue;
            }
            sum += flt[i * flt_s + j] * in_img[ny * pitch + nx];
        }
    }
    out_img[x + y * img_w] = sum;
}

// Host front-end function that allocates the memory and launches the GPU kernel
void hostFE(int filterWidth, float *filter, int imageHeight, int imageWidth,
            float *inputImage, float *outputImage)
{
    // allocate host mem
    float *in_img;
    size_t pitch;
    cudaMallocPitch((void **)&in_img, &pitch, imageWidth * sizeof(float), imageHeight);

    cudaHostRegister((void *)outputImage, imageWidth * imageHeight * sizeof(float),
                     cudaHostRegisterMapped);

    float *out_img;
    cudaHostGetDevicePointer(&out_img, outputImage, 0);
    // copy mem to device
    cudaMemcpy2D(in_img, pitch, inputImage, imageWidth * sizeof(float),
                 imageWidth * sizeof(float), imageHeight, cudaMemcpyHostToDevice);
    cudaMemcpyToSymbol(flt, filter, filterWidth * filterWidth * sizeof(float), 0,
                       cudaMemcpyHostToDevice);

    dim3 dimBlock(BlockSize, BlockSize);
    dim3 dimGrid((imageWidth / BlockSize) + (imageWidth % BlockSize == 0 ? 0 : 1),
                 (imageHeight / BlockSize) + (imageHeight % BlockSize == 0 ? 0 : 1));

    conv<<<dimGrid, dimBlock>>>(in_img, out_img, imageWidth, imageHeight, filterWidth, pitch / sizeof(float));

    cudaDeviceSynchronize();
    cudaHostUnregister((void *)outputImage);
    cudaFree(in_img);
    return;
}