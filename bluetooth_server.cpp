#include "bluetooth_server.hpp"

using namespace std;

atomic<bool> i_attack;

int s_sock, client, bytes_read;

void init_bluetooth_server() {
	
	struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
    socklen_t opt = sizeof(rem_addr);

    s_sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    loc_addr.rc_family = AF_BLUETOOTH;    
	loc_addr.rc_channel = (uint8_t) 1;
    bind(s_sock, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
    listen(s_sock, 1);

    client = accept(s_sock, (struct sockaddr *)&rem_addr, &opt);
	
	thread read_bluetooth(bluetooth_read_thread);
	thread write_bluetooth(bluetooth_write_thread);
	
	read_bluetooth.detach();
	write_bluetooth.detach();
	
}

void bluetooth_read_thread() {
	char buf[1] = { 0 };
	cout << "CONNECTED" << endl;
	while (true) {
		bytes_read = read(client, buf, sizeof(buf));
		if( bytes_read > 0 ) {
			if (ball_visible.load()) {
				if (atoi(buf[0]) > ext_ball_zone.load() || atoi(buf[0]) == 0){
					i_attack.store(true);
				} else {
					i_attack.store(false);
				}
			} else {
				if (atoi(buf[0]) > 0) {
					i_attack.store(false);
				} else {
					i_attack.store(true);
				}
			}
		}
	}
}

void bluetooth_write_thread() {
	while (true) {
		if (i_attack.load()) {
			write(client, "0", 1);
		} else {
			write(client, "1", 1);
		}
	}
}