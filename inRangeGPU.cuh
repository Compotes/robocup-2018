#ifndef IN_RANGE_GPU_H
#define IN_RANGE_GPU_H

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdio.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/core/cuda.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/cudafilters.hpp"
#include "opencv2/cudaimgproc.hpp"
#include "opencv2/cudaarithm.hpp"
#include <iostream>

__host__ void inRange_gpu(cv::cuda::GpuMat &src, int minH, int minS, int minV,
				 int maxH, int maxS, int maxV, cv::cuda::GpuMat &dst);

#endif
