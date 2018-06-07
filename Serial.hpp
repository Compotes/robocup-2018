#ifndef SERIAL_H
#define SERIAL_H

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include "lines.hpp"

#define SERIAL_SPEED B115200
#define PARITY 0

#define MOVE_COMMAND 255
#define KICK_COMMAND 254
#define START_COMMAND 251
#define LINE_CALIBRATION_COMMAND 250
#define INIT_COMMAND 249
#define RESET_COMMAND 248
#define DRIBBLER_START_COMMAND 247
#define LINE_DETECTED_COMMAND 246
#define STOP_COMMAND 245
#define START_ULTRASONIC_COMMAND 244
#define STOP_ULTRASONIC_COMMAND 243
#define LEFT_TRUE 242
#define LEFT_FALSE 241
#define RIGHT_TRUE 240
#define RIGHT_FALSE 239


#define DRIBBLER_STOP_SPEED 100

using namespace std;

extern int fd;

extern atomic<int> ext_degree;
extern atomic<int> ext_speed;
extern atomic<int> ext_dribbler_speed;
extern atomic<int> ext_azimuth;
extern atomic<bool> ext_calibration;
extern atomic<bool> ext_start;
extern atomic<bool> ext_dribbler_start;
extern atomic<bool> ext_kick;
extern atomic<bool> ext_send_calibration_data;
extern atomic<bool> ext_turn_off_dribbler;
extern atomic<bool> ext_line_detected;

int set_interface_attribs (int fd, int speed, int parity);
void set_blocking (int fd, int should_block);
int init_serial();
void write_protocol();
void read_protocol();
void sdPut(uint8_t byte);


#endif
