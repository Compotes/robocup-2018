#ifndef BLUETOOTH_SERVER_H
#define BLUETOOTH_SERVER_H

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

void init_bluetooth_server();
void bluetooth_read_thread();
void bluetooth_write_thread();

extern atomic<bool> i_attack;

#endif
