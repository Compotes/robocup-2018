#include "Compass.hpp"

atomic<int> compass_degree;

RTIMUBNO055 *compass;
RTIMUSettings settings;
RTFusionRTQF *fusion;
RTIMU_DATA data;

atomic<int> compass_zero;
atomic<bool> ext_compass_reset;

int error_codes;
int degree;

void init_compass() {
    thread compass_thread(update_compass);
    compass_thread.detach();
}

void compass_reset() {
	compass = (RTIMUBNO055 *)RTIMU::createIMU(&settings);

	if ((error_codes = compass->IMUInit()) < 0) {
		cout << FAIL_MSG << endl;
	} else {
		cout << BNO055 << SUCCESS_MSG << endl;
	}

	compass->setSlerpPower(0.02);
    compass->setGyroEnable(true);
    compass->setAccelEnable(true);
    compass->setCompassEnable(true);
}

void update_compass() {

	compass_reset();

	compass_zero.store(0);
	ext_compass_reset.store(0);

	while (true) {
		if(ext_compass_reset.load()){
			compass_reset();
			ext_compass_reset.store(0);
		}
		compass->IMURead();
		RTIMU_DATA imuData = compass->getIMUData();
		degree = (imuData.fusionPose.z() * RTMATH_RAD_TO_DEGREE);

		compass_degree.store((degree-compass_zero.load()+360) % 360);
	}
}



