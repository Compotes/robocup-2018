#ifndef GPIO_H
#define GPIO_H

#include <fstream>
#include <iostream>
#include <string>
#include "constants.hpp"

#define SENSOR_1_READ_GPIO 298
#define SENSOR_2_READ_GPIO 388

#define COMPASS_RESET_WRITE_GPIO 480
#define COMPASS_RESET_READ_GPIO 486

void init_gpio();
int get_gpio_status(int gpio);

#endif
