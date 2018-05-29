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

#include "bluetooth_client.hpp"
#include "who_am_I.hpp"
#include "bluetooth_server.hpp"

#define FORWARD_ANGLE 90
#define BACKWARD_ANGLE 270

#define GOT_BALL_THRESHOLD 340

#define GOAL_KICK_TOLERANCE 5

#define TRICK_SPEED 30
#define TRICK_TOLERANCE 20
#define I_HAVE_BALL_TOLLERANCE 6000

#define DEFAULT_SPEED 45
#define DEFAULT_FORWARD_SPEED 30
#define DEFAULT_BACKWARD_SPEED 30
#define KICK_DELAY 10 // in micro seconds
#define KICK_TIME_OUT 1 // in seconds
#define ROBOT_MAX 40
#define ATTACK_ANGLE_TOLERANCE 15

#define KICKER_DELAY_TO_KICK 1000

int azimuth;

// states
bool i_see_goal = false;
bool i_see_goal_to_kick = false;

bool i_see_ball_close = false;
bool i_have_ball = false;
bool i_see_ball = false;
bool i_see_ball_infront_me = false;
bool kick_started = false;

int i_have_ball_counter = 0;

bool trick = false;

bool last_site_right = false;
bool i_saw_line = false;
bool i_saw_line_again = false;

int ball_zone = BLYAT_ZONE_NUMBER;

timeval kicker_start;
timeval kicker_end;
timeval kick_start;
timeval kick_end;
bool kicker_available = true;

int compass_reset_status = 0;

int kicker_have_ball_delay = 0;

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

	init_who_am_I();
	init_camera();
	init_compass();
	init_serial();
	init_server();
	init_gpio();

	if (i_am_server) {
		init_bluetooth_server();
	} else {
		init_bluetooth_client();
	}

	ext_goolkeeper = 1;
	robot_speed.store(DEFAULT_SPEED);

	while(true) {
        int midx = CAM_W / 2;
        int px_deg = CAM_W / CAM_FOV;
        int bd = 0;
        int gd = 0;

		int gx = goal_x.load();
		gd = ((gx - midx) / px_deg);

		int bx = ball_x.load();
        bd = ((bx - midx) / px_deg);

		if (get_gpio_status(COMPASS_RESET_READ_GPIO)) {
			ext_compass_reset.store(true);
		}

		/*if (get_gpio_status(DRIBBLER_READ_GPIO) == 1 && i_have_ball_counter < I_HAVE_BALL_TOLLERANCE) {
			i_have_ball = true;
			i_have_ball_counter++;
		} else {*/
			//i_have_ball_counter = 0;
			//i_have_ball = false;
		//}

        i_see_ball = ball_visible.load();
        i_see_goal = goal_visible.load();
		ball_zone =  ext_ball_zone.load();//ball_close(ball_x.load());

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

		if ((FORWARD_ANGLE - bd) < (90+10) && (FORWARD_ANGLE - bd) > (90-10)) {
			if (ball_zone <= FIRST_ZONE_NUMBER) {
				i_see_ball_infront_me = 1;
			} else {
				i_see_ball_infront_me = 0;
			}
		} else {
			i_see_ball_infront_me = 0;
		}

		int local_degree = (FORWARD_ANGLE - bd);

		//Ball bypassing
		if (ball_visible.load()) {
			if (local_degree < (90 + ATTACK_ANGLE_TOLERANCE) && local_degree > (90 - ATTACK_ANGLE_TOLERANCE)) {
				local_speed = robot_speed.load();
			} else if (ball_zone <= SECOND_ZONE_NUMBER) {
				local_degree += 1.5*(local_degree-90)/ball_zone;
			} else if (ball_zone <= FIFTH_ZONE_NUMBER) {
				local_degree += 3*(local_degree-90)/ball_zone;
			}

        } else {
            local_degree = (compass_degree.load()+270) % 360;
        }

		i_have_ball = ball_close_kick; // false
		
		if (ext_goolkeeper) {
			if (!get_gpio_status(SENSOR_1_READ_GPIO) && !get_gpio_status(SENSOR_2_READ_GPIO)) {
				local_speed = 60;
				if(!i_saw_line) {
					local_degree = (compass_degree.load()+270) % 360;
				} else if(last_site_right) {
					local_degree = (compass_degree.load()+180) % 360;
				}else{
					local_degree = (compass_degree.load()+0) % 360;
				}
				if (ext_line_detected) {
					if(!i_saw_line){
						i_saw_line = true;
					} else if(!i_saw_line_again){
						i_saw_line_again = true;
						if(last_site_right) {
							last_site_right = false;
						} else {
							last_site_right = true;
						}
					} else {
						i_saw_line = false;
						i_saw_line_again = false;
					}
					ext_line_detected = false;
				}
			} else if (!get_gpio_status(SENSOR_1_READ_GPIO)) {
				local_speed = 60;
				last_site_right = true;
				local_degree = (compass_degree.load()+180) % 360;
			} else if (!get_gpio_status(SENSOR_2_READ_GPIO)) {
				local_speed = 60;
				last_site_right = false;
				local_degree = (compass_degree.load()+0) % 360;
			} else {
				local_speed = 80;
				i_saw_line = false;
				i_saw_line_again = false;
				if(!ball_visible || abs(bd) < 2) {
					local_speed = 0;
				} else if(bd < 0) {
					local_degree = (compass_degree.load()+180) % 360;
				} else {
					local_degree = (compass_degree.load()+0) % 360;
				}
			}
		} else {
			i_saw_line = false;
			i_saw_line_again = false;
		}

		if (i_see_goal) {
			trick = false;
            azimuth = (gd) * (-1);
		} else {
			trick = false;
            azimuth = (compass_degree.load()) % 360;
		}

		ext_degree.store(local_degree);
		ext_speed.store(local_speed);

		if (i_have_ball && i_see_goal_to_kick && kicker_available && i_see_ball_infront_me && !kick_started) /* && goal_height.load() > 70*/ {
			gettimeofday(&kicker_start, 0);
			gettimeofday(&kick_start, 0);

			kick_started = true;
			kicker_available = false;
		} /*else if (i_have_ball) {
			//trick = true;
		} */else if (i_have_ball && i_see_goal_to_kick && i_see_ball_infront_me && kick_started){
			gettimeofday(&kick_end, 0);
			if (kick_end.tv_usec - kick_start.tv_usec > KICK_DELAY) {
				ext_kick.store(true);
				kick_started = false;
			}
		} else {
			kick_started = false;
			kicker_available = true;
		}

		if (!kicker_available) {
			gettimeofday(&kicker_end, 0);
			if (kicker_end.tv_sec - kicker_start.tv_sec > KICK_TIME_OUT) {
				kicker_available = true;
			}
		}

		if (!trick) {
			if(azimuth > 180) {
				azimuth -= 360;
			}

			int az = (azimuth < 0) ? -1 : 1;
			if (goal_height.load() > 110) {
				azimuth *= 1.6;
			} else {
				azimuth *= 1.5;
			}
			if(ext_goolkeeper){
				azimuth *= 1.1;
			}
			azimuth = relative_azimuth(abs(azimuth)) * az;
		}

		ext_azimuth.store(azimuth);

		//ext_azimuth.store(azimuth * 100 / 180);

		mainMeasureFps();
	}
}
