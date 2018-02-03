#include "Serial.hpp"

struct termios tty;

int fd;
bool calibrating = false;
bool start = false;

atomic<int> ext_degree;
atomic<int> ext_speed;
atomic<int> ext_azimuth;
atomic<bool> ext_calibration;
atomic<bool> ext_start;
atomic<bool> ext_kick;
atomic<bool> ext_send_calibration_data;

struct data {
	uint8_t degree;
	uint8_t speed;
	uint8_t azimuth;
} serial_data;

int init_serial() {
	char *portname = "/dev/ttyTHS2";
	fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	set_interface_attribs (fd, B115200, 0);

	ext_degree.store(0);
	ext_speed.store(0);
	ext_azimuth.store(0);
	ext_calibration.store(0);
	ext_start.store(0);
	ext_kick.store(0);
	ext_send_calibration_data.store(0);


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
		} else if (buf[0] == INIT_COMMAND) {
			load_lines_values_from_file();
			ext_send_calibration_data.store(true);
		}
	}
}

void write_protocol() {

	while (true) {
		uint16_t degree = ext_degree.load();

		serial_data.degree = degree % 180;
		serial_data.speed = ext_speed.load() + 100;
		serial_data.azimuth = (ext_azimuth.load() + 100);

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
			sdPut(MOVE_COMMAND);
			sdPut(serial_data.degree);
			sdPut(serial_data.speed);
			sdPut(serial_data.azimuth);
		}

		if (ext_start && !start) {
			start = true;
			sdPut(START_STOP_COMMAND);
		} else if (!ext_start && start) {
			start = false;
			sdPut(START_STOP_COMMAND);
		}

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

			ext_send_calibration_data.store(false);
		}
	}
}

void sdPut(uint8_t byte) {
	write(fd, &byte, 1);
}

