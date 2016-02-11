#include "GPIO.hpp"

using namespace std;

void init_gpio() {
	cout << "STARTING GPIO" << endl;

	fstream sys_unexport;
	sys_unexport.open(GPIO_DIR + "unexport", ios::out);
	sys_unexport << DRIBBLER_READ_GPIO << endl;
	sys_unexport << DRIBBLER_WRITE_GPIO << endl;
	sys_unexport << COMPASS_RESET_READ_GPIO << endl;
	sys_unexport << COMPASS_RESET_WRITE_GPIO;
	sys_unexport.close();

	fstream sys_export;
	sys_export.open(GPIO_DIR + "export", ios::out);
	sys_export << DRIBBLER_READ_GPIO << endl;
	sys_export << DRIBBLER_WRITE_GPIO << endl;
	sys_export << COMPASS_RESET_READ_GPIO << endl;
	sys_export << COMPASS_RESET_WRITE_GPIO;
	sys_export.close();

	fstream sys_gpio_out;
	sys_gpio_out.open(GPIO_DIR + "gpio" + to_string(DRIBBLER_WRITE_GPIO) + "/direction", ios::out);
	sys_gpio_out << "out" << endl;
	sys_gpio_out.close();
	sys_gpio_out.open(GPIO_DIR + "gpio" + to_string(COMPASS_RESET_WRITE_GPIO) + "/direction", ios::out);
	sys_gpio_out << "out" << endl;
	sys_gpio_out.close();

	fstream sys_gpio_in;
	sys_gpio_in.open(GPIO_DIR + "gpio" + to_string(DRIBBLER_READ_GPIO) + "/direction", ios::out);
	sys_gpio_in << "in" << endl;
	sys_gpio_in.close();
	sys_gpio_in.open(GPIO_DIR + "gpio" + to_string(COMPASS_RESET_READ_GPIO) + "/direction", ios::out);
	sys_gpio_in << "in" << endl;
	sys_gpio_in.close();

	sys_gpio_in.open(GPIO_DIR + "gpio" + to_string(DRIBBLER_READ_GPIO) + "/active_low", ios::out);
	sys_gpio_in << "1" << endl;
	sys_gpio_in.close();
	sys_gpio_in.open(GPIO_DIR + "gpio" + to_string(COMPASS_RESET_READ_GPIO) + "/active_low", ios::out);
	sys_gpio_in << "1" << endl;
	sys_gpio_in.close();
}

int get_gpio_status(int gpio) {
	fstream gpio_file;
	gpio_file.open(GPIO_DIR + "gpio" + to_string(gpio) + "/value", ios::in);

	string line;
	getline(gpio_file, line);
	int val = atoi(line.c_str());

	gpio_file.close();
	return val;
}
