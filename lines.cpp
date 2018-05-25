#include "lines.hpp"

uint16_t loaded_values[CALIBRATION_VALUES_NUMBER];
uint16_t lines_values_from_file[CALIBRATION_VALUES_NUMBER];

int delta_azimuth(int x) {
	if (x < 0) {
		x = -x;
	}

	if (x > 180) {
		x = 360 - x;
	}

	return x;
}

void calibration () {
	ext_calibration.store(true);
	ext_start.store(true);
	this_thread::sleep_for(chrono::milliseconds(3000));
	ext_calibration.store(false);
}

void send_calibration_values () {

}

void save_lines_values(uint16_t loaded_value, int index) {
	loaded_values[index] = loaded_value;
}

void save_lines_values_to_file() {

	fstream calibration_values;
	calibration_values.open("/root/robocup-2018/CALIBRATION_VALUES.janojedebil", ios::out | ios::trunc);

	for(int i = 0; i < CALIBRATION_VALUES_NUMBER; i++) {
		calibration_values << loaded_values[i] << endl;
	}

	calibration_values.close();
	cout << "CALIBRATION DATA SAVED" << endl;
}

void load_lines_values_from_file() {
	fstream calibration_values;
	calibration_values.open("/root/robocup-2018/CALIBRATION_VALUES.janojedebil", ios::in);

	for(int i = 0; i < CALIBRATION_VALUES_NUMBER; i++) {
		calibration_values >> lines_values_from_file[i];
		cout << lines_values_from_file[i] << endl;
	}

	calibration_values.close();
	cout << "CALIBRATION DATA LOADED" << endl;
}
