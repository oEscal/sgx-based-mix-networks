#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <regex.h>
#include <thread>
#include <iostream>
#include <vector>
#include <thread>
#include <unistd.h>
#include <memory>
#include <exception>
#include <algorithm>

#include "Enclave_u.h"

using namespace std;

class Mix_Worker{

private:
	std::string ReceiverPort ="";

	sgx_enclave_id_t eid;
	unsigned char my_public_module[256];

	// flags
	int fan_all_out = 0;
	bool flag_received_previous_module = false; 
	bool flag_previous_started_fan_out = false;

	// ports of other mixes and consumer/producer
	int prev_port;
	int next_port;
	int producer_port;

	// sockets variables
	int sending_rate = 1000000/50;          // milliseconds
	int sockfd = 0, newsockfd = 0, portno = 0;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	// functions
	bool send_message_to_socket(string sender_name, int port, void *content, int size);
	void send_with_retry(string sender_name, int port, void *content, int size);
	void send_message_worker();
	void receive_messages();

public:
	Mix_Worker(string, int, int, int, sgx_enclave_id_t*);
	void start();
};
