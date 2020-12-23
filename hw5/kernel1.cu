#include <cuda.h>
#include <stdio.h>
#include <stdlib.h>

#define BlockSize 16

__global__ void mandelKernel(int *cudaMem, float lowerX, float lowerY,
                             float stepX, float stepY, int maxIteration, int widthX, int widthY)
{
    // To avoid error caused by the floating number, use the following pseudo code
    //
    // float x = lowerX + thisX * stepX;
    // float y = lowerY + thisY * stepY;
    int xid = threadIdx.x + blockIdx.x * BlockSize;
    int yid = threadIdx.y + blockIdx.y * BlockSize;
    if (xid >= widthX || yid >= widthY)
        return;

    float x = lowerX + xid * stepX;
    float y = lowerY + Yid * stepY;
    float z_re = x, z_im = y;

    int i;
    for (i = 0; i < maxIteration; i++)
    {
        if (z_re * z_re + z_im * z_im > 4.f)
            break;

        float new_re = z_re * z_re - z_im * z_im;
        float new_im = 2.f * z_re * z_im;
        z_re = x + new_re;
        z_im = y + new_im;
    }
    cudaMem[xid + yid * widthX] = i;
}

// Host front-end function that allocates the memory and launches the GPU kernel
void hostFE(float upperX, float upperY, float lowerX, float lowerY, int *img, int resX, int resY, int maxIterations)
{
    float stepX = (upperX - lowerX) / resX;
    float stepY = (upperY - lowerY) / resY;

    int *Mem = (int *)malloc(resX * resY * sizeof(int));
    int *cudaMem;
    cudaMalloc((void **)&cudaMem, resX * resY * sizeof(int));
    dim3 dimBLock(BlockSize, BlockSize);
    dim3 dimGrid((resX / BLockSize) + (resX % BlockSize == 0 ? 0 : 1),
                 (resY / BLockSize) + (resY % BlockSize == 0 ? 0 : 1));
    //  run on GPU
    mandelKernel<<<dimGrid, dimBlock>>>(cudaMem, lowerX, lowerY, stepX, stepY,
                                        maxIterations, resX, resY);
    //  wait for work done
    cudaDeviceSynchronize();
    cudaMemcpy((void *)Mem, (void *)cudaMem, resX * resY * sizeof(int),
               cudaMemcpyDeviceToHost);
    memcpy((void *)img, (void *)Mem, resX * resY * sizeof(int));
    // free mem
    cudaFree(cudaMem);
    free(Mem);
}