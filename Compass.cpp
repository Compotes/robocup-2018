#include "Compass.hpp"

atomic<int> compass_degree;

RTIMUBNO055 *compass;
RTIMUSettings settings;
RTFusionRTQF *fusion;
RTIMU_DATA data;

atomic<int> compass_zero;

void init_compass() {
    thread compass_thread(update_compass);
    compass_thread.detach();
}

void update_compass() {
	int error_code;
	int degree;
	compass = (RTIMUBNO055 *)RTIMU::createIMU(&settings);

	if ((error_code = compass->IMUInit()) < 0) {
		cout << FAIL_MSG << endl;
	} else {
		cout << BNO055 << SUCCESS_MSG << endl;
	}

	compass->setSlerpPower(0.02);
    compass->setGyroEnable(true);
    compass->setAccelEnable(true);
    compass->setCompassEnable(true);

	compass_zero.store(0);

	while (true) {
		compass->IMURead();
		RTIMU_DATA imuData = compass->getIMUData();
		degree = (imuData.fusionPose.z() * RTMATH_RAD_TO_DEGREE);

		compass_degree.store((degree-compass_zero.load()+360) % 360);
	}
}



