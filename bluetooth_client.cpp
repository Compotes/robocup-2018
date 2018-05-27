#include "bluetooth_client.hpp"

atomic<bool> ext_goolkeeper;

struct sockaddr_rc addr = { 0 };
int c_sock, status;
char dest[18] = "00:04:4B:8C:D4:63";
char buf[1] = {0};
int bluetooth_ball_zone = 0;
int new_zone;

void init_bluetooth_client(){
	ext_goolkeeper.store(1);
	init_client();
	thread bluetooth_read_thread(bluetooth_read);
    bluetooth_read_thread.detach();
	thread bluetooth_write_thread(bluetooth_write);
    bluetooth_write_thread.detach();
}

void init_client(){
	cout << "start_conecting" << endl;
	c_sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = (uint8_t) 1;
	str2ba( dest, &addr.rc_bdaddr );

	status = connect(c_sock, (struct sockaddr *)&addr, sizeof(addr));
	if(status) {
		init_client();
	} else {
		cout << "bluetooth connected" << endl;
	}
}

void bluetooth_read() {
	while (true) {
		read(c_sock, buf, sizeof buf);
		if(buf[0] == '0') {
			ext_goolkeeper.store(0);
		} else {
			ext_goolkeeper.store(1);
		}
	}
}

void bluetooth_write() {
	while (true){
		if (ball_visible.load()) {
			new_zone = ext_ball_zone.load();
		} else {
			new_zone = 0;
		}
		if(new_zone != bluetooth_ball_zone) {
			bluetooth_ball_zone = new_zone;
			write(c_sock, to_string(bluetooth_ball_zone).c_str(), 1);
		}
	}
}
