#ifndef GPIO_H
#define GPIO_H

#include <fstream>
#include <iostream>
#include <string>

#define DRIBBLER_READ_GPIO 298
#define DRIBBLER_WRITE_GPIO 388

const std::string GPIO_DIR = "/sys/class/gpio/";
void init_gpio();
int get_gpio_status(int gpio);

#endif
