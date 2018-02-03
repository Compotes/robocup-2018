#include "Server.hpp"

using namespace std;

int size = 0;
int A[20];
string html = "";
bool go = false;

map<string, string> arguments;

void load_html_for_server() {
	fstream html_file;
	html_file.open("server.html", ios::in);

	string line;
	html = "";
	while(getline(html_file, line)) {
		html += line;
	}

	cout << "HTML LOADED" << endl;
}

int web(int fd) {
	int j, file_fd, buflen;
	long i, ret, len;
	char * fstr;
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

		if (arguments["go"] == "1") {
			if(ext_start.load()){
				ext_start.store(false);
			} else {
				ext_start.store(true);
			}

			arguments["go"] = "0";
		}

		if (arguments["set_compass_nord"] == "1") {
			compass_zero.store((compass_zero.load()+compass_degree.load()) % 360);
			arguments["set_compass_nord"] = "0";
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

		if (arguments["load_values"] == "1") {
			load_values();
			arguments["load_values"] = "0";
		}
	}

	size+=1;
	size%=500;

	if(!strncmp(buffer, "GET /frame",10) || !strncmp(buffer,"get /frame",10)) {
		//sprintf(bufer,"<html><meta http-equiv='refresh' content='0'>frames: %d<br>degree: %d<br><svg width='1100' height='600'><rect width='100%%' height='100%%' fill='red'/><circle cx='200' cy='200' r='%d' stroke='red' stroke-width='4' fill='yellow' /></svg></html>", frame_rate.load(), compass_degree.load(), size);
        sprintf(bufer,"<html><meta http-equiv='refresh' content='0'>frames: %d<br>degree: %d<br><img src='127.0.0.1:3000/obr.jpg'></html>", frame_rate.load(), compass_degree.load(), size);
        sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: html\n\n", VERSION, strlen(bufer)); /* Header + a blank line */
		write(fd,buffer,strlen(buffer));
		write(fd,bufer,strlen(bufer));
	} else if(strstr(buffer, ".jpg") || strstr(buffer, ".jpeg")) {
		FILE *fp;
		fp = fopen("/mnt/ramdisk/obr.jpg", "rb");
		fseek(fp, 0, SEEK_END);
		int length = ftell(fp);
		rewind(fp);
		char *file_data = (char *)malloc((length+1)*sizeof(char));
		fread(file_data, length, 1, fp);

		sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection: close\nRefresh: 1\nContent-Type: image/jpeg\r\n\r\n", VERSION, strlen(bufer) + length);
		write(fd, buffer, strlen(buffer));
		write(fd, file_data, length);
    } else {
		sprintf(buffer,"HTTP/1.1 200 OK\nServer: nweb/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: html\n\n", VERSION, strlen(html.c_str())); /* Header + a blank line */
		write(fd,buffer,strlen(buffer));
		write(fd,html.c_str(),strlen(html.c_str()));
	}
	close(fd);
}

void start_server() {
	A[1] = 1;
	A[2] = 500;
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
	while(socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)){
		web(socketfd);
	}
}

void init_server() {
	thread server(start_server);
	server.detach();
}
