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

#define GOAL_KICK_TOLERANCE 5

#define TRICK_SPEED 30
#define TRICK_TOLERANCE 20
#define I_HAVE_BALL_TOLLERANCE 6000

#define DEFAULT_SPEED 20
#define DEFAULT_FORWARD_SPEED 30
#define DEFAULT_BACKWARD_SPEED 30
#define KICK_DELAY 1 // in seconds
#define ROBOT_MAX 40
#define ATTACK_ANGLE_TOLERANCE 15

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

#define KICKER_DELAY_TO_KICK 1000

int azimuth;

// states
bool i_see_goal = false;
bool i_see_goal_to_kick = false;

bool i_see_ball_close = false;
bool i_have_ball = false;
bool i_see_ball = false;

int i_have_ball_counter = 0;

bool idem_robit_trik = false;

int ball_zone = BLYAT_ZONE_NUMBER;

timeval kicker_start;
timeval kicker_end;
bool kicker_available = true;

int compass_reset_status = 0;

int calib_count = 0;
int kicker_have_ball_delay = 0;

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

		if (!get_gpio_status(COMPASS_RESET_READ_GPIO)) {
			ext_compass_reset.store(true);
		}

		if (get_gpio_status(DRIBBLER_READ_GPIO) == 0  && i_have_ball == 1 && i_have_ball_counter < I_HAVE_BALL_TOLLERANCE) {
			i_have_ball = true;
			i_have_ball_counter++;
		} else {
			i_have_ball_counter = 0;
			i_have_ball = !get_gpio_status(DRIBBLER_READ_GPIO);
		}

        i_see_ball = ball_visible.load();
        i_see_goal = goal_visible.load();
		ball_zone = ball_close(ball_x.load());

        if (ball_zone <= FIRST_ZONE_NUMBER) {
			i_see_ball_close = true;
		} else {
			i_see_ball_close = false;
		}

        if(ext_i_see_goal_to_kick) {
			i_see_goal_to_kick = true;
		} else {
			i_see_goal_to_kick = false;
		}

		int local_speed = robot_speed.load();

		if ((FORWARD_ANGLE - bd) < 93 && (FORWARD_ANGLE - bd) > 87) {
			if (ball_zone <= FIRST_ZONE_NUMBER) {
				i_have_ball = i_have_ball == 1 ? 1 : 0;
			} else {
				i_have_ball = 0;
			}
		} else {
			i_have_ball = 0;
		}

		int local_degree = (FORWARD_ANGLE - bd);

		//Ball bypassing
		if (ball_visible.load()) {
			if (local_degree < (90 + ATTACK_ANGLE_TOLERANCE) && local_degree > (90 - ATTACK_ANGLE_TOLERANCE)) {
				local_speed = robot_speed.load();
			} else if (ball_zone <= FIFTH_ZONE_NUMBER) {
				local_degree += 2*(local_degree-90)/ball_zone;
				/*int f;
				if ((local_degree) < 90) {
					f = 2*local_degree;
					local_degree -= f/ball_zone;
				} else {
					f = 2*(180-local_degree);
					local_degree += f/ball_zone;
				}*/
			}

        } else {
            local_degree = (compass_degree.load()+270) % 360;
        }


		if(i_have_ball) {
			idem_robit_trik = false;
			azimuth = (compass_degree.load()) % 360;
		} else if (i_see_goal) {
			idem_robit_trik = false;
            azimuth = (gd) * (-1);
		} else {
			idem_robit_trik = false;
            azimuth = (compass_degree.load()) % 360;
		}

		if (idem_robit_trik) {
			local_degree = (compass_degree.load()+90) % 360;
			local_speed = TRICK_SPEED;
			azimuth = (compass_degree.load()+180) % 360;

			if (abs(azimuth) < TRICK_TOLERANCE) {
				local_speed = 0;
			} else {
				azimuth = 100;
				local_speed = 10;
			}
		}

        ext_degree.store(local_degree);
		ext_speed.store(local_speed);

		if (i_have_ball) {
			kicker_have_ball_delay++;
		} else {
			kicker_have_ball_delay = 0;
		}

		if (i_have_ball && i_see_goal_to_kick && kicker_available && !idem_robit_trik /*&& kicker_have_ball_delay > KICKER_DELAY_TO_KICK*/) {
			gettimeofday(&kicker_start, 0);
			ext_kick.store(true);
			i_see_goal_to_kick = false;
			kicker_available = false;
			kicker_have_ball_delay = 0;
		}

		if (!kicker_available) {
			gettimeofday(&kicker_end, 0);
			if (kicker_end.tv_sec - kicker_start.tv_sec > KICK_DELAY) {
				kicker_available = true;
			}
		}

		if (!idem_robit_trik) {
			if(azimuth > 180) {
				azimuth -= 360;
			}

			int az = (azimuth < 0) ? -1 : 1;
			if (goal_height.load() > 110) {
				azimuth *= 1.7;
			} else {
				azimuth *= 1.5;
			}
			azimuth = relative_azimuth(abs(azimuth)) * az;
		}

		ext_azimuth.store(azimuth);

		//ext_azimuth.store(azimuth * 100 / 180);

		mainMeasureFps();
	}
}

