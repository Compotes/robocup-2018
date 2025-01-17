#ifndef GOAL_H
#define GOAL_H

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
#include "image_processing.cuh"

#define BALL_TOLERANCE 13

__host__ object goal_detect(cv::cuda::GpuMat &src, int min_width, int min_height, int max_pixel);

#endif
