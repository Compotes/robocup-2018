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
#include "image_processing.cuh"

#define EXPOSURE_TIME 10000
#define CAMERA_TIMEOUT 100000
#define FILTER 256
#define HEIGHT 1032
#define WIDTH 772
#define INPUT_CROP_TOP 30
#define GOAL_CROP_HEIGHT 400
#define MAX_GOAL_CENTERS_DISTANCE 300

#define CAM_W 1032
#define CAM_H 772
#define CAM_FOV 102.9

#define NUMBER_OF_ZONES 5

#define FIRST_ZONE_NUMBER 1
#define SECOND_ZONE_NUMBER 2
#define THIRD_ZONE_NUMBER 3
#define FOURTH_ZONE_NUMBER 4
#define FIFTH_ZONE_NUMBER 5
#define BLYAT_ZONE_NUMBER 6

#define FIRST_ZONE_TOLERANCE 10
#define SECOND_ZONE_TOLERANCE 10
#define THIRD_ZONE_TOLERANCE 10
#define FOURTH_ZONE_TOLERANCE 10
#define FIFTH_ZONE_TOLERANCE 10


extern atomic<int> frame_rate;
extern atomic<int> live_stream;

extern atomic<bool> ball_visible;
extern atomic<int> ball_x;
extern atomic<int> ball_y;
extern atomic<int> ext_ball_zone;

extern atomic<bool> ball_close_kick;

extern atomic<bool> goal_visible;
extern atomic<int> goal_x;
extern atomic<int> goal_y;
extern atomic<int> goal_height;
extern atomic<int> goal_width;

extern atomic<bool> ext_livestream;

extern atomic<bool> ext_attack_blue_goal;

extern atomic<bool> ext_i_see_goal_to_kick;

void update_camera();
void init_camera();
void update_filters(int, void*);
void load_values();

#endif
