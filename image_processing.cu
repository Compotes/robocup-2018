#include "image_processing.cuh"

using namespace std;

__device__ __managed__ int h_columns[1032], h_rows[800];

int max_width(int y){
	return y*0.5365897901-2.6025128518+BALL_TOLERANCE;
}

__global__ void column_kernel(const cv::cuda::PtrStepSz<uchar1> src, int* d_columns) {
	if(src(threadIdx.x, blockIdx.x).x == 255){
		atomicAdd(&d_columns[blockIdx.x], 1);
	}
}

__global__ void row_kernel(const cv::cuda::PtrStepSz<uchar1> src, int* d_rows, int start_x) {
	if(src(threadIdx.x, blockIdx.x+start_x).x == 255){
		atomicAdd(&d_rows[threadIdx.x], 1);
	}
}

__global__ void final_kernel(cv::cuda::PtrStepSz<uchar1> src, int* d_columns, int start_x, int start_y) {
	if(src(threadIdx.x+start_y, blockIdx.x+start_x).x == 255){
		atomicAdd(&d_columns[blockIdx.x], 1);
	}
}

__host__ object object_detect(cv::cuda::GpuMat &src, int min_width, int min_height, int max_pixel) {
	object maximum = {-1,-1,-1,-1}, actual_segment;
	vector < object > column_objects;
	vector < object > row_objects;
	vector < object > final_objects;
	//int h_columns[1032], h_rows[max_pixel];
	//int* d_columns, *d_rows;

	//cudaMalloc(&d_columns, sizeof(int)*1032);

    cudaMemset(h_columns, 0, 1032*sizeof(int));
    column_kernel<<<1032, max_pixel>>>(src, h_columns);
	//cudaMemcpy(&h_columns, d_columns, sizeof(int)*1032, cudaMemcpyDeviceToHost);
	//cudaFree(d_columns);
	cudaDeviceSynchronize();

	actual_segment = {0,0,0,max_pixel};
	int hole = 0;

	for (int i = 0; i < 1032; i++) {
		if (h_columns[i] > min_height) { // 20
			if(hole == 0){
				actual_segment.width ++;
			} else if(hole < 35){
				actual_segment.width += (hole+1);
				hole = 0;
			} else {
				hole = 0;
				if(actual_segment.width != 0) column_objects.push_back(actual_segment);
				actual_segment = {i,0,1,max_pixel};
			}
		} else {
			hole++;
		}
	}
	if(actual_segment.width != 0) column_objects.push_back(actual_segment);

	for (int j = 0; j < column_objects.size(); j++) {

		//cudaMalloc(&d_rows, sizeof(int)*max_pixel);
		cudaMemset(h_rows, 0, sizeof(int)*max_pixel);
		row_kernel<<<column_objects[j].width, max_pixel>>>(src, h_rows, column_objects[j].x);
		//cudaMemcpy(&h_rows, d_rows, sizeof(int)*max_pixel, cudaMemcpyDeviceToHost);
		//cudaFree(d_rows);
		cudaDeviceSynchronize();

		actual_segment = {column_objects[j].x, 0, column_objects[j].width, 0};
		hole = 0;

		for (int i = 0; i < max_pixel; i++) {
			if (h_rows[i] > min_width) { // 20
				if(hole == 0){
					actual_segment.height ++;
				} else if(hole < 10){
					actual_segment.height += (hole+1);
					hole = 0;
				} else {
					hole = 0;
					if(actual_segment.height != 0) row_objects.push_back(actual_segment);
					actual_segment = {column_objects[j].x, i, column_objects[j].width, 1};
				}
			} else {
				hole++;
			}
		}
		if(actual_segment.height != 0) row_objects.push_back(actual_segment);
	}

	for (int j = 0; j < row_objects.size(); j++) {

		//cudaMalloc(&d_columns, sizeof(int)*row_objects[j].width);
        cudaMemset(h_columns, 0, sizeof(int)*row_objects[j].width);
		final_kernel<<<row_objects[j].width, row_objects[j].height>>>(src, h_columns, row_objects[j].x, row_objects[j].y);
		//cout << j << " h" << row_objects[j].height << endl;
		//cout << j << " y" << row_objects[j].y << endl;
		//cudaMemcpy(&h_columns, d_columns, sizeof(int)*row_objects[j].width, cudaMemcpyDeviceToHost);
		//cudaFree(d_columns);
		cudaDeviceSynchronize();

		//actual_segment = {row_objects[j].x, row_objects[j].y, 0, row_objects[j].height};
		int hole = 0;
		int test = 0;
		while(h_columns[test] <= min_height) test++;
		actual_segment = {row_objects[j].x+test, row_objects[j].y, 1, row_objects[j].height};
		for (int i = test; i < row_objects[j].width; i++) {
			if (h_columns[i] > min_height) { // 20
				if(hole == 0){
					actual_segment.width ++;
				} else if(hole < 35){
					actual_segment.width += (hole+1);
					hole = 0;
				} else {
					hole = 0;
					if(actual_segment.width != 0) final_objects.push_back(actual_segment);
					actual_segment = {row_objects[j].x+i, row_objects[j].y, 1, row_objects[j].height};
				}
			} else {
				hole++;
			}
		}
		if(actual_segment.width != 0) final_objects.push_back(actual_segment);

	}

	for (int j = 0; j < final_objects.size(); j++) {
		if(final_objects[j].width*final_objects[j].height > maximum.width*maximum.height && final_objects[j].width < max_width(final_objects[j].y + final_objects[j].height/2)){
			maximum = final_objects[j];
		}
	}

	//if (max_width(maximum_row.start_pointer + maximum_row.size/2) < maximum_column.size) {
	//	return {-1,-1,-1,-1};
	//}
	if(maximum.width*maximum.height > 10) {
		return {maximum.x + maximum.width/2, maximum.y + maximum.height/2, maximum.width, maximum.height};
	}
	return {-1,-1,-1,-1};
}
