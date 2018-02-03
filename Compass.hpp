#ifndef COMPASS_H
#define COMPASS_H

#include <iostream>
#include <thread>
#include <atomic>

#include "RTIMULib.h"
#include "RTIMUSettings.h"
#include "RTIMUBNO055.h"
#include "RTIMUDefs.h"
#include "RTFusionRTQF.h"
#include "RTMath.h"

#define SUCCESS_MSG "HOTOVE!"
#define FAIL_MSG "POKAZENE!"

#define BNO055 "BNO055 "

using namespace std;

extern atomic<int> compass_degree;
extern atomic<int> compass_zero;

void update_compass();
void init_compass();

#endif
