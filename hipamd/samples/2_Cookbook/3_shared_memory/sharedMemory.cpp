/*
Copyright (c) 2015-2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include<iostream>

// hip header file
#include "hip/hip_runtime.h"


#define WIDTH     1024
#define HEIGHT    1024

#define NUM       (WIDTH*HEIGHT)

#define THREADS_PER_BLOCK_X  16
#define THREADS_PER_BLOCK_Y  16
#define THREADS_PER_BLOCK_Z  1

// Device (Kernel) function, it must be void
// hipLaunchParm provides the execution configuration
__global__ void matrixTranspose(hipLaunchParm lp,
                                float *out,
                                float *in,
                                const int width,
                                const int height)
{
    __shared__ float sharedMem[16*16];

    int x = hipBlockDim_x * hipBlockIdx_x + hipThreadIdx_x;
    int y = hipBlockDim_y * hipBlockIdx_y + hipThreadIdx_y;

    sharedMem[y * width + x] = in[x * height + y];

    __syncthreads();

    out[y * width + x] = sharedMem[y * width + x];
}

// CPU implementation of matrix transpose
void matrixTransposeCPUReference(
    float * output,
    float * input,
    const unsigned int width,
    const unsigned int height)
{
    for(unsigned int j=0; j < height; j++)
    {
        for(unsigned int i=0; i < width; i++)
        {
            output[i*height + j] = input[j*width + i];
        }
    }
}

int main() {

  float* Matrix;
  float* TransposeMatrix;
  float* cpuTransposeMatrix;

  float* gpuMatrix;
  float* gpuTransposeMatrix;

  hipDeviceProp_t devProp;
  hipGetDeviceProperties(&devProp, 0);

  std::cout << "Device name " << devProp.name << std::endl;

  int i;
  int errors;

  Matrix = (float*)malloc(NUM * sizeof(float));
  TransposeMatrix = (float*)malloc(NUM * sizeof(float));
  cpuTransposeMatrix = (float*)malloc(NUM * sizeof(float));

  // initialize the input data
  for (i = 0; i < NUM; i++) {
    Matrix[i] = (float)i*10.0f;
  }

  // allocate the memory on the device side
  hipMalloc((void**)&gpuMatrix, NUM * sizeof(float));
  hipMalloc((void**)&gpuTransposeMatrix, NUM * sizeof(float));

  // Memory transfer from host to device
  hipMemcpy(gpuMatrix, Matrix, NUM*sizeof(float), hipMemcpyHostToDevice);

  // Lauching kernel from host
  hipLaunchKernel(matrixTranspose,
                  dim3(WIDTH/THREADS_PER_BLOCK_X, HEIGHT/THREADS_PER_BLOCK_Y),
                  dim3(THREADS_PER_BLOCK_X, THREADS_PER_BLOCK_Y),
                  0, 0,
                  gpuTransposeMatrix , gpuMatrix, WIDTH ,HEIGHT);

  // Memory transfer from device to host
  hipMemcpy(TransposeMatrix, gpuTransposeMatrix, NUM*sizeof(float), hipMemcpyDeviceToHost);

  // CPU MatrixTranspose computation
  matrixTransposeCPUReference(cpuTransposeMatrix, Matrix, WIDTH, HEIGHT);

  // verify the results
  errors = 0;
  double eps = 1.0E-6;
  for (i = 0; i < NUM; i++) {
    if (std::abs(TransposeMatrix[i] - cpuTransposeMatrix[i]) > 0 ) {
    printf("%d cpu: %f gpu  %f\n",i,cpuTransposeMatrix[i],TransposeMatrix[i]);
      errors++;
    }
  }
  if (errors!=0) {
    printf("FAILED: %d errors\n",errors);
  } else {
    printf ("PASSED!\n");
  }

  //free the resources on device side
  hipFree(gpuMatrix);
  hipFree(gpuTransposeMatrix);

  //free the resources on host side
  free(Matrix);
  free(TransposeMatrix);
  free(cpuTransposeMatrix);

  return errors;
}
