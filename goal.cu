#include "goal.cuh"

using namespace std;

struct segment {
	int start_pointer;
	int size;
} maximum_column, actual_segment_column, maximum_row, actual_segment_row;

//int max_width(int y){
//	return y*0.5033116626-7.0541699873+BALL_TOLERANCE;
//}

__global__ void goal_kernel(const cv::cuda::PtrStepSz<uchar1> src, int* d_columns, int* d_rows) {
	if(src(threadIdx.x, blockIdx.x).x == 255){
		atomicAdd(&d_columns[blockIdx.x], 1);
		atomicAdd(&d_rows[threadIdx.x], 1);
	}
}

__host__ object goal_detect(cv::cuda::GpuMat &src, int min_width, int min_height, int max_pixel) {
	int h_columns[1032], h_rows[max_pixel];
	int* d_columns, *d_rows;

	cudaMalloc(&d_columns, sizeof(int)*1032);
	cudaMalloc(&d_rows, sizeof(int)*max_pixel);
	goal_kernel<<<1032, max_pixel>>>(src, d_columns, d_rows);
	cudaMemcpy(&h_columns, d_columns, sizeof(int)*1032, cudaMemcpyDeviceToHost);
	cudaFree(d_columns);
	cudaMemcpy(&h_rows, d_rows, sizeof(int)*max_pixel, cudaMemcpyDeviceToHost);
	cudaFree(d_rows);

	maximum_column = {0,0};
	actual_segment_column = {0,0};
	int hole = 0;

	for (int i = 0; i < 1032; i++) {
		if (h_columns[i] > min_height) { // 20
			if(hole == 0){
				actual_segment_column.size ++;
			} else if(hole < 35){
				actual_segment_column.size += hole+1;
				hole = 0;
			} else {
				hole = 0;
				if(actual_segment_column.size > maximum_column.size) {
					maximum_column = actual_segment_column;
				}
				actual_segment_column = {i,1};
			}
		} else {
			hole++;
		}
	}
	if(actual_segment_column.size > maximum_column.size) {
		maximum_column = actual_segment_column;
	}

	maximum_row = {0,0};
	actual_segment_row = {0,0};
	hole = 0;

	for (int i = 0; i < max_pixel; i++) {
		if (h_rows[i] > min_width) { // 20
			if(hole == 0){
				actual_segment_row.size ++;
			} else if(hole < 35){
				actual_segment_row.size += hole+1;
				hole = 0;
			} else {
				hole = 0;
				if(actual_segment_row.size > maximum_row.size) {
					maximum_row = actual_segment_row;
				}
				actual_segment_row = {i,1};
			}
		} else {
			hole++;
		}
	}
	if(actual_segment_row.size > maximum_row.size) {
		maximum_row = actual_segment_row;
	}

	//if (max_width(maximum_row.start_pointer + maximum_row.size/2) < maximum_column.size) {
	//	return {-1,-1,-1,-1};
	//}

	if(maximum_column.size > min_width) {
		return {maximum_column.start_pointer + maximum_column.size/2, maximum_row.start_pointer + maximum_row.size/2, maximum_column.size, maximum_row.size};
	}
	return {-1,-1,-1,-1};
}

