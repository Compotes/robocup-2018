#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>


#include "Camera.hpp"

void init_bluetooth();
void init_client();
void bluetooth_read();
void bluetooth_write();


#endif