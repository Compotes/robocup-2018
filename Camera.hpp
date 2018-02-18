#ifndef CAMERA_H
#define CAMERA_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <atomic>
#include <opencv2/videoio.hpp>

#include "xiApiPlusOcv.hpp"
#include "inRangeGPU.cuh"
#include "goal.cuh"
#include "Compass.hpp"

#define EXPOSURE_TIME 10000
#define CAMERA_TIMEOUT 100000
#define FILTER 256
#define HEIGHT 1032
#define WIDTH 772
#define INPUT_CROP_TOP 30
#define GOAL_CROP_HEIGHT 400
#define MAX_GOAL_CENTERS_DISTANCE 300

extern atomic<int> frame_rate;
extern atomic<int> live_stream;

extern atomic<bool> ball_visible;
extern atomic<int> ball_x;
extern atomic<int> ball_y;

extern atomic<bool> goal_visible;
extern atomic<int> goal_x;
extern atomic<int> goal_y;

void update_camera();
void init_camera();
void update_filters(int, void*);
void load_values();

#endif