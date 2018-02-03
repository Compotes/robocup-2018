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

int azimuth;
int a;
#define CAM_W 1032
#define CAM_H 772
#define CAM_FOV 102.9

#define FORWARD_ANGLE 90
#define BACKWARD_ANGLE 270

#define GOT_BALL_THRESHOLD 340

int main(int argc, char* argv[]) {

	init_camera();
	init_compass();
	init_serial();
	init_server();
	init_gpio();

	//ext_azimuth.store(0);

	while(true) {
		//usleep(1000*100);
		//this_thread::sleep_for(chrono::milliseconds(500));

		int holding_ball = get_gpio_status(DRIBBLER_READ_GPIO);
        if (holding_ball == 1 || (ball_visible.load() && ball_y.load() > GOT_BALL_THRESHOLD)) {
            ext_degree.store(FORWARD_ANGLE);
            ext_speed.store(0);
            continue;
        }

        //printf("%5d %5d\n", ball_x.load(), ball_y.load());
        if (ball_visible.load()) {
            int bx = ball_x.load();
            int midx = CAM_W / 2;
            int px_deg = CAM_W / CAM_FOV;
            int deg = (bx - midx) / px_deg;
            //printf("deg %5d\n", deg);
            ext_degree.store(FORWARD_ANGLE - deg);
            ext_speed.store(30);
        } else {
            ext_degree.store(FORWARD_ANGLE);
            ext_speed.store(-30);
        }

        //
		azimuth = compass_degree.load();
		if(azimuth > 180){
			azimuth -= 360;
		}
		azimuth = azimuth * 75 / 180;
		ext_azimuth.store(azimuth);
	}
}

