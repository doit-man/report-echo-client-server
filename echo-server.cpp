#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <thread>

using namespace std;

int sd_arr[1000]= {0};
int count=0;


void usage() {
	cout << "syntax: ts <port> [-an][-e[-b]]\n";
	cout << "  -an: auto newline\n";
	cout << "  -e : echo\n";
	cout << "sample: ts 1234\n";
}

struct Param {
	bool autoNewline{false};
	bool echo{false};
	bool broad_cast{false};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-an") == 0) {
				autoNewline = true;
				continue;
			}
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				if((strcmp(argv[i+1],"-b") == 0)){
					broad_cast = true;
					i++;
					continue;
				}
				continue;
			}
			port = stoi(argv[i]);
		}
		return port != 0;
	}
} param;

void recvThread(int sd) {
	cout << "connected\n";
	int index=count;
	count++;
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	
	while (true) {
		ssize_t res = recv(sd, buf, BUFSIZE - 1, 0);
		sd_arr[index]=sd;
		if (res == 0 || res == -1) {
			cerr << "recv return " << res << endl;
			perror("recv");
			break;
		}
		buf[res] = '\0';
		if (param.autoNewline)
			cout << buf << endl;
		else {
			cout << buf;
			cout.flush();
		}
		if (param.echo) {
			if(param.broad_cast){
				for(int i=0; i<count;i++){
					//printf("sd_arr[i] = %d, count = %d \n",sd_arr[i],count);
					res = send(sd_arr[i], buf, res, 0);
        	                        if (res == 0 || res == -1) {
                	                        cerr << "send return " << res << endl;
                        	                perror("send");
                                	        break;
                                	}

				}
			
			}
			else{
				res = send(sd, buf, res, 0);
				if (res == 0 || res == -1) {
					cerr << "send return " << res << endl;
					perror("send");
					break;
				}
			}
		}
	}
	cout << "disconnected\n";
    close(sd);
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}
		
	int optval = 1;
	int res = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}
	
	res = listen(sd, 5);
	if (res == -1) {
		perror("listen");
		return -1;
	}
	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);

		int cli_sd = accept(sd, (struct sockaddr *)&cli_addr, &len);
		if (cli_sd == -1) {
			perror("accept");
			break;
		}
		//printf("sd = %d\n",sd);
		thread* t = new thread(recvThread, cli_sd);
		t->detach();
	}
	close(sd);
}
