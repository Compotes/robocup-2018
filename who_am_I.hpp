#ifndef WHO_AM_I_H
#define WHO_AM_I_H

#include <cstdlib>
#include <string>
#include <atomic>
#include <iostream>
#include <fstream>
#include "constants.hpp"

using namespace std;

extern atomic<bool> i_am_server;

void init_who_am_I();

#endif
