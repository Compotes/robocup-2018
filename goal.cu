#include "goal.cuh"

using namespace std;

 struct segment {
	long long average;
	int size;
	int sum;
} maximum, actual_segment;

__global__ void goal_kernel(const cv::cuda::PtrStepSz<uchar1> src, int* d_columns) {
	if(src(threadIdx.x, blockIdx.x).x == 255){
		atomicAdd(&d_columns[blockIdx.x], 1);
	}
}

__host__ int goal_detect(cv::cuda::GpuMat &src) {
	int h_columns[1032];
	int* d_columns;

	cudaMalloc(&d_columns, sizeof(int)*1032);
	goal_kernel<<<1032, 500>>>(src, d_columns);
	cudaMemcpy(&h_columns, d_columns, sizeof(int)*1032, cudaMemcpyDeviceToHost);
	cudaFree(d_columns);

	maximum = {0,0,0};
	actual_segment = {0,0,0};
	int hole = 0;

	for (int i = 0; i < 1032; i++) {
		if (h_columns[i] > 22) { // 20
			if(hole == 0){
				//actual_segment.average += h_columns[i]*i;
				actual_segment.size ++;
				//actual_segment.sum += h_columns[i];
			} else if(hole < 30){
				//actual_segment.average += h_columns[i]*i;
				actual_segment.size += hole+1;
				//actual_segment.sum += h_columns[i];
				hole = 0;
			} else {
				hole = 0;
				if(actual_segment.size > maximum.size) {
					maximum = actual_segment;
				}
				//actual_segment = {h_columns[i]*i,1,h_columns[i]};
				actual_segment = {i,1,0};
			}
			/*ball_visible = 1;
			center_x += h_columns[i]*i;
			sum += h_columns[i];*/
		} else {
			hole++;
		}
	}
	if(actual_segment.size > maximum.size) {
		maximum = actual_segment;
	}

	if(maximum.size > 38) {
		return maximum.average + maximum.size/2;//maximum.average/maximum.sum;
	}
	return -1;
}

