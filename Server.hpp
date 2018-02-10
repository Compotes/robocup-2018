#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <string.h>
#include <fstream>
#include <map>

#include "Compass.hpp"
#include "Camera.hpp"
#include "Serial.hpp"
#include "lines.hpp"

#define VERSION 23
#define BUFSIZE 8096
#define ERROR      42
#define LOG        44
#define FORBIDDEN 403
#define NOTFOUND  404

extern atomic<int> robot_speed;

void init_server();

#endif
