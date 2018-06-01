#ifndef LINES_H
#define LINES_H

#define DELTA_ANGLE 20
#define CALIBRATION_DEGREE 0
#define CALIBRATION_SPEED 110
#define CALIBRATION_AZIMUTH 200
#define CALIBRATION_VALUES_NUMBER 16

#include <fstream>
#include <iostream>
#include <atomic>
#include <string>
#include "Compass.hpp"
#include "Serial.hpp"
#include "constants.hpp"

using namespace std;

extern uint16_t lines_values_from_file[CALIBRATION_VALUES_NUMBER];


void calibration();
void send_calibration_values();
void save_lines_values(uint16_t loaded_value, int index);
void save_lines_values_to_file();
void load_lines_values_from_file();

#endif
