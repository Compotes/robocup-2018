#include "Server.hpp"

using namespace std;

int size = 0;
int A[20];
string html = "";
bool go = false;


atomic<int> robot_speed;
map<string, string> arguments;
map<string, string> json_data;
string url;

void load_html_for_server() {
	fstream html_file;
	html_file.open("/root/robocup-2018/server.html", ios::in);

	string line;
	html = "";
	while(getline(html_file, line)) {
		html += line;
	}

	cout << "HTML LOADED" << endl;
}

string load_png(const string name) {
	string ret;
	fstream png;
	png.open(name, ios::in);

	string line;
	while(getline(png, line)) {
		ret += line;
	}
	return ret;
}

void web(int fd) {
	long ret;
	static char buffer[BUFSIZE+1]; /* static so zero filled */
	static char bufer[BUFSIZE+1];
	ret =read(fd,buffer,BUFSIZE); 	/* read Web request in one go */
	//cout << "--------------------------------------\n\n";
	if(ret > 0 && ret < BUFSIZE)	/* return code is valid chars */
		buffer[ret]=0;		/* terminate the buffer */
	else buffer[0]=0;
	//printf(buffer);

	if(!strncmp(buffer, "GET /?",6) || !strncmp(buffer,"get /?",6)) {
		for(int i = 5; buffer[i] != ' ';) {
			string var = "", val = "";
			i++;
			while(buffer[i] != '=') {
				var += buffer[i];
				i++;
			}

			i++;
			while(buffer[i] != '&' && buffer[i] != ' ') {
				val += buffer[i];
				i++;
			}

			arguments[var] = val;
		}

		if (arguments["kick"] == "1") {
			ext_kick.store(1);
			arguments["kick"] = "0";
			cout << "KICK" << endl;
		}

		if (arguments["calibrate"] == "1") {
			thread calib_thread(calibration);
			calib_thread.detach();
			arguments["calibrate"] = "0";
		}

		if (arguments["set_compass_north"] == "1") {
			ext_compass_reset.store(1);
			//compass_zero.store((compass_zero.load()+compass_degree.load()) % 360);
			arguments["set_compass_north"] = "0";
		}

		if(arguments["live_stream"] == "1") {
			int v = live_stream.load();
			live_stream.store(!v);
			if (v == 0) {
				cout << "LIVESTREAM: ON" << endl;
			} else {
				cout << "LIVESTREAM: OFF" << endl;
			}
			arguments["live_stream"] = "0";
		}

		if(arguments["reload_server"] == "1") {
			load_html_for_server();
			arguments["reload_server"] = "0";
		}
	}

	if(!strncmp(buffer, "GET /data?", 10)) {
		for(int i = 9; buffer[i] != ' ';) {
			string var = "", val = "";
			i++;
			while(buffer[i] != '=') {
				var += buffer[i];
				i++;
			}

			i++;
			while(buffer[i] != '&' && buffer[i] != ' ') {
				val += buffer[i];
				i++;
			}

			json_data[var] = val;
		}

		int _robot_speed = atoi(json_data["robot_speed"].c_str());
		if (_robot_speed != robot_speed.load()) {
			robot_speed.store(_robot_speed);
			url = "robot_speed";
		}

		int _dribbler_speed = atoi(json_data["dribbler_speed"].c_str());
		if (_dribbler_speed != ext_dribbler_speed.load()) {
			ext_dribbler_speed.store(_dribbler_speed);
			url = "dribbler_speed";
		}

		if (json_data["blue_goal"] == "true") {
			ext_attack_blue_goal.store(true);
		} else {
			ext_attack_blue_goal.store(false);
		}

		if (json_data["go"] == "1") {
			if(ext_start.load()){
				ext_start.store(false);
			} else {
				ext_start.store(true);
			}
			json_data["go"] = "0";
		}

		if (json_data["live_stream"] == "1") {
			if(ext_livestream.load()){
				ext_livestream.store(false);
			} else {
				ext_livestream.store(true);
			}
			json_data["live_stream"] = "0";
		}

		if (json_data["go_dribbler"] == "1") {
			if(ext_dribbler_start.load()){
				ext_dribbler_start.store(false);
			} else {
				ext_dribbler_start.store(true);
			}
			json_data["go_dribbler"] = "0";
		}

		if (json_data["kick"] == "1") {
			ext_kick.store(1);
			json_data["kick"] = "0";
			cout << "KICK" << endl;
		}

		if (json_data["calibrate"] == "1") {
			thread calib_thread(calibration);
			calib_thread.detach();
			json_data["calibrate"] = "0";
		}

		if (json_data["set_compass_north"] == "1") {
			ext_compass_reset.store(1);
			//compass_zero.store((compass_zero.load()+compass_degree.load()) % 360);
			json_data["set_compass_north"] = "0";
		}
	}

	size+=1;
	size%=500;

	if(!strncmp(buffer, "GET /frame",10) || !strncmp(buffer,"get /frame",10)) {
		//sprintf(bufer,"<html><meta http-equiv='refresh' content='0'>frames: %d<br>degree: %d<br><svg width='1100' height='600'><rect width='100%%' height='100%%' fill='red'/><circle cx='200' cy='200' r='%d' stroke='red' stroke-width='4' fill='yellow' /></svg></html>", frame_rate.load(), compass_degree.load(), size);
        sprintf(bufer,"<html><meta http-equiv='refresh' content='0'>frames: %d<br>degree: %d<br><img src='127.0.0.1:3000/obr.jpg'></html>", frame_rate.load(), ext_azimuth.load());
        sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nAccess-Control-Allow-Origin: *\nConnection: close\nContent-Type: html\n\n", VERSION+1, strlen(bufer)); /* Header + a blank line */
		write(fd,buffer,strlen(buffer));
		write(fd,bufer,strlen(bufer));
	} else if(strstr(buffer, "GET /data")) {
		string display_robot_speed = json_data[url];
		sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nAccess-Control-Allow-Origin: *\nConnection: close\nContent-Type: html\n\n", VERSION+2, strlen(display_robot_speed.c_str())); /* Header + a blank line */
		sprintf(bufer, "%s", display_robot_speed.c_str());
		write(fd,buffer,strlen(buffer));
		write(fd, bufer, strlen(display_robot_speed.c_str()));
	} else if(strstr(buffer, "GET /robot_status")) {
		int width = goal_width.load()*CAM_FOV / CAM_W;
		int goal_degree = (goal_x.load() - CAM_W/2)*CAM_FOV / CAM_W;
		goal_degree -= width/2;
		width = width*10/36;
		string goal_color = "yellow";
		if(ext_attack_blue_goal.load()){
			goal_color = "blue";
		}
		int azimuth = compass_degree.load();
		int level = 6-ext_ball_zone.load();
		int ball_degree = (ball_x.load() - CAM_W/2)*CAM_FOV / CAM_W;
		if(ball_degree == -51){
			level = -500;
		}
		sprintf(bufer,"--width:%d; --goal_degree:%d; --goal_color: %s; --azimuth: %d; --level:%d; --ball_degree:%d", width, goal_degree, goal_color.c_str(), azimuth, level, ball_degree);
        sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nAccess-Control-Allow-Origin: *\nConnection: close\nContent-Type: html\n\n", VERSION, strlen(bufer)); /* Header + a blank line */
		write(fd,buffer,strlen(buffer));
		write(fd,bufer,strlen(bufer));
	} else {
		sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nAccess-Control-Allow-Origin: *\nConnection: close\nContent-Type: html\n\n", VERSION, strlen(html.c_str())); /* Header + a blank line */
		write(fd,buffer,strlen(buffer));
		write(fd,html.c_str(),strlen(html.c_str()));
	}
	close(fd);
}

void stream(int fd){
	static char buffer[BUFSIZE+1]; /* static so zero filled */
	static char bufer[BUFSIZE+1];
	int _azimut = ext_azimuth.load(),
		_framerate = frame_rate.load();

	while(true) {
		if (ext_azimuth.load() != _azimut || frame_rate.load() != _framerate) {
			_azimut = ext_azimuth.load(),
			_framerate = frame_rate.load();

			sprintf(bufer,"retry: 100\ndata: fps:%d azimuth:%d\n\n", _framerate, _azimut);
			sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\ncache-control: no-cache, public\nAccess-Control-Allow-Origin: *\nConnection: keep-alive\nContent-Type: text/event-stream\n\n", VERSION, strlen(bufer)); /* Header + a blank line */
			write(fd,buffer,strlen(buffer));
			write(fd,bufer,strlen(bufer));
		} else {
			this_thread::sleep_for(chrono::milliseconds(10));
		}
		//this_thread::sleep_for(chrono::milliseconds(100));
	}
}

void start_stream(){
	int one = 1;
	long long listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

	listenfd = socket(AF_INET, SOCK_STREAM,0);

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(4000);
	bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr));
	listen(listenfd,128);

	length = sizeof(cli_addr);
	while(true){
		//web(accept(listenfd, (struct sockaddr *)&cli_addr, &length));
		socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length);
		thread stream_thread(stream, socketfd);
		stream_thread.detach();
	}
}

void start_server() {
	int one = 1;
	long long listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

	listenfd = socket(AF_INET, SOCK_STREAM,0);

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(3000);
	bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr));
	listen(listenfd,128);

	load_html_for_server();

	length = sizeof(cli_addr);
	while(true){
		//web(accept(listenfd, (struct sockaddr *)&cli_addr, &length));
		socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length);
		thread web_thread(web, socketfd);
		web_thread.detach();
	}
}

void init_server() {
	thread server(start_server);
	server.detach();

	this_thread::sleep_for(chrono::milliseconds(100));

	/*thread server2(start_stream);
	server2.detach();*/
}

/*
FILE *fp;
fp = fopen("/mnt/ramdisk/obr.jpg", "rb");
fseek(fp, 0, SEEK_END);
int length = ftell(fp);
rewind(fp);
char *file_data = (char *)malloc((length+1)*sizeof(char));
fread(file_data, length, 1, fp);
 */
