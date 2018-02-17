#include <stdio.h>
#include "xiApiPlusOcv.hpp"
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <ctime>

#include "Compass.hpp"
#include "Serial.hpp"
#include "Camera.hpp"
#include "Server.hpp"
#include "GPIO.hpp"

#define CAM_W 1032
#define CAM_H 772
#define CAM_FOV 102.9

#define FORWARD_ANGLE 90
#define BACKWARD_ANGLE 270

#define GOT_BALL_THRESHOLD 340

#define GOAL_KICK_TOLERANCE 15

#define DEFAULT_SPEED 20
#define DEFAULT_FORWARD_SPEED 30
#define DEFAULT_BACKWARD_SPEED 30
#define KICK_DELAY 3 // in seconds
#define ROBOT_MAX 40

#define FIRST_ZONE_TOLERANCE 150
#define SECOND_ZONE_TOLERANCE 10
#define THIRD_ZONE_TOLERANCE 10
#define FOURTH_ZONE_TOLERANCE 10
#define FIFTH_ZONE_TOLERANCE 10

#define NUMBER_OF_ZONES 5

#define FIRST_ZONE_NUMBER 1
#define SECOND_ZONE_NUMBER 2
#define THIRD_ZONE_NUMBER 3
#define FOURTH_ZONE_NUMBER 4
#define FIFTH_ZONE_NUMBER 5
#define BLYAT_ZONE_NUMBER 6

int azimuth;

// states
bool i_see_goal = false;
bool i_see_goal_to_kick = false;

bool i_see_ball_close = false;
bool i_have_ball = false;
bool i_see_ball = false;

int ball_zone = BLYAT_ZONE_NUMBER;

timeval kicker_start;
timeval kicker_end;
bool kicker_available = true;

int calib_count = 0;

void distance_calibration(int count) {
	fstream calibration_data;
	calibration_data.open("D_CALIB_DATA." + to_string(count), ios::out);
	cout << "calibration STARTS in" << endl;
	for (int i = 5; i > 0; i--) {
		cout << i << endl;
		usleep(1000*1000);
	}
	for(int i = 0; i < 15*100; i++) {
		calibration_data << ball_x.load() << " " << ball_y.load() << endl;
		cout << i << endl;
		usleep(1000*10);
	}
	calib_count++;
	calibration_data.close();
}

void start_distance_calibration_thread() {
	thread calib(distance_calibration, calib_count);
	calib.detach();
}

int ball_close(int x) {
	//first zone
	float res = 0.0006484349*x*x-0.648445*x+499.613981 - FIRST_ZONE_TOLERANCE;
	if (res < ball_y.load()) return FIRST_ZONE_NUMBER;

	//second zone
	res = 0.0005064493*x*x-0.5180190126*x+351.566136 - SECOND_ZONE_TOLERANCE;
	if (res < ball_y.load()) return SECOND_ZONE_NUMBER;

	//third zone
	res = 0.0003762345*x*x-0.3793232269*x+268.615192 - THIRD_ZONE_TOLERANCE;
	if (res < ball_y.load()) return THIRD_ZONE_NUMBER;

	// fourth zone
	res = 0.0003288547*x*x-0.3320353119*x+227.712064 - FOURTH_ZONE_TOLERANCE;
	if (res < ball_y.load()) return FOURTH_ZONE_NUMBER;

	// fifth zone
	res = 0.0003011454*x*x-0.3028442648*x+201.358953 - FIFTH_ZONE_TOLERANCE;
	if (res < ball_y.load()) return FIFTH_ZONE_NUMBER;

	return BLYAT_ZONE_NUMBER;
}

unsigned int mainCounter = 0;
unsigned int mainFps = 0;
struct timespec mainT0, mainT1;

void mainMeasureFps() {
    clock_gettime(CLOCK_REALTIME, &mainT1);
    uint64_t deltaTime = (mainT1.tv_sec - mainT0.tv_sec) * 1000000 + (mainT1.tv_nsec - mainT0.tv_nsec) / 1000 / 1000;
    mainCounter++;
    if (deltaTime > 1000) {
        mainFps = mainCounter;
        printf("MAIN FPS:%10d ", mainFps);
        mainCounter = 0;
        mainT0 = mainT1;
    }
}

int relative_azimuth(int x) {
	return (0.0011552787*x*x+0.3192050394*x)+3;
}

int main(int argc, char* argv[]) {

	init_camera();
	init_compass();
	init_serial();
	init_server();
	init_gpio();

	robot_speed.store(DEFAULT_SPEED);

	while(true) {
        int midx = CAM_W / 2;
        int px_deg = CAM_W / CAM_FOV;
        int alignWeight = 98;
        int bd = 0;
        int gd = 0;

		int gx = goal_x.load();
		gd = ((gx - midx) / px_deg);

		int bx = ball_x.load();
        bd = ((bx - midx) / px_deg);

		i_have_ball = get_gpio_status(DRIBBLER_READ_GPIO);
        i_see_ball = ball_visible.load();
        i_see_goal = goal_visible.load();
		ball_zone = ball_close(ball_x.load());

        if (ball_zone <= BLYAT_ZONE_NUMBER) {
			i_see_ball_close = true;
		} else {
			i_see_ball_close = false;
		}

        if(abs(gd) < GOAL_KICK_TOLERANCE) {
			i_see_goal_to_kick = true;
		} else {
			i_see_goal_to_kick = false;
		}

		int local_speed = robot_speed.load();

		if ((FORWARD_ANGLE - bd) < 100 && (FORWARD_ANGLE - bd) > 80) {
			if (ball_zone <= FIRST_ZONE_NUMBER) {
				i_have_ball = i_have_ball == 1 ? 1 : 0;
			} else {
				i_have_ball = 0;
			}
		} else {
			i_have_ball = 0;
		}

		int local_degree = (FORWARD_ANGLE - bd);
		if (ball_visible.load()) {
			if (local_degree < 110 && local_degree > 70) {
				local_speed = robot_speed.load();
			} else if (i_see_ball_close) {
				int f;
				if ((local_degree) < 90) {
					f = 1.7*local_degree;
					local_degree -= f/ball_zone;
				} else {
					f = 1.7*(180-local_degree);
					local_degree += f/ball_zone;
				}
			}

        } else {
            local_degree = (compass_degree.load()+270) % 360;
        }
        ext_degree.store(local_degree);
		ext_speed.store(local_speed);

		/*if (i_see_ball && !i_have_ball) {
			azimuth = (bd) % 360 * (-1);
		} else */if (i_see_goal) {
            azimuth = (gd) * (-1);
		} else {
            azimuth = (compass_degree.load()) % 360;
		}

		if (i_have_ball && i_see_goal_to_kick && kicker_available) {
			gettimeofday(&kicker_start, 0);
			ext_kick.store(true);
			i_see_goal_to_kick = false;
			kicker_available = false;
		}

		if (!kicker_available) {
			gettimeofday(&kicker_end, 0);
			if (kicker_end.tv_sec - kicker_start.tv_sec > KICK_DELAY) {
				kicker_available = true;
			}
		}

		if(false/*i_have_ball*/) {
			ext_azimuth.store(100);
			if(azimuth > 180) {
				azimuth -= 360;
			}

			int az = (azimuth < 0) ? -1 : 1;
			azimuth = relative_azimuth(abs(azimuth)) * az;

			azimuth = (azimuth * alignWeight / 180);
			ext_speed.store(azimuth);
		} else {
			if(azimuth > 180) {
				azimuth -= 360;
			}

			/*int az = (azimuth < 0) ? -1 : 1;
			azimuth = relative_azimuth(abs(azimuth)) * az;

			azimuth = (azimuth * alignWeight / 180);
			ext_azimuth.store(azimuth);*/

			ext_azimuth.store(azimuth * 100 / 180);
		}

		mainMeasureFps();
	}
}

