#include "Serial.hpp"

struct termios tty;

int fd;
bool calibrating = false;
bool start = false;
bool dribbler_start = false;
bool local_goalkeeper;

atomic<int> ext_degree;
atomic<int> ext_speed;
atomic<int> ext_dribbler_speed;
atomic<int> ext_azimuth;
atomic<bool> ext_calibration;
atomic<bool> ext_start;
atomic<bool> ext_real_start;
atomic<bool> ext_dribbler_start;
atomic<bool> ext_kick;
atomic<bool> ext_send_calibration_data;
atomic<bool> ext_turn_off_dribbler;
atomic<bool> ext_line_detected;
atomic<int> ext_left_ultrasonic;
atomic<int> ext_right_ultrasonic;


bool data_changed = 0;

struct data {
	uint8_t degree;
	uint8_t speed;
	uint8_t azimuth;
} serial_data;

int init_serial() {
	const char *portname = "/dev/ttyTHS2";
	fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	set_interface_attribs (fd, B115200, 0);

	ext_degree.store(0);
	ext_speed.store(0);
	ext_azimuth.store(0);
	ext_calibration.store(0);
	ext_start.store(0);
	ext_kick.store(0);
	ext_send_calibration_data.store(0);
	ext_line_detected.store(0);

	load_lines_values_from_file();
	ext_send_calibration_data.store(true);

    thread serial_write_thread(write_protocol);
    serial_write_thread.detach();

    thread serial_read_thread(read_protocol);
    serial_read_thread.detach();
    return 0;
}

int set_interface_attribs (int fd, int speed, int parity) {
        struct termios tty;
        memset (&tty, 0, sizeof tty);

        tcgetattr (fd, &tty);

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

		tcsetattr (fd, TCSANOW, &tty);
        return 0;
}

void read_protocol() {
	char buf[1];
	uint16_t value;

	while (true) {
		buf[0] = 0;
		read(fd, buf, sizeof buf);

		if (buf[0] == LINE_CALIBRATION_COMMAND) {
			for(int i = 0; i < CALIBRATION_VALUES_NUMBER; i++) {
				read(fd, buf, sizeof buf);
				value = buf[0];

				cout << value << endl;

				save_lines_values(value, i);
			}
			save_lines_values_to_file();
		} else if (buf[0] == LINE_DETECTED_COMMAND) {
			ext_line_detected.store(true);
		} else if (buf[0] == INIT_COMMAND) {
			load_lines_values_from_file();
			ext_send_calibration_data.store(true);
		} else if (buf[0] == START_COMMAND){
			cout << "start" << endl;
			ext_start.store(true);
		} else if (buf[0] == STOP_COMMAND){
			cout << "stop" << endl;
			ext_start.store(false);
		} else if (buf[0] == LEFT_FALSE){
			cout << "left: 0" << endl;
			ext_left_ultrasonic.store(0);
		} else if (buf[0] == LEFT_TRUE){
			cout << "left: 1" << endl;
			ext_left_ultrasonic.store(1);
		} else if (buf[0] == RIGHT_FALSE){
			cout << "right: 0" << endl;
			ext_right_ultrasonic.store(0);
		} else if (buf[0] == RIGHT_TRUE){
			cout << "right: 1" << endl;
			ext_right_ultrasonic.store(1);
		} else if (buf[0] == LEFT_CLOSE) {
			ext_left_ultrasonic.store(2);
		} else if (buf[0] == RIGHT_CLOSE) {
			ext_right_ultrasonic.store(2);
		}
	}
}

unsigned int serialCounter = 0;
unsigned int serialFps = 0;
struct timespec serialT0, serialT1;

void serialMeasureFps() {
    clock_gettime(CLOCK_REALTIME, &serialT1);
    uint64_t deltaTime = (serialT1.tv_sec - serialT0.tv_sec) * 1000000 + (serialT1.tv_nsec - serialT0.tv_nsec) / 1000 / 1000;
    serialCounter++;
    if (deltaTime > 1000) {
        serialFps = serialCounter;
        printf("SERIAL FPS:%10d ", serialFps);
        serialCounter = 0;
        serialT0 = serialT1;
    }
}

void write_protocol() {
	local_goalkeeper = ext_goalkeeper;

	while (true) {
		int16_t degree = ext_degree.load();
		int speed = ext_speed.load();
		int dribbler_speed = ext_dribbler_speed.load() + 100;
		int azimuth = ext_azimuth.load();

		if (azimuth > 80/*100*/) azimuth = 80;//100;
		if (azimuth < -80/*100*/) azimuth = -80;//100;

		if (degree >= 180) {
			degree -= 180;
			speed = -speed;
		}

		if (degree < 0) {
			speed = -speed;
			degree = 180+degree;
		}

		data_changed = false;
		data_changed = (serial_data.degree == degree % 180) ? 1 : 0;
		data_changed = (serial_data.speed == speed + 100) ? 1 : 0;
		data_changed = (serial_data.azimuth == azimuth + 100) ? 1 : 0;

		serial_data.degree = degree % 180;
		serial_data.speed = speed + 100;
		serial_data.azimuth = (azimuth + 100);

		if (degree > 180) serial_data.speed += 100;

		if (ext_calibration && !calibrating) {
			calibrating = true;
			sdPut(LINE_CALIBRATION_COMMAND);
			cout << "Calibration start" << endl;
		} else if (!ext_calibration && calibrating) {
			calibrating = false;
			sdPut(LINE_CALIBRATION_COMMAND);
			cout << "Calibration end" << endl;
		}

		if (calibrating) {
			sdPut(MOVE_COMMAND);
			sdPut(CALIBRATION_DEGREE);
			sdPut(CALIBRATION_SPEED);
			sdPut(CALIBRATION_AZIMUTH);
		} else {
			if (data_changed) {
				sdPut(MOVE_COMMAND);
				sdPut(serial_data.degree);
				sdPut(serial_data.speed);
				sdPut(serial_data.azimuth);
				//cout << serial_data.azimuth -100 << '\n';
			}
		}

		if (ext_dribbler_start && !dribbler_start) {
			dribbler_start = true;
			sdPut(DRIBBLER_START_COMMAND);
			sdPut(dribbler_speed);
		} else if (!ext_dribbler_start && dribbler_start) {
			dribbler_start = false;
			sdPut(DRIBBLER_START_COMMAND);
			sdPut(DRIBBLER_STOP_SPEED);
		}

		/*if (ext_start && !start) {
			start = true;
			sdPut(START_COMMAND);
		} else if (!ext_start && start) {
			start = false;
			sdPut(STOP_COMMAND);
		}*/

		if (ext_kick) {
			sdPut(KICK_COMMAND);
			ext_kick.store(0);
		}

		if (ext_send_calibration_data) {
			sdPut(INIT_COMMAND);

			for(int i = 0; i < CALIBRATION_VALUES_NUMBER; i++) {
				cout << lines_values_from_file[i] << endl;
				sdPut(lines_values_from_file[i]);
			}
			/*if(ext_start.load()){
				sdPut(START_COMMAND);
			} else {
				sdPut(STOP_COMMAND);
			}*/
			if(dribbler_start){
				sdPut(DRIBBLER_START_COMMAND);
				sdPut(dribbler_speed);
			}
			ext_send_calibration_data.store(false);
			if(local_goalkeeper) sdPut(START_ULTRASONIC_COMMAND);
		}
		if(local_goalkeeper && !ext_goalkeeper){
			local_goalkeeper = false;
			sdPut(STOP_ULTRASONIC_COMMAND);
		} else if(!local_goalkeeper && ext_goalkeeper){
			local_goalkeeper = true;
			sdPut(START_ULTRASONIC_COMMAND);
		}
		serialMeasureFps();
		this_thread::sleep_for(chrono::milliseconds(10));
	}
}

void sdPut(uint8_t byte) {
	write(fd, &byte, 1);
}

